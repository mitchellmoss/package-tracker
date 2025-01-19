#include "upsclient.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <QEventLoop>
#include <QTimer>
#include <QDateTime>
#include <QDebug>

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
    request.setRawHeader("transId", "TRACK" + QDateTime::currentDateTime().toString("yyyyMMddhhmmss").toUtf8());
    request.setRawHeader("transactionSrc", "PackageTracker");

    // Add debug output
    qDebug() << "Tracking package:" << trackingNumber;
    qDebug() << "Using token:" << token;

    QNetworkReply* reply = manager->get(request);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "Tracking Error:" << reply->errorString();
            qDebug() << "Response:" << reply->readAll();
            emit trackingError(reply->errorString());
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isObject()) {
            emit trackingError("Invalid response format");
            return;
        }

        emit trackingInfoReceived(doc.object());
    });
}

void UPSClient::subscribeToTracking(const QString& trackingNumber, const QString& callbackUrl)
{
    QString token = getAuthToken();
    if (token.isEmpty()) {
        emit trackingError("Failed to get authentication token");
        return;
    }

    this->callbackUrl = callbackUrl;

    QUrl url("https://onlinetools.ups.com/api/track/v1/subscriptions/standard/package");
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("transId", "SUB" + QDateTime::currentDateTime().toString("yyyyMMddhhmmss").toUtf8());
    request.setRawHeader("transactionSrc", "PackageTracker");

    // Create subscription payload
    QJsonObject payload;
    payload["locale"] = "en_US";
    payload["countryCode"] = "US";
    payload["trackingNumberList"] = QJsonArray{trackingNumber};
    payload["destination"] = QJsonObject{
        {"url", callbackUrl},
        {"credentialType", "Bearer"},
        {"credential", token}
    };

    qDebug() << "Subscribing to tracking:" << trackingNumber;
    qDebug() << "Callback URL:" << callbackUrl;

    qDebug() << "Tracking FedEx package:" << trackingNumber;
    qDebug() << "Using token:" << token;

    QNetworkReply* reply = manager->post(request, QJsonDocument(payload).toJson());
    
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "FedEx Tracking Error:" << reply->errorString();
            qDebug() << "Response:" << reply->readAll();
            emit trackingError(reply->errorString());
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isObject()) {
            emit trackingError("Invalid response format");
            return;
        }

        emit trackingInfoReceived(doc.object());
    });
    
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "Subscription Error:" << reply->errorString();
            qDebug() << "Response:" << reply->readAll();
            emit trackingError(reply->errorString());
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isObject()) {
            emit trackingError("Invalid subscription response");
            return;
        }

        qDebug() << "Subscription successful:" << doc.toJson();
    });
}

void UPSClient::onTrackingEvent(QNetworkReply* reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "Tracking Event Error:" << reply->errorString();
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    if (!doc.isObject()) {
        qDebug() << "Invalid tracking event format";
        return;
    }

    handleTrackingEvent(doc.object());
}

void UPSClient::handleTrackingEvent(const QJsonObject& event)
{
    // Parse the tracking event according to UPS API format
    QJsonObject result;
    result["trackingNumber"] = event["trackingNumber"].toString();
    result["status"] = event["activityStatus"].toObject()["description"].toString();
    
    QJsonArray events;
    events.append(QJsonObject{
        {"timestamp", event["localActivityDate"].toString() + " " + event["localActivityTime"].toString()},
        {"description", event["activityStatus"].toObject()["description"].toString()},
        {"location", event["activityLocation"].toObject()["city"].toString() + ", " + 
                   event["activityLocation"].toObject()["stateProvince"].toString()}
    });
    
    result["events"] = events;
    
    if (event.contains("scheduledDeliveryDate")) {
        result["estimatedDelivery"] = event["scheduledDeliveryDate"].toString();
    }
    
    emit trackingEventReceived(result);
}

