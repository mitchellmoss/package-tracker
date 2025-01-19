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
    authManager = new QNetworkAccessManager(this);  // Add this line
    connect(manager, &QNetworkAccessManager::finished, this, &UPSClient::onRequestFinished);
}

void UPSClient::trackPackage(const QString& trackingNumber)
{
    QString token = getAuthToken();
    if (token.isEmpty()) {
        emit trackingError("Failed to get authentication token");
        return;
    }

    QUrl url("https://wwwcie.ups.com/api/track/v1/details/" + trackingNumber);
    QUrlQuery query;
    query.addQueryItem("locale", "en_US");
    query.addQueryItem("returnSignature", "false");
    query.addQueryItem("returnMilestones", "false");
    query.addQueryItem("returnPOD", "false");
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QByteArray("Bearer ") + token.toUtf8());
    request.setRawHeader("transId", QDateTime::currentDateTime().toString("TRACKyyyyMMddhhmmss").toUtf8());
    request.setRawHeader("transactionSrc", "testing");
    
    qDebug() << "Tracking package:" << trackingNumber;
    qDebug() << "Request URL:" << url.toString();
    qDebug() << "Authorization:" << "Bearer " + token.toUtf8();
    qDebug() << "Request URL:" << url.toString();

    QNetworkReply* reply = manager->get(request);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            QString error = reply->errorString();
            QByteArray response = reply->readAll();
            qDebug() << "Tracking Error:" << error;
            qDebug() << "Response:" << response;
            
            // Try to parse error details
            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(response, &parseError);
            if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
                QJsonObject obj = doc.object();
                if (obj.contains("response") && obj["response"].toObject().contains("errors")) {
                    QJsonArray errors = obj["response"].toObject()["errors"].toArray();
                    if (!errors.isEmpty()) {
                        error = errors[0].toObject()["message"].toString();
                    }
                }
            }
            
            emit trackingError(error);
            return;
        }

        QByteArray response = reply->readAll();
        qDebug() << "Tracking Response:" << response;
        
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(response, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
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
    QUrl url("https://wwwcie.ups.com/security/v1/oauth/token");
    QNetworkRequest request(url);
    
    // Set exact content type header
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    // Create properly encoded Basic Auth header
    QString credentials = clientId + ":" + clientSecret;
    QByteArray base64Credentials = credentials.toUtf8().toBase64();
    request.setRawHeader("Authorization", "Basic " + base64Credentials);

    // Create properly formatted form data
    QByteArray postData;
    postData.append("grant_type=client_credentials");
    postData.append("&scope=trck");

    qDebug() << "Requesting UPS auth token";
    qDebug() << "Client ID:" << clientId;
    qDebug() << "Auth header:" << "Basic " + base64Credentials;
    qDebug() << "Post data:" << postData;

    // Send the request
    QNetworkReply* reply = authManager->post(request, postData);
    
    // Wait for reply with timeout
    QEventLoop loop;
    QTimer::singleShot(10000, &loop, &QEventLoop::quit);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    // Check for network errors
    if (reply->error() != QNetworkReply::NoError) {
        QString error = reply->errorString();
        QByteArray response = reply->readAll();
        qDebug() << "Auth Error:" << error;
        qDebug() << "Status Code:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        qDebug() << "Response Headers:" << reply->rawHeaderPairs();
        qDebug() << "Response Body:" << response;
        emit trackingError("Authentication failed: " + error);
        return QString();
    }

    // Read response data
    QByteArray responseData = reply->readAll();
    qDebug() << "Auth Response Status:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    qDebug() << "Auth Response Headers:" << reply->rawHeaderPairs();
    qDebug() << "Auth Response Body:" << responseData;

    if (responseData.isEmpty()) {
        qDebug() << "Empty response received from UPS auth endpoint";
        emit trackingError("Empty response received from UPS authentication");
        return QString();
    }

    // Parse the response
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);
    
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        qDebug() << "Failed to parse UPS auth response:" << parseError.errorString();
        emit trackingError("Failed to parse UPS auth response");
        return QString();
    }
    
    QJsonObject obj = doc.object();
    if (!obj.contains("access_token")) {
        // Check for error details
        if (obj.contains("fault")) {
            QJsonObject fault = obj["fault"].toObject();
            QString error = fault["faultstring"].toString();
            if (fault.contains("detail")) {
                error += ": " + fault["detail"].toObject()["errorcode"].toString();
            }
            emit trackingError(error);
        } else {
            emit trackingError("No access token in UPS response");
        }
        return QString();
    }

    QString token = obj["access_token"].toString();
    int expiresIn = obj["expires_in"].toInt();
    qDebug() << "Successfully retrieved UPS token, expires in:" << expiresIn << "seconds";
    qDebug() << "Successfully retrieved UPS token";
    // ...

    reply->deleteLater();  // Add this line before returning
    return token;
}


