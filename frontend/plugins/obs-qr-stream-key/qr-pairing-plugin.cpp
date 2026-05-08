#include "qr-pairing-plugin.hpp"

#include <obs-module.h>
#include <obs-frontend-api.h>

#include <QDesktopServices>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMainWindow>
#include <QTimer>
#include <QUuid>
#include <QUrl>
#include <chrono>
#include <thread>

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <httplib.h>

#include "adda-login-client.hpp"
#include "network-util.hpp"
#include "qr-choice-dialog.hpp"
#include "qr-pairing-dialog.hpp"
#include "qr-pairing-server.hpp"
#include "qr-relay-client.hpp"

static constexpr const char *kFacultyId = "150";

QRPairingPlugin *QRPairingPlugin::instance()
{
	static QRPairingPlugin inst;
	return &inst;
}

// ── Entry point: show choice screen ───────────────────────────────────────────
void QRPairingPlugin::init()
{
	QTimer::singleShot(500, this, &QRPairingPlugin::showChoiceDialog);
}

void QRPairingPlugin::showChoiceDialog()
{
	if (choiceDialog_) {
		choiceDialog_->close();
		choiceDialog_->deleteLater();
		choiceDialog_ = nullptr;
	}

	QMainWindow *mainWin = reinterpret_cast<QMainWindow *>(
		obs_frontend_get_main_window());

	choiceDialog_ = new QRChoiceDialog(mainWin);

	connect(choiceDialog_, &QDialog::accepted, this, [this]() {
		const auto choice = choiceDialog_->choice();
		choiceDialog_->deleteLater();
		choiceDialog_ = nullptr;
		if (choice == QRChoiceDialog::Choice::Admin)
			onAdminChosen();
		else if (choice == QRChoiceDialog::Choice::YouTube)
			onYouTubeChosen();
	});

	connect(choiceDialog_, &QDialog::rejected, this, [this]() {
		choiceDialog_->deleteLater();
		choiceDialog_ = nullptr;
	});

	choiceDialog_->show();
	choiceDialog_->raise();
	choiceDialog_->activateWindow();
}

// ── Admin Login ────────────────────────────────────────────────────────────────
// Mirrors the original init() Admin QR flow exactly.
void QRPairingPlugin::onAdminChosen()
{
	activeToken_ = QUuid::createUuid().toString(QUuid::WithoutBraces);

	// ── 1. Adda247 login client ────────────────────────────────────────
	addaClient_ = std::make_unique<AddaLoginClient>();
	connect(addaClient_.get(), &AddaLoginClient::loginConfirmed,
		this, &QRPairingPlugin::onAddaLoginConfirmed);

	// ── 2. Show dialog immediately (QR loads async) ────────────────────
	dialog_ = new QRPairingDialog(QRPairingDialog::Mode::Admin, {}, nullptr);
	connect(dialog_, &QDialog::rejected,
		this, &QRPairingPlugin::onDialogRejected);
	connect(dialog_, &QRPairingDialog::backRequested, this, [this]() {
		shutdownNetworking();
		if (dialog_) { dialog_->deleteLater(); dialog_ = nullptr; }
		showChoiceDialog();
	});

	QTimer::singleShot(300, dialog_, [this]() {
		if (dialog_) {
			dialog_->show();
			dialog_->raise();
			dialog_->activateWindow();
		}
	});

	// ── 3. Generate Adda QR asynchronously ────────────────────────────
	std::thread([this]() {
		const QString addaQrData = addaClient_->generateQR();
		if (!addaQrData.isEmpty()) {
			QMetaObject::invokeMethod(this, [this, addaQrData]() {
				if (dialog_) {
					dialog_->setQRData(addaQrData);
					blog(LOG_INFO,
					     "[obs-qr-stream-key] Admin QR set in dialog");
					addaClient_->startPolling();
				}
			}, Qt::QueuedConnection);
		} else {
			QMetaObject::invokeMethod(this, [this]() {
				if (dialog_)
					dialog_->onPairingError(
						"Failed to load QR. Check connection.");
			}, Qt::QueuedConnection);
		}
	}).detach();

	// ── 4. 5-min timeout ──────────────────────────────────────────────
	startTimeout();
}

