#include "upsclient.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <QEventLoop>
#include <QTimer>
#include <QDateTime>
#include <QDebug>

UPSClient::UPSClient(const QString& clientId, const QString& clientSecret, QObject *parent)
    : QObject(parent), clientId(clientId), clientSecret(clientSecret)
{
    manager = new QNetworkAccessManager(this);
    connect(manager, &QNetworkAccessManager::finished, this, &UPSClient::onRequestFinished);
}

void UPSClient::trackPackage(const QString& trackingNumber)
{
    QString token = getAuthToken();
    if (token.isEmpty()) {
        emit trackingError("Failed to get authentication token");
        return;
    }

    QUrl url("https://onlinetools.ups.com/api/track/v1/details/" + trackingNumber);
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("transId", "TRACK" + QDateTime::currentDateTime().toString("yyyyMMddhhmmss").toUtf8());
    request.setRawHeader("transactionSrc", "PackageTracker");

    // Add debug output
    qDebug() << "Tracking package:" << trackingNumber;
    qDebug() << "Using token:" << token;

    QNetworkReply* reply = manager->get(request);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "Tracking Error:" << reply->errorString();
            qDebug() << "Response:" << reply->readAll();
            emit trackingError(reply->errorString());
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isObject()) {
            emit trackingError("Invalid response format");
            return;
        }

        emit trackingInfoReceived(doc.object());
    });
}

void UPSClient::subscribeToTracking(const QString& trackingNumber, const QString& callbackUrl)
{
    QString token = getAuthToken();
    if (token.isEmpty()) {
        emit trackingError("Failed to get authentication token");
        return;
    }

    this->callbackUrl = callbackUrl;

    QUrl url("https://onlinetools.ups.com/api/track/v1/subscriptions/standard/package");
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("transId", "SUB" + QDateTime::currentDateTime().toString("yyyyMMddhhmmss").toUtf8());
    request.setRawHeader("transactionSrc", "PackageTracker");

    // Create subscription payload
    QJsonObject payload;
    payload["locale"] = "en_US";
    payload["countryCode"] = "US";
    payload["trackingNumberList"] = QJsonArray{trackingNumber};
    payload["destination"] = QJsonObject{
        {"url", callbackUrl},
        {"credentialType", "Bearer"},
        {"credential", token}
    };

    qDebug() << "Subscribing to tracking:" << trackingNumber;
    qDebug() << "Callback URL:" << callbackUrl;

    QNetworkReply* reply = manager->post(request, QJsonDocument(payload).toJson());
    
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "Subscription Error:" << reply->errorString();
            qDebug() << "Response:" << reply->readAll();
            emit trackingError(reply->errorString());
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isObject()) {
            emit trackingError("Invalid subscription response");
            return;
        }

        qDebug() << "Subscription successful:" << doc.toJson();
    });
}

void UPSClient::onTrackingEvent(QNetworkReply* reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "Tracking Event Error:" << reply->errorString();
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    if (!doc.isObject()) {
        qDebug() << "Invalid tracking event format";
        return;
    }

    handleTrackingEvent(doc.object());
}

void UPSClient::handleTrackingEvent(const QJsonObject& event)
{
    // Parse the tracking event according to UPS API format
    QJsonObject result;
    result["trackingNumber"] = event["trackingNumber"].toString();
    result["status"] = event["activityStatus"].toObject()["description"].toString();
    
    QJsonArray events;
    events.append(QJsonObject{
        {"timestamp", event["localActivityDate"].toString() + " " + event["localActivityTime"].toString()},
        {"description", event["activityStatus"].toObject()["description"].toString()},
        {"location", event["activityLocation"].toObject()["city"].toString() + ", " + 
                   event["activityLocation"].toObject()["stateProvince"].toString()}
    });
    
    result["events"] = events;
    
    if (event.contains("scheduledDeliveryDate")) {
        result["estimatedDelivery"] = event["scheduledDeliveryDate"].toString();
    }
    
    emit trackingEventReceived(result);
}

QString UPSClient::getAuthToken()
{
    QUrl url("https://onlinetools.ups.com/security/v1/oauth/token");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setRawHeader("x-merchant-id", "string"); // Required header

    // Create proper Basic Auth header
    QString auth = QString("%1:%2").arg(clientId).arg(clientSecret);
    QString authHeader = "Basic " + auth.toUtf8().toBase64();
    request.setRawHeader("Authorization", authHeader.toUtf8());

    // Create form data
    QUrlQuery params;
    params.addQueryItem("grant_type", "client_credentials");

    // Add debug output
    qDebug() << "Requesting UPS auth token with client ID:" << clientId;
    qDebug() << "Auth header:" << authHeader;

    QNetworkReply* reply = manager->post(request, params.toString(QUrl::FullyEncoded).toUtf8());
    
    // Wait for reply with timeout
    QEventLoop loop;
    QTimer::singleShot(10000, &loop, &QEventLoop::quit); // Increased timeout
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "Auth Error:" << reply->errorString();
        qDebug() << "Response:" << reply->readAll();
        return QString();
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    if (!doc.isObject()) {
        qDebug() << "Invalid JSON response";
        return QString();
    }

    QJsonObject obj = doc.object();
    if (!obj.contains("access_token")) {
        qDebug() << "No access token in response:" << obj;
        return QString();
    }

    return obj["access_token"].toString();
}

void UPSClient::onRequestFinished(QNetworkReply* reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        emit trackingError(reply->errorString());
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    if (!doc.isObject()) {
        emit trackingError("Invalid response format");
        return;
    }

    QJsonObject result;
    QJsonObject trackResponse = doc.object()["trackResponse"].toObject();
    QJsonObject shipment = trackResponse["shipment"].toArray().first().toObject();
    
    result["trackingNumber"] = shipment["trackingNumber"].toString();
    result["status"] = shipment["currentStatus"].toObject()["description"].toString();
    
    QJsonArray events;
    for (const QJsonValue& activity : shipment["activity"].toArray()) {
        QJsonObject event = activity.toObject();
        events.append(QJsonObject{
            {"timestamp", event["date"].toString() + " " + event["time"].toString()},
            {"description", event["status"].toObject()["description"].toString()},
            {"location", event["location"].toObject()["address"].toObject()["city"].toString() + ", " + 
                       event["location"].toObject()["address"].toObject()["stateProvince"].toString()}
        });
    }
    result["events"] = events;
    
    if (shipment.contains("deliveryDate")) {
        QJsonObject delivery = shipment["deliveryDate"].toObject();
        result["estimatedDelivery"] = delivery["date"].toString();
    }
    
    emit trackingInfoReceived(result);
}
