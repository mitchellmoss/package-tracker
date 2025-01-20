#include "fedexclient.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <QEventLoop>
#include <QTimer>

FedExClient::FedExClient(const QString& apiKey, const QString& apiSecret, QObject *parent)
    : QObject(parent), apiKey(apiKey), apiSecret(apiSecret)
{
    manager = new QNetworkAccessManager(this);
    connect(manager, &QNetworkAccessManager::finished, this, &FedExClient::onRequestFinished);
}

void FedExClient::trackPackage(const QString& trackingNumber)
{
    QString token = getAuthToken();
    if (token.isEmpty()) {
        emit trackingError("Failed to get authentication token");
        return;
    }

    QUrl url("https://apis.fedex.com/track/v1/trackingnumbers");
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");
    
    // Add required headers
    QString transactionId = QUuid::createUuid().toString();
    request.setRawHeader("x-customer-transaction-id", transactionId.toUtf8());
    request.setRawHeader("x-locale", "en_US");
    request.setRawHeader("x-version", "1");
    request.setRawHeader("x-fedex-client-id", apiKey.toUtf8());
    request.setRawHeader("x-fedex-client-secret", apiSecret.toUtf8());

    QJsonObject trackingNumberInfo;
    trackingNumberInfo["trackingNumber"] = trackingNumber;
    trackingNumberInfo["carrierCode"] = "FDXE"; // FedEx Express
    trackingNumberInfo["shipDateBegin"] = QDate::currentDate().addDays(-45).toString("yyyy-MM-dd");
    trackingNumberInfo["shipDateEnd"] = QDate::currentDate().addDays(1).toString("yyyy-MM-dd");

    QJsonObject payload;
    payload["includeDetailedScans"] = true;
    payload["trackingInfo"] = QJsonArray{trackingNumberInfo};
    payload["appDeviceType"] = "DESKTOP.BROWSER.CHROME.WINDOWS";
    payload["appType"] = "WTRK";
    payload["uniqueKey"] = "";

    QByteArray jsonData = QJsonDocument(payload).toJson();
    
    qDebug() << "FedEx Tracking Request URL:" << url.toString();
    qDebug() << "FedEx Tracking Headers:";
    qDebug() << " - Authorization: Bearer" << token;
    qDebug() << " - Transaction ID:" << transactionId;
    qDebug() << "FedEx Tracking Request Payload:" << jsonData;

    QNetworkReply* reply = manager->post(request, jsonData);
    
    // Add error handling for the request itself
    connect(reply, &QNetworkReply::errorOccurred, this, [this, reply](QNetworkReply::NetworkError error) {
        qDebug() << "FedEx Tracking Network Error:" << error;
        qDebug() << "Error String:" << reply->errorString();
    });
}

QString FedExClient::getAuthToken()
{
    QUrl url("https://apis.fedex.com/oauth/token");
    QNetworkRequest request(url);
    
    // Set required headers
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setRawHeader("Accept", "application/json");
    
    // Create form data
    QUrlQuery query;
    query.addQueryItem("grant_type", "client_credentials");
    query.addQueryItem("client_id", apiKey);
    query.addQueryItem("client_secret", apiSecret);
    
    QByteArray postData = query.toString(QUrl::FullyEncoded).toUtf8();

    qDebug() << "FedEx Auth Request URL:" << url.toString();
    qDebug() << "FedEx Auth Request Headers:";
    qDebug() << " - Content-Type:" << request.header(QNetworkRequest::ContentTypeHeader).toString();
    qDebug() << " - Accept: application/json";
    qDebug() << "FedEx Auth Request Data:" << postData;

    // Create a separate manager for auth requests to avoid signal conflicts
    QNetworkAccessManager authManager;
    QNetworkReply* reply = authManager.post(request, postData);
    
    // Wait for reply with timeout
    QEventLoop loop;
    QTimer::singleShot(30000, &loop, &QEventLoop::quit); // Increased timeout to 30 seconds
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    // Check for timeout
    if (reply->isRunning()) {
        reply->abort();
        emit trackingError("FedEx authentication request timed out");
        reply->deleteLater();
        return QString();
    }

    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QByteArray responseData = reply->readAll();
    
    qDebug() << "FedEx Auth Response Status:" << statusCode;
    qDebug() << "FedEx Auth Response Headers:" << reply->rawHeaderPairs();
    qDebug() << "FedEx Auth Response Data:" << responseData;

    if (reply->error() != QNetworkReply::NoError || statusCode != 200) {
        QString errorMsg = "FedEx Auth Error: " + reply->errorString();
        if (!responseData.isEmpty()) {
            QJsonDocument errorDoc = QJsonDocument::fromJson(responseData);
            if (errorDoc.isObject()) {
                QJsonObject errorObj = errorDoc.object();
                if (errorObj.contains("errors")) {
                    QJsonArray errors = errorObj["errors"].toArray();
                    if (!errors.isEmpty()) {
                        QJsonObject error = errors.first().toObject();
                        errorMsg += "\nError Code: " + error["code"].toString();
                        errorMsg += "\nMessage: " + error["message"].toString();
                    }
                }
            }
        }
        qDebug() << errorMsg;
        emit trackingError(errorMsg);
        reply->deleteLater();
        return QString();
    }

    qDebug() << "Raw FedEx auth response:" << responseData;
    
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
    return token;
}

