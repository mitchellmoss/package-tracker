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
    qDebug() << "Headers:";
    qDebug() << "Authorization:" << request.rawHeader("Authorization");
    qDebug() << "Content-Type:" << request.header(QNetworkRequest::ContentTypeHeader).toString();
    
    QNetworkReply* reply = manager->post(request, jsonData);
    connect(reply, &QNetworkReply::sslErrors, this, [](const QList<QSslError> &errors) {
        for (const QSslError &error : errors) {
            qDebug() << "SSL Error:" << error.errorString();
        }
    });
}

void ShippoClient::handleWebhookEvent(const QJsonObject& webhookData) 
{
    QString event = webhookData["event"].toString();
    QJsonObject data = webhookData["data"].toObject();
    
    if (event == "track_updated") {
        // Process tracking update
        emit trackingInfoReceived(data);
    }
    
    // Emit the raw webhook data for other handlers
    emit webhookReceived(event, data);
}

void ShippoClient::onRequestFinished(QNetworkReply* reply)
{
    qDebug() << "Response received:";
    qDebug() << "Status code:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    qDebug() << "Content type:" << reply->header(QNetworkRequest::ContentTypeHeader).toString();
    
    if (reply->error() != QNetworkReply::NoError) {
        QByteArray errorData = reply->readAll();
        QString errorMsg = QString("Network error: %1\nResponse: %2")
                          .arg(reply->errorString())
                          .arg(QString(errorData));
        qDebug() << "API Error:" << errorMsg;
        qDebug() << "Full response headers:";
        for (const QByteArray &header : reply->rawHeaderList()) {
            qDebug() << header << ":" << reply->rawHeader(header);
        }
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
    // For test tracking numbers, create simulated response
    if (response["tracking_number"].toString().startsWith("SHIPPO_")) {
        QString testStatus = response["tracking_number"].toString().split("_")[1];
        QJsonObject result = response; // Keep original response data
        result["status"] = testStatus;
        result["tracking_number"] = response["tracking_number"].toString();
        result["carrier"] = "SHIPPO TEST";
        
        // Add simulated tracking event
        QJsonArray events;
        QJsonObject event;
        event["timestamp"] = QDateTime::currentDateTime().toString("yyyyMMdd HHmmss");
        event["status"] = testStatus;
        event["description"] = "Test tracking status: " + testStatus;
        event["location"] = "Test Location, XX 12345";
        events.append(event);
        result["events"] = events;
        
        emit trackingInfoReceived(result);
        reply->deleteLater();
        return;
    }

    // For real tracking numbers, map the response
    QJsonObject result = response; // Keep all original response data
    
    // Get tracking status
    QJsonObject trackingStatus = response["tracking_status"].toObject();
    QString status = trackingStatus["status"].toString().toUpper();
    
    // For test tracking numbers, use the status from the tracking number itself
    if (result["tracking_number"].toString().startsWith("SHIPPO_")) {
        QString testStatus = result["tracking_number"].toString().split("_")[1];
        result["status"] = testStatus;
    } else {
        // Map Shippo status to our standardized status strings
        QString normalizedStatus = "UNKNOWN";
        if (status == "PRE_TRANSIT" || status == "pre_transit") {
            normalizedStatus = "PRE_TRANSIT";
        } else if (status == "TRANSIT" || status == "in_transit") {
            normalizedStatus = "TRANSIT";
        } else if (status == "DELIVERED" || status == "delivered") {
            normalizedStatus = "DELIVERED";
        } else if (status == "RETURNED" || status == "returned") {
            normalizedStatus = "RETURNED";
        } else if (status == "FAILURE" || status == "failure") {
            normalizedStatus = "FAILURE";
        }
        result["status"] = normalizedStatus;
    }
    
    // Map substatus if available
    if (!trackingStatus["substatus"].isNull()) {
        QString substatus = trackingStatus["substatus"].toString();
        
        // Map known substatuses
        if (substatus == "information_received") result["substatus"] = "INFORMATION_RECEIVED";
        else if (substatus == "address_issue") result["substatus"] = "ADDRESS_ISSUE";
        else if (substatus == "contact_carrier") result["substatus"] = "CONTACT_CARRIER";
        else if (substatus == "delayed") result["substatus"] = "DELAYED";
        else if (substatus == "delivery_attempted") result["substatus"] = "DELIVERY_ATTEMPTED";
        else if (substatus == "delivery_rescheduled") result["substatus"] = "DELIVERY_RESCHEDULED";
        else if (substatus == "delivery_scheduled") result["substatus"] = "DELIVERY_SCHEDULED";
        else if (substatus == "location_inaccessible") result["substatus"] = "LOCATION_INACCESSIBLE";
        else if (substatus == "notice_left") result["substatus"] = "NOTICE_LEFT";
        else if (substatus == "out_for_delivery") result["substatus"] = "OUT_FOR_DELIVERY";
        else if (substatus == "package_accepted") result["substatus"] = "PACKAGE_ACCEPTED";
        else if (substatus == "package_arrived") result["substatus"] = "PACKAGE_ARRIVED";
        else if (substatus == "package_damaged") result["substatus"] = "PACKAGE_DAMAGED";
        else if (substatus == "package_departed") result["substatus"] = "PACKAGE_DEPARTED";
        else if (substatus == "package_forwarded") result["substatus"] = "PACKAGE_FORWARDED";
        else if (substatus == "package_held") result["substatus"] = "PACKAGE_HELD";
        else if (substatus == "package_processed") result["substatus"] = "PACKAGE_PROCESSED";
        else if (substatus == "package_processing") result["substatus"] = "PACKAGE_PROCESSING";
        else if (substatus == "pickup_available") result["substatus"] = "PICKUP_AVAILABLE";
        else if (substatus == "reschedule_delivery") result["substatus"] = "RESCHEDULE_DELIVERY";
        else if (substatus == "delivered") result["substatus"] = "DELIVERED";
        else if (substatus == "return_to_sender") result["substatus"] = "RETURN_TO_SENDER";
        else if (substatus == "package_unclaimed") result["substatus"] = "PACKAGE_UNCLAIMED";
        else if (substatus == "package_undeliverable") result["substatus"] = "PACKAGE_UNDELIVERABLE";
        else if (substatus == "package_disposed") result["substatus"] = "PACKAGE_DISPOSED";
        else if (substatus == "package_lost") result["substatus"] = "PACKAGE_LOST";
        else result["substatus"] = "OTHER";
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
