#ifndef SHIPPOCLIENT_H
#define SHIPPOCLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

// Shippo tracking statuses
enum class ShippoStatus {
    PRE_TRANSIT,
    TRANSIT, 
    DELIVERED,
    RETURNED,
    FAILURE,
    UNKNOWN
};

// Shippo tracking substatuses
enum class ShippoSubstatus {
    NONE,
    // PRE_TRANSIT
    INFORMATION_RECEIVED,
    // TRANSIT
    ADDRESS_ISSUE,
    CONTACT_CARRIER,
    DELAYED,
    DELIVERY_ATTEMPTED, 
    DELIVERY_RESCHEDULED,
    DELIVERY_SCHEDULED,
    LOCATION_INACCESSIBLE,
    NOTICE_LEFT,
    OUT_FOR_DELIVERY,
    PACKAGE_ACCEPTED,
    PACKAGE_ARRIVED,
    PACKAGE_DAMAGED,
    PACKAGE_DEPARTED,
    PACKAGE_FORWARDED,
    PACKAGE_HELD,
    PACKAGE_PROCESSED,
    PACKAGE_PROCESSING,
    PICKUP_AVAILABLE,
    RESCHEDULE_DELIVERY,
    // DELIVERED
    DELIVERED,
    // RETURNED
    RETURN_TO_SENDER,
    PACKAGE_UNCLAIMED,
    // FAILURE  
    PACKAGE_UNDELIVERABLE,
    PACKAGE_DISPOSED,
    PACKAGE_LOST,
    // UNKNOWN
    OTHER
};

class ShippoClient : public QObject
{
    Q_OBJECT
    
public:
    explicit ShippoClient(const QString& apiToken, QObject *parent = nullptr);
    void trackPackage(const QString& trackingNumber);
    void handleWebhookEvent(const QJsonObject& webhookData);
    
signals:
    void trackingInfoReceived(const QJsonObject& info);
    void trackingError(const QString& error);
    void webhookReceived(const QString& event, const QJsonObject& data);
    
private slots:
    void onRequestFinished(QNetworkReply* reply);
    
private:
    QNetworkAccessManager* manager;
    QString apiToken;
};

#endif // SHIPPOCLIENT_H
