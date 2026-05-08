#include "qr-pairing-server.hpp"

#include <obs-module.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>

#include <chrono>

QRPairingServer::QRPairingServer(QObject *parent) : QObject(parent) {}

QRPairingServer::~QRPairingServer()
{
	stop();
}

bool QRPairingServer::start(uint16_t port)
{
	if (running_.load()) {
		blog(LOG_WARNING,
		     "[obs-qr-stream-key] Server already running on port %u",
		     port_);
		return false;
	}

	blog(LOG_INFO,
	     "[obs-qr-stream-key] Registering POST /pair handler on port %u",
	     port);

	// Register the /pair POST handler
	svr_.Post("/pair", [this](const httplib::Request &req,
				  httplib::Response &res) {
		blog(LOG_INFO,
		     "[obs-qr-stream-key] POST /pair received — token param: '%s', body length: %zu",
		     req.has_param("token")
			     ? req.get_param_value("token").c_str()
			     : "(missing)",
		     req.body.size());
		handlePairRequest(req, res);
	});

	// Catch-all handler to log any unmatched routes (helps diagnose 404s)
	svr_.set_error_handler([](const httplib::Request &req,
				  httplib::Response &res) {
		blog(LOG_WARNING,
		     "[obs-qr-stream-key] Unmatched request: %s %s (status %d)",
		     req.method.c_str(), req.path.c_str(), res.status);
	});

	// Start the server on a background thread.
	serverThread_ = std::thread([this, port]() {
		blog(LOG_INFO,
		     "[obs-qr-stream-key] HTTP server thread starting on 0.0.0.0:%u",
		     port);
		running_.store(true);
		svr_.listen("0.0.0.0", static_cast<int>(port));
		running_.store(false);
		blog(LOG_INFO,
		     "[obs-qr-stream-key] HTTP server thread exited");
	});

	// Poll until the server reports it is running or we time out.
	constexpr int maxWaitMs = 500;
	constexpr int stepMs = 10;
	for (int waited = 0; waited < maxWaitMs; waited += stepMs) {
		if (svr_.is_running()) {
			port_ = port;
			blog(LOG_INFO,
			     "[obs-qr-stream-key] HTTP server is_running=true on port %u",
			     port_);
			return true;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(stepMs));
	}

	blog(LOG_ERROR,
	     "[obs-qr-stream-key] HTTP server failed to start on port %u (timed out after %dms)",
	     port, maxWaitMs);

	// Server did not come up — clean up
	svr_.stop();
	if (serverThread_.joinable())
		serverThread_.join();
	running_.store(false);
	return false;
}

void QRPairingServer::stop()
{
	if (!running_.load() && !serverThread_.joinable())
		return;

	svr_.stop();
	if (serverThread_.joinable())
		serverThread_.join();

	running_.store(false);
	port_ = 0;
}

uint16_t QRPairingServer::port() const
{
	return port_;
}

void QRPairingServer::setToken(const QString &token)
{
	std::lock_guard<std::mutex> lock(tokenMutex_);
	token_ = token;
}

void QRPairingServer::invalidateToken()
{
	std::lock_guard<std::mutex> lock(tokenMutex_);
	token_.clear();
}

void QRPairingServer::handlePairRequest(const httplib::Request &req,
					httplib::Response &res)
{
	// --- Token validation (Req 3.4, 9.1) ---
	QString activeToken;
	{
		std::lock_guard<std::mutex> lock(tokenMutex_);
		activeToken = token_;
	}

	const std::string requestToken =
		req.has_param("token") ? req.get_param_value("token") : "";

	blog(LOG_INFO,
	     "[obs-qr-stream-key] handlePairRequest: activeToken='%s', requestToken='%s'",
	     activeToken.toUtf8().constData(), requestToken.c_str());

	if (activeToken.isEmpty() ||
	    requestToken != activeToken.toStdString()) {
		blog(LOG_WARNING,
		     "[obs-qr-stream-key] Token mismatch — returning 403 (activeToken empty=%d)",
		     activeToken.isEmpty() ? 1 : 0);
		res.status = 403;
		res.set_content(
			R"({"status":"error","message":"invalid token"})",
			"application/json");
		return;
	}

	blog(LOG_INFO,
	     "[obs-qr-stream-key] Token valid — parsing JSON body (%zu bytes)",
	     req.body.size());

	// --- JSON parsing (Req 4.1, 8.3) ---
	QJsonParseError parseError;
	const QJsonDocument doc = QJsonDocument::fromJson(
		QByteArray::fromStdString(req.body), &parseError);

	if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
		blog(LOG_WARNING,
		     "[obs-qr-stream-key] Invalid JSON in /pair request: %s",
		     parseError.errorString().toUtf8().constData());
		res.status = 400;
		res.set_content(
			R"({"status":"error","message":"invalid JSON"})",
			"application/json");
		return;
	}

	// --- Extract fields (Req 4.1) ---
	const QJsonObject obj = doc.object();
	const QString streamKey = obj.value("streamKey").toString();
	const QString classId = obj.value("classId").toString();
	const QString sceneCollection = obj.value("sceneCollection").toString();

	blog(LOG_INFO,
	     "[obs-qr-stream-key] Pairing data received: streamKey='%s' classId='%s' sceneCollection='%s'",
	     streamKey.isEmpty() ? "(empty)" : "***",
	     classId.toUtf8().constData(),
	     sceneCollection.toUtf8().constData());

	// --- Emit signal on Qt main thread (Req 3.3, 3.5) ---
	QMetaObject::invokeMethod(
		this, "pairingReceived", Qt::QueuedConnection,
		Q_ARG(QString, streamKey), Q_ARG(QString, sceneCollection),
		Q_ARG(QString, classId));

	// --- Success response (Req 3.5) ---
	res.status = 200;
	res.set_content(R"({"status":"ok"})", "application/json");
	blog(LOG_INFO, "[obs-qr-stream-key] /pair responded 200 OK");
}
