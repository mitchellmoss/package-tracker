#include "fedexclient.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <QEventLoop>
#include <QTimer>

FedExClient::FedExClient(const QString& apiKey, const QString& apiSecret, QObject *parent)
    : QObject(parent), apiKey(apiKey), apiSecret(apiSecret)
{
    manager = new QNetworkAccessManager(this);
    connect(manager, &QNetworkAccessManager::finished, this, &FedExClient::onRequestFinished);
}

void FedExClient::trackPackage(const QString& trackingNumber)
{
    QString token = getAuthToken();
    if (token.isEmpty()) {
        emit trackingError("Failed to get authentication token");
        return;
    }

    QUrl url("https://apis.fedex.com/track/v1/trackingnumbers");
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    // Add required headers
    QString transactionId = QUuid::createUuid().toString();
    request.setRawHeader("x-customer-transaction-id", transactionId.toUtf8());
    request.setRawHeader("x-locale", "en_US");

    QJsonObject trackingNumberInfo;
    trackingNumberInfo["trackingNumber"] = trackingNumber;

    QJsonObject payload;
    payload["includeDetailedScans"] = true;
    payload["trackingInfo"] = QJsonArray{trackingNumberInfo};

    manager->post(request, QJsonDocument(payload).toJson());
}

QString FedExClient::getAuthToken()
{
    QUrl url("https://apis.fedex.com/oauth/token");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    // Create proper Basic Auth header
    QString auth = QString("%1:%2").arg(apiKey).arg(apiSecret);
    QString authHeader = "Basic " + auth.toUtf8().toBase64();
    request.setRawHeader("Authorization", authHeader.toUtf8());

    // Create form data
    QUrlQuery params;
    params.addQueryItem("grant_type", "client_credentials");

    QNetworkReply* reply = manager->post(request, params.toString(QUrl::FullyEncoded).toUtf8());
    
    // Wait for reply with timeout
    QEventLoop loop;
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        QString response = reply->readAll();
        qDebug() << "FedEx Auth Error:" << reply->errorString();
        qDebug() << "Status Code:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        qDebug() << "Response:" << response;
        emit trackingError(QString("FedEx Auth Error: %1\nResponse: %2").arg(reply->errorString()).arg(response));
        return QString();
    }

    QByteArray responseData = reply->readAll();
    qDebug() << "Raw UPS auth response:" << responseData;
    
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        qDebug() << "JSON parse error:" << parseError.errorString();
        emit trackingError("Failed to parse UPS auth response");
        return QString();
    }
    
    if (!doc.isObject()) {
        qDebug() << "Invalid JSON response format";
        emit trackingError("Invalid UPS auth response format");
        return QString();
    }
    
    QJsonObject obj = doc.object();
    if (!obj.contains("access_token")) {
        qDebug() << "No access token in response:" << obj;
        emit trackingError("No access token in UPS response");
        return QString();
    }

    QString token = obj["access_token"].toString();
    qDebug() << "Successfully retrieved UPS token";
    return token;
}

void FedExClient::onRequestFinished(QNetworkReply* reply)
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
    QJsonObject response = doc.object();
    
    if (!response.contains("output")) {
        emit trackingError("Invalid response format - missing output");
        return;
    }

    QJsonObject output = response["output"].toObject();
    if (!output.contains("completeTrackResults")) {
        emit trackingError("Invalid response format - missing track results");
        return;
    }

    QJsonArray trackResults = output["completeTrackResults"].toArray();
    if (trackResults.isEmpty()) {
        if (output.contains("alerts")) {
            emit trackingError(output["alerts"].toString());
        } else {
            emit trackingError("No tracking results found");
        }
        return;
    }

    QJsonObject trackResult = trackResults.first().toObject();
    if (!trackResult.contains("trackingNumber")) {
        emit trackingError("Invalid response format - missing tracking number");
        return;
    }

    result["trackingNumber"] = trackResult["trackingNumber"].toString();
    
    if (trackResult.contains("latestStatusDetail")) {
        QJsonObject status = trackResult["latestStatusDetail"].toObject();
        result["status"] = status["description"].toString();
    }

    QJsonArray events;
    if (trackResult.contains("scanEvents")) {
        for (const QJsonValue& scan : trackResult["scanEvents"].toArray()) {
            QJsonObject event = scan.toObject();
            QString location;
            if (event.contains("scanLocation")) {
                QJsonObject loc = event["scanLocation"].toObject();
                QString city = loc["city"].toString();
                QString state = loc["stateOrProvinceCode"].toString();
                location = city + (state.isEmpty() ? "" : ", " + state);
            }
            
            events.append(QJsonObject{
                {"timestamp", event["date"].toString() + " " + event["time"].toString()},
                {"description", event["eventDescription"].toString()},
                {"location", location}
            });
        }
    }
    result["events"] = events;

    if (trackResult.contains("estimatedDeliveryTimeWindow")) {
        QJsonObject window = trackResult["estimatedDeliveryTimeWindow"].toObject();
        if (window.contains("window")) {
            QJsonObject timeWindow = window["window"].toObject();
            result["estimatedDelivery"] = timeWindow["begins"].toString() + " - " + 
                                        timeWindow["ends"].toString();
        }
    }
    
    emit trackingInfoReceived(result);
}
