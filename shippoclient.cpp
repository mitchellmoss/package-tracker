#include "shippoclient.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QUrl>
#include <QDateTime>

ShippoClient::ShippoClient(const QString& apiToken, QObject *parent)
    : QObject(parent), apiToken(apiToken)
{
    manager = new QNetworkAccessManager(this);
    connect(manager, &QNetworkAccessManager::finished, this, &ShippoClient::onRequestFinished);
}

void ShippoClient::trackPackage(const QString& trackingNumber)
{
    QUrl url(QString("https://api.goshippo.com/tracks/%1").arg(trackingNumber));
    QNetworkRequest request(url);
    
    request.setRawHeader("Authorization", QString("ShippoToken %1").arg(apiToken).toLatin1());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    qDebug() << "Tracking package:" << trackingNumber;
    manager->get(request);
}

void ShippoClient::onRequestFinished(QNetworkReply* reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        QByteArray errorData = reply->readAll();
        QString errorMsg = QString("Network error: %1\nResponse: %2")
                          .arg(reply->errorString())
                          .arg(QString(errorData));
        qDebug() << "API Error:" << errorMsg;
        emit trackingError(errorMsg);
        reply->deleteLater();
        return;
    }

    QByteArray responseData = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(responseData);
    
    if (!doc.isObject()) {
        emit trackingError("Invalid response format");
        reply->deleteLater();
        return;
    }

    QJsonObject response = doc.object();
    QJsonObject result;

    // Map Shippo tracking data to our internal format
    result["trackingNumber"] = response["tracking_number"].toString();
    result["carrier"] = response["carrier"].toString();
    result["status"] = response["tracking_status"].toObject()["status"].toString();
    
    if (response.contains("eta")) {
        result["estimatedDelivery"] = response["eta"].toString();
    }

    // Convert tracking history
    QJsonArray events;
    QJsonArray trackingHistory = response["tracking_history"].toArray();
    for (const QJsonValue& event : trackingHistory) {
        QJsonObject trackEvent = event.toObject();
        events.append(QJsonObject{
            {"timestamp", trackEvent["status_date"].toString()},
            {"description", trackEvent["status_details"].toString()},
            {"location", trackEvent["location"].toObject()["city"].toString() + 
                        ", " + trackEvent["location"].toObject()["state"].toString()}
        });
    }
    result["events"] = events;

    emit trackingInfoReceived(result);
    reply->deleteLater();
}