// ── YouTube Live ───────────────────────────────────────────────────────────────
void QRPairingPlugin::onYouTubeChosen()
{
	activeToken_ = QUuid::createUuid().toString(QUuid::WithoutBraces);

	// Relay client — app scans relay URL and POSTs stream key to relay backend
	relayClient_ = std::make_unique<QRRelayClient>();
	connect(relayClient_.get(), &QRRelayClient::pairingReceived,
		this, &QRPairingPlugin::onPairingReceived);

	std::thread([this]() {
		// Register relay session — returns the URL the mobile app scans
		const QString relayUrl =
			relayClient_->registerSession(activeToken_, kFacultyId);

		// Also start local server as secondary receiver (same WiFi fallback)
		const QString ip    = NetworkUtil::localIPv4Address();
		const uint16_t port = (!ip.isEmpty()) ? NetworkUtil::findAvailablePort() : 0;

		if (port != 0) {
			QMetaObject::invokeMethod(this, [this, port]() {
				server_ = std::make_unique<QRPairingServer>();
				server_->setToken(activeToken_);
				if (server_->start(port)) {
					connect(server_.get(),
						&QRPairingServer::pairingReceived,
						this, &QRPairingPlugin::onPairingReceived);
					blog(LOG_INFO,
					     "[obs-qr-stream-key] Local server also ready on port %u", port);
				} else {
					server_.reset();
				}
			}, Qt::BlockingQueuedConnection);
		}

		// QR encodes relay URL (what app scans); fallback to local if relay failed
		const QString qrUrl = relayUrl.isEmpty()
			? QString("http://%1:%2/pair?token=%3").arg(ip).arg(port).arg(activeToken_)
			: relayUrl;

		blog(LOG_INFO, "[obs-qr-stream-key] YouTube QR URL: %s",
		     qrUrl.toUtf8().constData());

		QMetaObject::invokeMethod(this, [this, qrUrl]() {
			relayClient_->startPolling(activeToken_);

			dialog_ = new QRPairingDialog(
				QRPairingDialog::Mode::YouTube, qrUrl, nullptr);

			connect(dialog_, &QDialog::rejected,
				this, &QRPairingPlugin::onDialogRejected);
			connect(dialog_, &QRPairingDialog::backRequested, this, [this]() {
				shutdownNetworking();
				if (dialog_) { dialog_->deleteLater(); dialog_ = nullptr; }
				showChoiceDialog();
			});

			QTimer::singleShot(300, dialog_, [this]() {
				if (dialog_) {
					dialog_->show(); dialog_->raise(); dialog_->activateWindow();
				}
			});
		}, Qt::QueuedConnection);
	}).detach();

	startTimeout();
}

// ── Helpers ────────────────────────────────────────────────────────────────────
void QRPairingPlugin::startTimeout()
{
	if (timeoutTimer_) {
		timeoutTimer_->stop();
		timeoutTimer_->deleteLater();
	}
	timeoutTimer_ = new QTimer(this);
	timeoutTimer_->setSingleShot(true);
	connect(timeoutTimer_, &QTimer::timeout, this, [this]() {
		blog(LOG_INFO, "[obs-qr-stream-key] Pairing timed out");
		shutdownNetworking();
	});
	timeoutTimer_->start(300 * 1000);
}

void QRPairingPlugin::shutdownNetworking()
{
	if (server_) {
		server_->invalidateToken();
		server_->stop();
		server_.reset();
	}
	if (relayClient_) {
		relayClient_->invalidateSession(activeToken_);
		relayClient_.reset();
	}
	if (addaClient_) {
		addaClient_->stopPolling();
		addaClient_.reset();
	}
}

void QRPairingPlugin::shutdown()
{
	if (timeoutTimer_) timeoutTimer_->stop();
	shutdownNetworking();
	if (choiceDialog_) { choiceDialog_->close(); choiceDialog_ = nullptr; }
	if (dialog_) { dialog_->close(); dialog_ = nullptr; }
}

void QRPairingPlugin::onDialogRejected()
{
	shutdownNetworking();
	if (dialog_) { dialog_->deleteLater(); dialog_ = nullptr; }
}

