#include "adda-login-client.hpp"

#include <obs-module.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <thread>

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <httplib.h>

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
	cli.set_connection_timeout(8);
	cli.set_read_timeout(8);
	cli.enable_server_certificate_verification(false);

	// Match exact headers from the working curl command
	httplib::Headers headers = {
		{"accept",          "application/json, text/plain, */*"},
		{"accept-language", "en-US,en;q=0.9"},
		{"content-length",  "0"},
		{"origin",          "https://admin.adda247.com"},
		{"referer",         "https://admin.adda247.com/login"},
		{"user-agent",      "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) "
		                    "AppleWebKit/537.36 (KHTML, like Gecko) "
		                    "Chrome/147.0.0.0 Safari/537.36"}};

	auto res = cli.Post(kGeneratePath, headers, "", "application/json");

	if (!res || res->status != 200) {
		blog(LOG_WARNING,
		     "[obs-qr-stream-key] Adda QR generate failed (status=%d)",
		     res ? res->status : -1);
		if (res)
			blog(LOG_WARNING,
			     "[obs-qr-stream-key] Response body: %s",
			     res->body.c_str());
		return {};
	}

	const QJsonDocument doc =
		QJsonDocument::fromJson(QByteArray::fromStdString(res->body));
	if (!doc.isObject() || !doc.object()["success"].toBool()) {
		blog(LOG_WARNING,
		     "[obs-qr-stream-key] Adda QR generate: unexpected response: %s",
		     res->body.c_str());
		return {};
	}

	const QJsonObject root = doc.object();
	sessionId_ = root["sessionId"].toString();

	// qrData is a JSON string — encode it directly as the QR payload
	const QString qrData = root["qrData"].toString();

	blog(LOG_INFO,
	     "[obs-qr-stream-key] Adda QR session: %s, qrData length: %lld",
	     sessionId_.toUtf8().constData(),
	     (long long)qrData.length());

	return qrData;
}

void AddaLoginClient::startPolling()
{
	if (sessionId_.isEmpty())
		return;
	pollTimer_->start();
	blog(LOG_INFO, "[obs-qr-stream-key] Adda login polling started for session: %s",
	     sessionId_.toUtf8().constData());
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
	const std::string path =
		"/api/users/qr-login/status/" + sessionIdStr +
		"?t=" + std::to_string(QDateTime::currentMSecsSinceEpoch());

	// Run HTTP call on background thread to avoid blocking Qt event loop
	std::thread([this, path]() {
		httplib::SSLClient cli(kHost, kPort);
		cli.set_connection_timeout(5);
		cli.set_read_timeout(5);
		cli.enable_server_certificate_verification(false);

		// Match exact headers from the working curl command
		httplib::Headers headers = {
			{"accept",          "application/json, text/plain, */*"},
			{"accept-language", "en-US,en;q=0.9"},
			{"referer",         "https://admin.adda247.com/login"},
			{"user-agent",      "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) "
			                    "AppleWebKit/537.36 (KHTML, like Gecko) "
			                    "Chrome/147.0.0.0 Safari/537.36"}};

		auto res = cli.Get(path.c_str(), headers);

		if (!res || res->status != 200)
			return;

		const QJsonDocument doc = QJsonDocument::fromJson(
			QByteArray::fromStdString(res->body));
		if (!doc.isObject())
			return;

		const QJsonObject root = doc.object();
		if (!root["success"].toBool())
			return;

		const QString status = root["status"].toString();
		if (status != "confirmed")
			return; // still pending

		// Login confirmed — pass FULL authData to plugin on main thread
		const QJsonObject authData = root["authData"].toObject();

		blog(LOG_INFO,
		     "[obs-qr-stream-key] Adda login confirmed: %s (faculty_id=%d)",
		     authData["email"].toString().toUtf8().constData(),
		     authData["faculty_id"].toInt());

		QMetaObject::invokeMethod(this, [this, authData]() {
			stopPolling();
			emit loginConfirmed(authData);
		}, Qt::QueuedConnection);
	}).detach();
}
