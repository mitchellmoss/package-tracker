#include "upsclient.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <QEventLoop>
#include <QTimer>

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

    manager->get(request);
}

QString UPSClient::getAuthToken()
{
    QUrl url("https://onlinetools.ups.com/security/v1/oauth/token");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QUrlQuery params;
    params.addQueryItem("grant_type", "client_credentials");

    QString auth = QString("%1:%2").arg(clientId).arg(clientSecret);
    request.setRawHeader("Authorization", "Basic " + auth.toUtf8().toBase64());

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

    emit trackingInfoReceived(doc.object());
}
