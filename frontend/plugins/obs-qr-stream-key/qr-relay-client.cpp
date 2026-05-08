#include "qr-relay-client.hpp"

#include <obs-module.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>

// Use cpp-httplib for HTTPS — it uses the system's native SSL on macOS
// (SecureTransport via the system libssl), avoiding Qt's missing TLS backend.
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <httplib.h>

static constexpr const char *kRelayHost = "obs-backend-chi.vercel.app";
static constexpr int kRelayPort = 443;
static constexpr const char *kRegisterPath [[maybe_unused]] = "/api/session";
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

QString QRRelayClient::registerSession(const QString &token,
					const QString &facultyId)
{
	httplib::SSLClient cli(kRelayHost, kRelayPort);
	cli.set_connection_timeout(5);
	cli.set_read_timeout(5);
	cli.enable_server_certificate_verification(false); // internal backend

	QJsonObject body;
	body["token"] = token;
	body["facultyId"] = facultyId;
	const std::string bodyStr =
		QJsonDocument(body).toJson(QJsonDocument::Compact).toStdString();

	httplib::Headers headers = {{"x-auth-token", kAuthToken},
				    {"Content-Type", "application/json"}};

	auto res = cli.Post("/api/session", headers, bodyStr, "application/json");

	QString pairUrl;
	if (res && res->status == 200) {
		const QJsonDocument doc = QJsonDocument::fromJson(
			QByteArray::fromStdString(res->body));
		if (doc.isObject()) {
			pairUrl = doc.object()["data"]
					  .toObject()["pairUrl"]
					  .toString();
		}
	} else {
		blog(LOG_WARNING,
		     "[obs-qr-stream-key] Relay registration failed (status=%d) — using fallback URL",
		     res ? res->status : -1);
	}

	// Fallback: build URL ourselves if backend didn't return one
	if (pairUrl.isEmpty()) {
		pairUrl = QString("https://%1/api/pair?token=%2")
				  .arg(kRelayHost)
				  .arg(token);
	}

	blog(LOG_INFO, "[obs-qr-stream-key] Relay session URL: %s",
	     pairUrl.toUtf8().constData());
	return pairUrl;
}

void QRRelayClient::startPolling(const QString &token)
{
	activeToken_ = token;
	polling_.store(true);
	pollTimer_->start();
	blog(LOG_INFO,
	     "[obs-qr-stream-key] Started relay polling for token: %s",
	     token.toUtf8().constData());
}

void QRRelayClient::stopPolling()
{
	polling_.store(false);
	if (pollTimer_)
		pollTimer_->stop();
	activeToken_.clear();
}

void QRRelayClient::invalidateSession(const QString &token)
{
	stopPolling();
	if (token.isEmpty())
		return;

	const std::string tokenStr = token.toStdString();
	std::thread([tokenStr]() {
		httplib::SSLClient cli(kRelayHost, kRelayPort);
		cli.set_connection_timeout(3);
		cli.enable_server_certificate_verification(false);
		httplib::Headers headers = {{"x-auth-token", kAuthToken}};
		const std::string path =
			std::string("/api/session?token=") + tokenStr;
		cli.Delete(path.c_str(), headers);
	}).detach();
}

void QRRelayClient::doPoll()
{
	if (!polling_.load() || activeToken_.isEmpty())
		return;

	const std::string tokenStr = activeToken_.toStdString();
	const std::string path =
		std::string(kPollPathPrefix) + tokenStr;

	httplib::SSLClient cli(kRelayHost, kRelayPort);
	cli.set_connection_timeout(3);
	cli.set_read_timeout(3);
	cli.enable_server_certificate_verification(false);

	httplib::Headers headers = {{"x-auth-token", kAuthToken}};
	auto res = cli.Get(path.c_str(), headers);

	if (!polling_.load())
		return;

	if (!res) {
		blog(LOG_WARNING,
		     "[obs-qr-stream-key] Relay poll failed — no response");
		return;
	}

	if (res->status != 200)
		return;

	const QJsonDocument doc =
		QJsonDocument::fromJson(QByteArray::fromStdString(res->body));
	if (!doc.isObject())
		return;

	const QJsonObject root = doc.object();
	if (!root["success"].toBool())
		return;

	const QJsonValue dataVal = root["data"];
	if (dataVal.isNull() || !dataVal.isObject())
		return; // still pending

	const QString streamKey =
		dataVal.toObject()["streamKey"].toString();
	if (streamKey.isEmpty())
		return; // still pending

	// Got result — stop polling and emit on Qt main thread
	stopPolling();

	const QString classId = dataVal.toObject()["classId"].toString();
	const QString sceneCollection =
		dataVal.toObject()["sceneCollection"].toString();

	blog(LOG_INFO,
	     "[obs-qr-stream-key] Relay pairing received, classId=%s",
	     classId.toUtf8().constData());

	emit pairingReceived(streamKey, sceneCollection, classId);
}
