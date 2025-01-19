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
    
signals:
    void trackingInfoReceived(const QJsonObject& info);
    void trackingError(const QString& error);
    
private slots:
    void onRequestFinished(QNetworkReply* reply);
    
private:
    QNetworkAccessManager* manager;
    QString clientId;
    QString clientSecret;
    
    QString getAuthToken();
};

#endif // UPSCLIENT_H
