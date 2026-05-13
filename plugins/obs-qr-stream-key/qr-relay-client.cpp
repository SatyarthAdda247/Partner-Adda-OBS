#include "qr-relay-client.hpp"
#include <obs-module.h>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QUuid>
#include <thread>

// Suppress OpenSSL deprecation warnings
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4996)
#endif

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"

#ifdef _MSC_VER
#pragma warning(pop)
#endif

static constexpr const char *kRelayHost = "obs-backend-chi.vercel.app";
static constexpr int kRelayPort = 443;
static constexpr const char *kPollPathPrefix = "/api/session?token=";
static constexpr const char *kAuthToken = "fpoa43edty5";

QRRelayClient::QRRelayClient(QObject *parent) : QObject(parent)
{
	pollTimer_ = new QTimer(this);
	pollTimer_->setInterval(2000);
	connect(pollTimer_, &QTimer::timeout, this, &QRRelayClient::doPoll);
}

QRRelayClient::~QRRelayClient()
{
	stopPolling();
}

QString QRRelayClient::generateQR()
{
	// Generate unique token
	QString token = QUuid::createUuid().toString(QUuid::WithoutBraces);
	QString facultyId = "150"; // Default faculty ID
	
	blog(LOG_INFO, "[obs-qr-stream-key] Registering YouTube Live session with token: %s",
	     token.toUtf8().constData());
	
	QString pairUrl = registerSession(token, facultyId);
	
	if (!pairUrl.isEmpty()) {
		activeToken_ = token;
		blog(LOG_INFO, "[obs-qr-stream-key] YouTube Live QR generated: %s",
		     pairUrl.toUtf8().constData());
		return pairUrl;
	}
	
	return {};
}

QString QRRelayClient::registerSession(const QString &token, const QString &facultyId)
{
	httplib::SSLClient cli(kRelayHost, kRelayPort);
	cli.set_connection_timeout(10);
	cli.set_read_timeout(10);
	cli.enable_server_certificate_verification(false);
	
	QJsonObject body;
	body["token"] = token;
	body["facultyId"] = facultyId;
	
	const std::string bodyStr = QJsonDocument(body).toJson(QJsonDocument::Compact).toStdString();
	
	httplib::Headers headers = {
		{"x-auth-token", kAuthToken},
		{"Content-Type", "application/json"},
		{"accept", "application/json"}
	};
	
	blog(LOG_INFO, "[obs-qr-stream-key] Registering session at %s", kRelayHost);
	
	auto res = cli.Post("/api/session", headers, bodyStr, "application/json");
	
	QString pairUrl;
	if (res && res->status == 200) {
		blog(LOG_INFO, "[obs-qr-stream-key] Session registered successfully");
		const QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(res->body));
		if (doc.isObject()) {
			pairUrl = doc.object()["data"].toObject()["pairUrl"].toString();
		}
	} else {
		blog(LOG_WARNING, "[obs-qr-stream-key] Relay registration failed (status=%d) - using fallback URL",
		     res ? res->status : -1);
	}
	
	if (pairUrl.isEmpty()) {
		pairUrl = QString("https://%1/api/pair?token=%2").arg(kRelayHost).arg(token);
	}
	
	return pairUrl;
}

void QRRelayClient::startPolling()
{
	if (activeToken_.isEmpty()) return;
	polling_.store(true);
	pollTimer_->start();
	blog(LOG_INFO, "[obs-qr-stream-key] Started polling for YouTube Live stream key");
}

void QRRelayClient::stopPolling()
{
	polling_.store(false);
	if (pollTimer_) pollTimer_->stop();
}

void QRRelayClient::doPoll()
{
	if (!polling_.load() || activeToken_.isEmpty()) return;
	
	const std::string tokenStr = activeToken_.toStdString();
	const std::string path = std::string(kPollPathPrefix) + tokenStr;
	
	// Run HTTP request in background thread to avoid blocking UI
	std::thread([this, path]() {
		if (!polling_.load()) return;
		
		httplib::SSLClient cli(kRelayHost, kRelayPort);
		cli.set_connection_timeout(3);
		cli.set_read_timeout(3);
		cli.enable_server_certificate_verification(false);
		
		httplib::Headers headers = {{"x-auth-token", kAuthToken}};
		
		auto res = cli.Get(path.c_str(), headers);
		
		if (!polling_.load()) return;
		
		if (!res || res->status != 200) return;
		
		const QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(res->body));
		if (!doc.isObject()) return;
		
		const QJsonObject root = doc.object();
		if (!root["success"].toBool()) return;
		
		const QJsonValue dataVal = root["data"];
		if (dataVal.isNull() || !dataVal.isObject()) return;
		
		const QString streamKey = dataVal.toObject()["streamKey"].toString();
		if (streamKey.isEmpty()) return;
		
		blog(LOG_INFO, "[obs-qr-stream-key] YouTube Live stream key received!");
		
		const QString classId = dataVal.toObject()["classId"].toString();
		const QString sceneCollection = dataVal.toObject()["sceneCollection"].toString();
		
		// Emit signal on main thread using QMetaObject::invokeMethod
		QMetaObject::invokeMethod(
			this,
			[this, streamKey, sceneCollection, classId]() {
				stopPolling();
				emit pairingReceived(streamKey, sceneCollection, classId);
			},
			Qt::QueuedConnection);
	}).detach();
}