void FedExClient::onRequestFinished(QNetworkReply* reply)
{
    QByteArray responseData = reply->readAll();
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    
    qDebug() << "FedEx Tracking Response Status:" << statusCode;
    qDebug() << "FedEx Tracking Response Headers:" << reply->rawHeaderPairs();
    qDebug() << "FedEx Tracking Response Body:" << responseData;

    if (reply->error() != QNetworkReply::NoError) {
        QString errorMsg = "Network Error: " + reply->errorString();
        qDebug() << "FedEx Tracking Error:" << errorMsg;
        qDebug() << "Response:" << responseData;
        
        // Try to parse error response for more details
        QJsonDocument errorDoc = QJsonDocument::fromJson(responseData);
        if (errorDoc.isObject()) {
            QJsonObject errorObj = errorDoc.object();
            if (errorObj.contains("transactionId")) {
                errorMsg += "\nTransaction ID: " + errorObj["transactionId"].toString();
            }
            if (errorObj.contains("errors")) {
                QJsonArray errors = errorObj["errors"].toArray();
                for (const QJsonValue &error : errors) {
                    QJsonObject errorDetails = error.toObject();
                    errorMsg += "\nCode: " + errorDetails["code"].toString();
                    errorMsg += "\nMessage: " + errorDetails["message"].toString();
                }
            }
        }
        
        emit trackingError(errorMsg);
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(responseData);
    if (!doc.isObject()) {
        emit trackingError("Invalid response format");
        return;
    }

    QJsonObject result;
    QJsonObject response = doc.object();
    
    // Check for alerts first
    if (response.contains("alerts")) {
        QJsonArray alerts = response["alerts"].toArray();
        if (!alerts.isEmpty()) {
            QString errorMsg;
            for (const QJsonValue& alert : alerts) {
                if (alert.isString()) {
                    errorMsg += alert.toString() + "\n";
                } else if (alert.isObject()) {
                    errorMsg += alert.toObject()["message"].toString() + "\n";
                }
            }
            emit trackingError(errorMsg.trimmed());
            return;
        }
    }

    if (!response.contains("output")) {
        emit trackingError("Invalid response format - missing output");
        return;
    }

    QJsonObject output = response["output"].toObject();
    
    // Check if output is empty (no tracking data found)
    if (output.isEmpty()) {
        emit trackingError("No tracking data found for this number");
        return;
    }

    if (!output.contains("completeTrackResults")) {
        emit trackingError("Invalid response format - missing track results");
        return;
    }

    QJsonArray trackResults = output["completeTrackResults"].toArray();
    if (trackResults.isEmpty()) {
        if (output.contains("alerts")) {
            emit trackingError(output["alerts"].toString());
        } else {
            emit trackingError("No tracking results found");
        }
        return;
    }

    QJsonObject trackResult = trackResults.first().toObject();
    if (!trackResult.contains("trackingNumber")) {
        emit trackingError("Invalid response format - missing tracking number");
        return;
    }

    result["trackingNumber"] = trackResult["trackingNumber"].toString();
    
    // Get latest status
    if (trackResult.contains("latestStatusDetail")) {
        QJsonObject status = trackResult["latestStatusDetail"].toObject();
        result["status"] = status["description"].toString();
        
        // Check for delay details
        if (status.contains("delayDetail")) {
            QJsonObject delay = status["delayDetail"].toObject();
            QString delayStatus = delay["status"].toString();
            if (delayStatus == "DELAYED") {
                result["delay"] = QJsonObject{
                    {"type", delay["type"].toString()},
                    {"subType", delay["subType"].toString()}
                };
            }
        }
    }

    QJsonArray events;
    if (trackResult.contains("scanEvents")) {
        for (const QJsonValue& scan : trackResult["scanEvents"].toArray()) {
            QJsonObject event = scan.toObject();
            QString location;
            if (event.contains("scanLocation")) {
                QJsonObject loc = event["scanLocation"].toObject();
                QString city = loc["city"].toString();
                QString state = loc["stateOrProvinceCode"].toString();
                location = city + (state.isEmpty() ? "" : ", " + state);
            }
            
            events.append(QJsonObject{
                {"timestamp", event["date"].toString() + " " + event["time"].toString()},
                {"description", event["eventDescription"].toString()},
                {"location", location}
            });
        }
    }
    result["events"] = events;

    if (trackResult.contains("estimatedDeliveryTimeWindow")) {
        QJsonObject window = trackResult["estimatedDeliveryTimeWindow"].toObject();
        if (window.contains("window")) {
            QJsonObject timeWindow = window["window"].toObject();
            result["estimatedDelivery"] = timeWindow["begins"].toString() + " - " + 
                                        timeWindow["ends"].toString();
        }
    }
    
    emit trackingInfoReceived(result);
}
