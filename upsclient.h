#ifndef UPSCLIENT_H
#define UPSCLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class UPSClient : public QObject
{
    Q_OBJECT
    
public:
    explicit UPSClient(const QString& clientId, const QString& clientSecret, QObject *parent = nullptr);
    void trackPackage(const QString& trackingNumber);
    void subscribeToTracking(const QString& trackingNumber, const QString& callbackUrl);
    
signals:
    void trackingInfoReceived(const QJsonObject& info);
    void trackingError(const QString& error);
    void trackingEventReceived(const QJsonObject& event);
    
private slots:
    void onRequestFinished(QNetworkReply* reply);
    void onTrackingEvent(QNetworkReply* reply);
    
private:
    QNetworkAccessManager* manager;
    QString clientId;
    QString clientSecret;
    QString callbackUrl;
    
    QString getAuthToken();
    bool verifyCredentials();
    void handleTrackingEvent(const QJsonObject& event);
};

#endif // UPSCLIENT_H