QString UPSClient::getAuthToken()
{
    QUrl url("https://onlinetools.ups.com/security/v1/oauth/token");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    // Create proper Basic Auth header matching curl
    QString auth = QString("%1:%2").arg(clientId).arg(clientSecret);
    QString authHeader = "Basic " + auth.toUtf8().toBase64();
    request.setRawHeader("Authorization", authHeader.toUtf8());
    
    // Remove x-merchant-id header as it's not in the working curl request
    // request.setRawHeader("x-merchant-id", clientId.toUtf8());

    // Create form data as plain string to match curl
    QString postData = "grant_type=client_credentials";

    // Add debug output
    qDebug() << "Requesting UPS auth token with client ID:" << clientId;
    qDebug() << "Auth header:" << authHeader;
    qDebug() << "Post data:" << postData;
    qDebug() << "UPS Auth Request URL:" << url;
    qDebug() << "UPS Auth Request Data:" << postData;

    // Set content type header to match curl
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    
    // Remove other headers that weren't in the working curl request
    // request.setRawHeader("Accept", "application/json");
    // request.setRawHeader("Content-Length", QByteArray::number(postData.size()));
    
    // Send the request with the raw post data
    QNetworkReply* reply = manager->post(request, postData.toUtf8());
    
    // Wait for reply with timeout
    QEventLoop loop;
    QTimer::singleShot(10000, &loop, &QEventLoop::quit);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    // Check for network errors
    if (reply->error() != QNetworkReply::NoError) {
        QString headers;
        for (const auto& pair : reply->rawHeaderPairs()) {
            headers += QString("%1: %2\n").arg(QString(pair.first)).arg(QString(pair.second));
        }
        QString errorDetails = QString("Network Error: %1\nURL: %2\nHeaders:\n%3")
            .arg(reply->errorString())
            .arg(url.toString())
            .arg(headers);
        qDebug() << errorDetails;
        emit trackingError(errorDetails);
        return QString();
    }

    // Verify SSL
    if (!reply->sslConfiguration().isNull()) {
        QSslCertificate cert = reply->sslConfiguration().peerCertificate();
        if (cert.isNull()) {
            qDebug() << "SSL Certificate verification failed";
            emit trackingError("SSL Certificate verification failed");
            return QString();
        }
    }
    
    // Read response data
    QByteArray responseData = reply->readAll();
    
    // Log raw response including non-printable characters
    qDebug() << "Raw response bytes:" << responseData.toHex();
    qDebug() << "Raw response string:" << responseData;
    
    // Check for SSL errors by connecting to the sslErrors signal
    QList<QSslError> sslErrors;
    connect(reply, &QNetworkReply::sslErrors, this, [this, &sslErrors](const QList<QSslError> &errors) {
        sslErrors = errors;
        if (!sslErrors.isEmpty()) {
            qDebug() << "SSL Errors:";
            for (const QSslError& error : sslErrors) {
                qDebug() << " -" << error.errorString();
            }
            emit trackingError("SSL Error: " + sslErrors.first().errorString());
        }
    });

    qDebug() << "Response Headers:";
    for (const QByteArray& header : reply->rawHeaderList()) {
        qDebug() << " -" << header << ":" << reply->rawHeader(header);
    }
    
    // Check response status code
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    qDebug() << "Status Code:" << statusCode;
    
    if (responseData.isEmpty()) {
        QString headers;
        for (const auto& pair : reply->rawHeaderPairs()) {
            headers += QString("%1: %2\n").arg(QString(pair.first)).arg(QString(pair.second));
        }
        QString errorDetails = QString("Empty response received\nStatus: %1\nHeaders:\n%2")
            .arg(statusCode)
            .arg(headers);
        qDebug() << errorDetails;
        
        if (statusCode == 200) {
            emit trackingError("Received empty response with 200 status - check API credentials and permissions");
        } else {
            emit trackingError(QString("Empty response from UPS API\nStatus: %1").arg(statusCode));
        }
        return QString();
    }

    // Check if response is HTML (could indicate an error page)
    if (responseData.startsWith("<!DOCTYPE") || responseData.startsWith("<html")) {
        QString errorDetails = QString("Received HTML response instead of JSON\nResponse: %1")
            .arg(QString(responseData.left(500))); // Show first 500 chars
        qDebug() << errorDetails;
        emit trackingError("UPS API returned HTML error page - check API endpoint URL");
        return QString();
    }
    
    if (statusCode != 200) {
        qDebug() << "UPS Auth Error - Status Code:" << statusCode;
        qDebug() << "Response:" << responseData;
        
        // Try to parse error details
        QJsonParseError parseError;
        QJsonDocument errorDoc = QJsonDocument::fromJson(responseData, &parseError);
        if (parseError.error == QJsonParseError::NoError && errorDoc.isObject()) {
            QJsonObject errorObj = errorDoc.object();
            QString errorMessage;
            
            // Check for different error formats
            if (errorObj.contains("response")) {
                errorMessage = errorObj["response"].toObject()["errors"].toArray()[0].toObject()["message"].toString();
            } else if (errorObj.contains("error")) {
                errorMessage = errorObj["error"].toString();
                if (errorObj.contains("error_description")) {
                    errorMessage += ": " + errorObj["error_description"].toString();
                }
            } else {
                errorMessage = "Unknown error format";
            }
            
            emit trackingError(QString("UPS Auth Error (%1): %2").arg(statusCode).arg(errorMessage));
        } else {
            // Check for HTML response which might indicate a different error
            if (responseData.contains("<html") || responseData.contains("<!DOCTYPE")) {
                emit trackingError("UPS API returned HTML error page - check API endpoint URL");
            } else {
                emit trackingError(QString("UPS Auth Error (%1): %2").arg(statusCode).arg(reply->errorString()));
            }
        }
        return QString();
    }

    qDebug() << "Raw UPS auth response:" << responseData;
    
    // Parse the response
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        qDebug() << "JSON parse error:" << parseError.errorString();
        emit trackingError("Failed to parse UPS auth response");
        return QString();
    }
    
    if (!doc.isObject()) {
        qDebug() << "Invalid JSON response format";
        emit trackingError("Invalid UPS auth response format");
        return QString();
    }
    
    QJsonObject obj = doc.object();
    if (!obj.contains("access_token")) {
        qDebug() << "No access token in response:" << obj;
        emit trackingError("No access token in UPS response");
        return QString();
    }

    QString token = obj["access_token"].toString();
    qDebug() << "Successfully retrieved UPS token";
    qDebug() << "Token details:";
    qDebug() << "  - Type:" << obj["token_type"].toString();
    qDebug() << "  - Expires in:" << obj["expires_in"].toString() << "seconds";
    qDebug() << "  - Scope:" << (obj.contains("scope") ? obj["scope"].toString() : "none");
    return token;
}

