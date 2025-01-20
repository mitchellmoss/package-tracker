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
    QUrl url("https://api.goshippo.com/tracks/");
    QNetworkRequest request(url);
    
    // Fix auth header format per Shippo API docs
    request.setRawHeader("Authorization", QString("ShippoToken %1").arg(apiToken).toUtf8());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    qDebug() << "Tracking package:" << trackingNumber;
    qDebug() << "URL:" << url.toString();
    
    // Always use POST with carrier and tracking number for consistency
    QJsonObject postData;
    postData["carrier"] = "shippo";  // Use shippo carrier for test numbers
    postData["tracking_number"] = trackingNumber;
    postData["metadata"] = "Order " + trackingNumber;  // Add metadata for reference
    
    QJsonDocument doc(postData);
    QByteArray jsonData = doc.toJson();
    qDebug() << "POST data:" << QString(jsonData);
    
    manager->post(request, jsonData);
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
    
    // Get tracking status
    QJsonObject trackingStatus = response["tracking_status"].toObject();
    result["status"] = trackingStatus["status"].toString();
    
    // Include substatus if available
    if (!trackingStatus["substatus"].isNull()) {
        result["substatus"] = trackingStatus["substatus"].toString();
    }
    
    if (response.contains("eta")) {
        result["estimatedDelivery"] = response["eta"].toString();
    }

    // Add service level information if available
    if (response.contains("servicelevel")) {
        QJsonObject serviceLevel = response["servicelevel"].toObject();
        result["service"] = serviceLevel["name"].toString();
    }

    // Add address information
    if (response.contains("address_from")) {
        QJsonObject fromAddr = response["address_from"].toObject();
        result["fromLocation"] = QString("%1, %2 %3")
            .arg(fromAddr["city"].toString())
            .arg(fromAddr["state"].toString())
            .arg(fromAddr["zip"].toString());
    }
    
    if (response.contains("address_to")) {
        QJsonObject toAddr = response["address_to"].toObject();
        result["toLocation"] = QString("%1, %2 %3")
            .arg(toAddr["city"].toString())
            .arg(toAddr["state"].toString())
            .arg(toAddr["zip"].toString());
    }

    // Convert tracking history
    QJsonArray events;
    QJsonArray trackingHistory = response["tracking_history"].toArray();
    for (const QJsonValue& event : trackingHistory) {
        QJsonObject trackEvent = event.toObject();
        QJsonObject location = trackEvent["location"].toObject();
        
        QJsonObject eventObj;
        eventObj["timestamp"] = trackEvent["status_date"].toString();
        eventObj["status"] = trackEvent["status"].toString();
        eventObj["description"] = trackEvent["status_details"].toString();
        eventObj["location"] = QString("%1, %2 %3")
            .arg(location["city"].toString())
            .arg(location["state"].toString())
            .arg(location["zip"].toString());
            
        if (!trackEvent["substatus"].isNull()) {
            eventObj["substatus"] = trackEvent["substatus"].toString();
        }
        
        events.append(eventObj);
    }
    result["events"] = events;

    emit trackingInfoReceived(result);
    reply->deleteLater();
}
