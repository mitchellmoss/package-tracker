#ifndef FEDEXCLIENT_H
#define FEDEXCLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class FedExClient : public QObject
{
    Q_OBJECT
    
public:
    explicit FedExClient(const QString& apiKey, const QString& apiSecret, QObject *parent = nullptr);
    void trackPackage(const QString& trackingNumber);
    
signals:
    void trackingInfoReceived(const QJsonObject& info);
    void trackingError(const QString& error);
    
private slots:
    void onRequestFinished(QNetworkReply* reply);
    
private:
    QNetworkAccessManager* manager;
    QString apiKey;
    QString apiSecret;
    
    QString getAuthToken();
};

#endif // FEDEXCLIENT_H
