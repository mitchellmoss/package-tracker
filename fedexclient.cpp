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

    QJsonObject payload;
    payload["includeDetailedScans"] = true;
    QJsonObject trackingInfo;
    trackingInfo["trackingNumberInfo"] = QJsonObject{
        {"trackingNumber", trackingNumber}
    };
    payload["trackingInfo"] = QJsonArray{trackingInfo};

    manager->post(request, QJsonDocument(payload).toJson());
}

QString FedExClient::getAuthToken()
{
    QUrl url("https://apis.fedex.com/oauth/token");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QUrlQuery params;
    params.addQueryItem("grant_type", "client_credentials");
    params.addQueryItem("client_id", apiKey);
    params.addQueryItem("client_secret", apiSecret);

    QNetworkReply* reply = manager->post(request, params.toString(QUrl::FullyEncoded).toUtf8());
    
    // Wait for reply with timeout
    QEventLoop loop;
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        return QString();
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    return doc.object()["access_token"].toString();
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

    emit trackingInfoReceived(doc.object());
}
