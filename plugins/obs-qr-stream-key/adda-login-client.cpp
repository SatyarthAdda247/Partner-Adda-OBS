#include "adda-login-client.hpp"
#include <obs-module.h>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <thread>

// Suppress OpenSSL deprecation warnings in httplib.h
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4996) // 'function': was declared deprecated
#endif

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"

#ifdef _MSC_VER
#pragma warning(pop)
#endif

AddaLoginClient::AddaLoginClient(QObject *parent) : QObject(parent)
{
	pollTimer_ = new QTimer(this);
	pollTimer_->setInterval(2000);
	connect(pollTimer_, &QTimer::timeout, this, &AddaLoginClient::doPoll);
}

AddaLoginClient::~AddaLoginClient()
{
	stopPolling();
}

QString AddaLoginClient::generateQR()
{
	httplib::SSLClient cli(kHost, kPort);
	cli.set_connection_timeout(10);
	cli.set_read_timeout(10);
	cli.enable_server_certificate_verification(false);

	httplib::Headers headers = {
		{"accept", "application/json, text/plain, */*"},
		{"accept-language", "en-US,en;q=0.9"},
		{"content-type", "application/json"},
		{"content-length", "0"},
		{"origin", "https://admin.adda247.com"},
		{"priority", "u=1, i"},
		{"referer", "https://admin.adda247.com/login"},
		{"sec-ch-ua", "\"Google Chrome\";v=\"147\", \"Not.A/Brand\";v=\"8\", \"Chromium\";v=\"147\""},
		{"sec-ch-ua-mobile", "?0"},
		{"sec-ch-ua-platform", "\"Windows\""},
		{"sec-fetch-dest", "empty"},
		{"sec-fetch-mode", "cors"},
		{"sec-fetch-site", "same-origin"},
		{"user-agent",
		 "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/147.0.0.0 Safari/537.36"}};

	blog(LOG_INFO, "[obs-qr-stream-key] Generating QR code from admin.adda247.com...");

	auto res = cli.Post(kGeneratePath, headers, "", "application/json");

	if (!res) {
		blog(LOG_WARNING, "[obs-qr-stream-key] Adda QR generate failed - no response");
		return {};
	}

	blog(LOG_INFO, "[obs-qr-stream-key] QR generate response status: %d", res->status);
	blog(LOG_INFO, "[obs-qr-stream-key] QR generate response body: %s", res->body.c_str());

	if (res->status != 200) {
		blog(LOG_WARNING, "[obs-qr-stream-key] Adda QR generate failed (status=%d)",
		     res->status);
		return {};
	}

	const QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(res->body));
	if (!doc.isObject()) {
		blog(LOG_WARNING, "[obs-qr-stream-key] Invalid JSON response");
		return {};
	}

	const QJsonObject root = doc.object();
	
	if (!root["success"].toBool()) {
		blog(LOG_WARNING, "[obs-qr-stream-key] API returned success=false");
		return {};
	}

	sessionId_ = root["sessionId"].toString();
	
	// Get qrData from API and add redirect URL for mobile app
	QString apiQrData = root["qrData"].toString();
	
	if (apiQrData.isEmpty()) {
		blog(LOG_WARNING, "[obs-qr-stream-key] Empty qrData in response");
		return {};
	}
	
	// Parse the qrData JSON to add redirectUrl
	QJsonDocument qrDoc = QJsonDocument::fromJson(apiQrData.toUtf8());
	if (qrDoc.isObject()) {
		QJsonObject qrObj = qrDoc.object();
		// Add redirect URL so mobile app knows where to go after scanning
		qrObj["redirectUrl"] = "https://admin.adda247.com/dashboard/teacher?page=1";
		
		// Convert back to JSON string
		QString finalQrData = QString::fromUtf8(QJsonDocument(qrObj).toJson(QJsonDocument::Compact));
		
		blog(LOG_INFO, "[obs-qr-stream-key] QR generated successfully, sessionId: %s",
		     sessionId_.toUtf8().constData());
		blog(LOG_INFO, "[obs-qr-stream-key] QR data: %s", finalQrData.toUtf8().constData());
		
		return finalQrData;
	}
	
	// Fallback: use original qrData if parsing fails
	blog(LOG_INFO, "[obs-qr-stream-key] QR generated successfully, sessionId: %s",
	     sessionId_.toUtf8().constData());
	blog(LOG_INFO, "[obs-qr-stream-key] QR data: %s", apiQrData.toUtf8().constData());
	
	return apiQrData;
}

void AddaLoginClient::startPolling()
{
	if (sessionId_.isEmpty())
		return;
	pollTimer_->start();
}

void AddaLoginClient::stopPolling()
{
	if (pollTimer_)
		pollTimer_->stop();
}

void AddaLoginClient::doPoll()
{
	if (sessionId_.isEmpty())
		return;

	const std::string sessionIdStr = sessionId_.toStdString();
	const std::string path = "/api/users/qr-login/status/" + sessionIdStr + "?t=" +
				 std::to_string(QDateTime::currentMSecsSinceEpoch());

	std::thread([this, path]() {
		httplib::SSLClient cli(kHost, kPort);
		cli.set_connection_timeout(5);
		cli.set_read_timeout(5);
		cli.enable_server_certificate_verification(false);

		httplib::Headers headers = {
			{"accept", "application/json, text/plain, */*"},
			{"accept-language", "en-US,en;q=0.9"},
			{"referer", "https://admin.adda247.com/login"},
			{"user-agent",
			 "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/147.0.0.0 Safari/537.36"}};

		auto res = cli.Get(path.c_str(), headers);

		if (!res || res->status != 200)
			return;

		const QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(res->body));
		if (!doc.isObject())
			return;

		const QJsonObject root = doc.object();
		if (!root["success"].toBool())
			return;

		const QString status = root["status"].toString();
		if (status != "confirmed")
			return;

		const QJsonObject authData = root["authData"].toObject();

		QMetaObject::invokeMethod(
			this,
			[this, authData]() {
				stopPolling();
				emit loginConfirmed(authData);
			},
			Qt::QueuedConnection);
	}).detach();
}