// ── Admin login confirmed ──────────────────────────────────────────────────────
void QRPairingPlugin::onAddaLoginConfirmed(const QJsonObject &authData)
{
	const QString email        = authData["email"].toString();
	const QString token        = authData["token"].toString();
	const QString facultyToken = authData["facultyToken"].toString();
	const QString lcsdoubtToken= authData["lcsdoubtToken"].toString();
	const QString name         = authData["name"].toString();
	const int     facultyId    = authData["faculty_id"].toInt();

	blog(LOG_INFO, "[obs-qr-stream-key] Adda247 login confirmed: %s (faculty_id=%d)",
	     email.toUtf8().constData(), facultyId);

	if (dialog_)
		dialog_->onAdminLoginConfirmed(email);

	if (token.isEmpty()) {
		// No token — just open admin panel directly
		QDesktopServices::openUrl(
			QUrl("https://admin.adda247.com/dashboard/teacher?page=1"));
		return;
	}

	// Spin up a one-shot local HTTP server that injects ALL auth tokens
	// into localStorage for admin.adda247.com, then redirects to the
	// teacher dashboard.
	auto srv = std::make_shared<httplib::Server>();

	// Capture all tokens needed by the admin panel
	const std::string tokenStr        = token.toStdString();
	const std::string facultyTokenStr = facultyToken.toStdString();
	const std::string lcsdoubtStr     = lcsdoubtToken.toStdString();
	const std::string nameStr         = name.toStdString();
	const std::string emailStr        = email.toStdString();
	const std::string authDataStr     =
		QJsonDocument(authData).toJson(QJsonDocument::Compact).toStdString();

	srv->Get("/auth", [srv, tokenStr, facultyTokenStr, lcsdoubtStr,
			   nameStr, emailStr, authDataStr](
				const httplib::Request &,
				httplib::Response &res) {
		// Store every token the admin panel expects
		const std::string html =
			"<!DOCTYPE html><html><head>"
			"<meta charset='utf-8'>"
			"<title>Adda247 – Signing in...</title>"
			"</head>"
			"<body style='font-family:sans-serif;text-align:center;padding:60px'>"
			"<h2>Signing you in to Adda247 Admin...</h2>"
			"<script>(function(){"
			"try{"
			" localStorage.setItem('token','"              + tokenStr        + "');"
			" localStorage.setItem('adminToken','"         + tokenStr        + "');"
			" localStorage.setItem('facultyToken','"       + facultyTokenStr + "');"
			" localStorage.setItem('lcsdoubtToken','"      + lcsdoubtStr     + "');"
			" localStorage.setItem('userEmail','"          + emailStr        + "');"
			" localStorage.setItem('userName','"           + nameStr         + "');"
			" localStorage.setItem('authData','"           + authDataStr     + "');"
			"}catch(e){}"
			"window.location.replace('https://admin.adda247.com/dashboard/teacher?page=1');"
			"})();</script></body></html>";
		res.set_content(html, "text/html");
		std::thread([srv]() {
			std::this_thread::sleep_for(std::chrono::milliseconds(300));
			srv->stop();
		}).detach();
	});

	const int localPort = srv->bind_to_any_port("127.0.0.1");
	if (localPort <= 0) {
		blog(LOG_ERROR, "[obs-qr-stream-key] Failed to bind auth redirect server");
		QDesktopServices::openUrl(
			QUrl("https://admin.adda247.com/dashboard/teacher?page=1"));
		return;
	}
	std::thread([srv]() { srv->listen_after_bind(); }).detach();

	QDesktopServices::openUrl(
		QUrl(QString("http://127.0.0.1:%1/auth").arg(localPort)));
}

// ── Stream key received ────────────────────────────────────────────────────────
void QRPairingPlugin::onPairingReceived(const QString &streamKey,
					const QString & /*sceneCollection*/,
					const QString &classId)
{
	static std::atomic<bool> handled{false};
	bool expected = false;
	if (!handled.compare_exchange_strong(expected, true))
		return;

	blog(LOG_INFO,
	     "[obs-qr-stream-key] Stream key received, classId=%s",
	     classId.toUtf8().constData());

	applyStreamKey(streamKey);
	if (dialog_) dialog_->onPairingSuccess(classId);
	if (timeoutTimer_) timeoutTimer_->stop();
	if (server_) server_->invalidateToken();
	if (relayClient_) relayClient_->stopPolling();
	if (addaClient_) addaClient_->stopPolling();

	handled.store(false);
}

void QRPairingPlugin::applyStreamKey(const QString &streamKey)
{
	if (streamKey.isEmpty()) return;

	obs_service_t *service = obs_frontend_get_streaming_service();
	if (!service) return;

	obs_data_t *settings = obs_service_get_settings(service);
	if (!settings) return;

	obs_data_set_string(settings, "key", streamKey.toUtf8().constData());
	obs_data_set_string(settings, "server",
			    "rtmp://a.rtmp.youtube.com/live2");

	obs_service_update(service, settings);
	obs_data_release(settings);
	obs_frontend_save_streaming_service();

	blog(LOG_INFO, "[obs-qr-stream-key] Stream key applied — YouTube Primary ingest");

	QTimer::singleShot(600, nullptr, []() {
		QMainWindow *w = reinterpret_cast<QMainWindow *>(
			obs_frontend_get_main_window());
		if (w)
			QMetaObject::invokeMethod(w,
				"on_actionSettings_triggered",
				Qt::QueuedConnection);
	});
}
