#ifndef SHIPPOCLIENT_H
#define SHIPPOCLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class ShippoClient : public QObject
{
    Q_OBJECT
    
public:
    explicit ShippoClient(const QString& apiToken, QObject *parent = nullptr);
    void trackPackage(const QString& trackingNumber);
    
signals:
    void trackingInfoReceived(const QJsonObject& info);
    void trackingError(const QString& error);
    
private slots:
    void onRequestFinished(QNetworkReply* reply);
    
private:
    QNetworkAccessManager* manager;
    QString apiToken;
};

#endif // SHIPPOCLIENT_H