bool UPSClient::verifyCredentials() {
    QString token = getAuthToken();
    if (token.isEmpty()) {
        return false;
    }
    
    // Make a test tracking request
    QUrl url("https://onlinetools.ups.com/api/track/v1/details/1Z9999999999999999");
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QNetworkReply* reply = manager->get(request);
    
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    
    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "Credential verification failed:" << reply->errorString();
        return false;
    }
    
    return true;
}

void UPSClient::onRequestFinished(QNetworkReply* reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        QString errorMessage = reply->errorString();
        QByteArray response = reply->readAll();
        
        // Try to parse error response
        QJsonParseError parseError;
        QJsonDocument errorDoc = QJsonDocument::fromJson(response, &parseError);
        if (parseError.error == QJsonParseError::NoError && errorDoc.isObject()) {
            QJsonObject errorObj = errorDoc.object();
            if (errorObj.contains("response")) {
                errorMessage = errorObj["response"].toObject()["errors"].toArray()[0].toObject()["message"].toString();
            }
        }
        
        qDebug() << "UPS API Error:" << errorMessage;
        qDebug() << "Response:" << response;
        emit trackingError(errorMessage);
        return;
    }

    QByteArray responseData = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(responseData);
    if (!doc.isObject()) {
        emit trackingError("Invalid response format");
        return;
    }

    QJsonObject result;
    QJsonObject trackResponse = doc.object()["trackResponse"].toObject();
    if (!trackResponse.contains("shipment")) {
        emit trackingError("Invalid tracking response format");
        return;
    }

    QJsonObject shipment = trackResponse["shipment"].toArray().first().toObject();
    
    result["trackingNumber"] = shipment["trackingNumber"].toString();
    result["status"] = shipment["currentStatus"].toObject()["description"].toString();
    
    QJsonArray events;
    for (const QJsonValue& activity : shipment["activity"].toArray()) {
        QJsonObject event = activity.toObject();
        events.append(QJsonObject{
            {"timestamp", event["date"].toString() + " " + event["time"].toString()},
            {"description", event["status"].toObject()["description"].toString()},
            {"location", event["location"].toObject()["address"].toObject()["city"].toString() + ", " + 
                       event["location"].toObject()["address"].toObject()["stateProvince"].toString()}
        });
    }
    result["events"] = events;
    
    if (shipment.contains("deliveryDate")) {
        QJsonObject delivery = shipment["deliveryDate"].toObject();
        result["estimatedDelivery"] = delivery["date"].toString();
    }
    
    emit trackingInfoReceived(result);
}