void UPSClient::onRequestFinished(QNetworkReply* reply)
{
    QByteArray responseData = reply->readAll();
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    
    qDebug() << "Tracking response status:" << statusCode;
    qDebug() << "Response headers:" << reply->rawHeaderPairs();
    qDebug() << "Response body:" << responseData;

    if (reply->error() != QNetworkReply::NoError || statusCode != 200) {
        QString errorMessage = reply->errorString();
        
        // Try to parse error response
        QJsonParseError parseError;
        QJsonDocument errorDoc = QJsonDocument::fromJson(responseData, &parseError);
        if (parseError.error == QJsonParseError::NoError && errorDoc.isObject()) {
            QJsonObject errorObj = errorDoc.object();
            if (errorObj.contains("response")) {
                QJsonArray errors = errorObj["response"].toObject()["errors"].toArray();
                if (!errors.isEmpty()) {
                    errorMessage = errors[0].toObject()["message"].toString();
                }
            } else if (errorObj.contains("fault")) {
                QJsonObject fault = errorObj["fault"].toObject();
                errorMessage = fault["faultstring"].toString();
                if (fault.contains("detail")) {
                    errorMessage += ": " + fault["detail"].toObject()["errorcode"].toString();
                }
            }
        }
        
        qDebug() << "UPS API Error:" << errorMessage;
        emit trackingError(errorMessage);
        return;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        emit trackingError("Invalid response format");
        return;
    }

    QJsonObject result;
    QJsonObject response = doc.object();
    
    if (!response.contains("trackResponse")) {
        emit trackingError("Invalid tracking response format");
        return;
    }

    QJsonObject trackResponse = response["trackResponse"].toObject();
    if (!trackResponse.contains("shipment")) {
        emit trackingError("No shipment data in response");
        return;
    }

    QJsonArray shipments = trackResponse["shipment"].toArray();
    if (shipments.isEmpty()) {
        emit trackingError("No shipment data available");
        return;
    }

    QJsonObject shipment = shipments.first().toObject();
    
    result["trackingNumber"] = shipment["trackingNumber"].toString();
    result["status"] = shipment["currentStatus"].toObject()["description"].toString();
    
    QJsonArray events;
    if (shipment.contains("activity")) {
        for (const QJsonValue& activity : shipment["activity"].toArray()) {
            QJsonObject event = activity.toObject();
            events.append(QJsonObject{
                {"timestamp", event["date"].toString() + " " + event["time"].toString()},
                {"description", event["status"].toObject()["description"].toString()},
                {"location", event["location"].toObject()["address"].toObject()["city"].toString() + ", " + 
                           event["location"].toObject()["address"].toObject()["stateProvince"].toString()}
            });
        }
    }
    result["events"] = events;
    
    if (shipment.contains("deliveryDate")) {
        QJsonObject delivery = shipment["deliveryDate"].toObject();
        result["estimatedDelivery"] = delivery["date"].toString();
    }
    
    emit trackingInfoReceived(result);
}
