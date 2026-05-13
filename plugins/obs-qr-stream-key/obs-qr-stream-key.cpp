#include <obs-module.h>
#include <obs-frontend-api.h>
#include <QMainWindow>
#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QUuid>
#include <QInputDialog>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDesktopServices>
#include <QUrl>
#include <QPointer>
#include <QThread>
#include <QApplication>
#include <QIcon>
#include <QFile>
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

#include "qr-dialog.hpp"
#include "adda-login-client.hpp"
#include "qr-relay-client.hpp"
#include "auth-selection-dialog.hpp"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-qr-stream-key", "en-US")

MODULE_EXPORT const char *obs_module_name(void)
{
	return "obs-qr-stream-key";
}

MODULE_EXPORT const char *obs_module_description(void)
{
	return "Adda Partner - QR authentication and YouTube Class integration for Adda247";
}

static QAction *qrAuthAction = nullptr;
static AddaLoginClient *addaClient = nullptr;
static QRRelayClient *relayClient = nullptr;

// YouTube OLC API constants
static constexpr const char *kOlcHost = "liveclasses.adda247.com";
static constexpr int kOlcPort = 443;
static constexpr const char *kAuthToken = "fpoa43edty5";
static constexpr const char *kJwtToken =
	"eyJhbGciOiJIUzUxMiJ9.eyJzdWIiOiJ0ZWNoX3RlYW0ifQ."
	"GtM-3lGcNcnNCMFvg8cRJbw3DsFNReCHJ3yd2P9kjwUkRLYqneNj8UkwkwCMIZtK"
	"-P-ILtXSDrJbzWgJ1PZFPg";
static constexpr int kDefaultFacultyId = 59;
static constexpr const char *kDefaultEmail = "ayush.chauhan%40adda247.com";

// Forward declarations
static void showAuthSelectionDialog();
static void handlePaidClass();
static void handleYouTubeClass();
static void applyStreamSettings(const QString &streamKey, const QString &server = QString());

static void showAuthSelectionDialog()
{
	QMainWindow *mainWindow = static_cast<QMainWindow *>(obs_frontend_get_main_window());
	
	// Show selection dialog with both options
	AuthSelectionDialog *dialog = new AuthSelectionDialog(mainWindow);
	
	QObject::connect(dialog, &AuthSelectionDialog::adminLoginSelected, []() {
		handlePaidClass();
	});
	
	QObject::connect(dialog, &AuthSelectionDialog::youtubeLiveSelected, []() {
		handleYouTubeClass();
	});
	
	dialog->exec();
	dialog->deleteLater();
}

// ─── Both buttons use the same QR scan flow. YouTube Class is identical to Paid Class.
static void handleYouTubeClass()
{
	// Same flow as Paid Class: QR scan → get key from app → apply → open YouTube Studio
	handlePaidClass();
}

// ─── Helper: Fetch OLC schedule and open matching YouTube join URL ─────────────
static void fetchAndOpenYouTubeJoinUrl(QMainWindow *mainWindow, const QString &appliedStreamKey)
{
	blog(LOG_INFO, "[obs-qr-stream-key] Fetching OLC schedule to find YouTube join URL...");

	std::thread([mainWindow, appliedStreamKey]() {
		httplib::SSLClient cli(kOlcHost, kOlcPort);
		cli.set_connection_timeout(10);
		cli.set_read_timeout(10);
		cli.enable_server_certificate_verification(false);

		std::string path = std::string("/api/v1/olc-schedule/teacherOLCList"
			"?facultyId=") + std::to_string(kDefaultFacultyId) +
			"&type=today&pageSize=10&pageNumber=1&olcType=YouTube"
			"&email=" + kDefaultEmail;

		httplib::Headers headers = {
			{"accept", "application/json"},
			{"content-type", "application/json"},
			{"origin", "https://admin.adda247.com"},
			{"referer", "https://admin.adda247.com/"},
			{"x-auth-token", kAuthToken},
			{"x-jwt-token", kJwtToken},
			{"user-agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
				"AppleWebKit/537.36 (KHTML, like Gecko) Chrome/148.0.0.0 Safari/537.36"}
		};

		auto res = cli.Get(path.c_str(), headers);

		QMetaObject::invokeMethod(mainWindow, [mainWindow, appliedStreamKey, res = std::move(res)]() {
			if (!res || res->status != 200) {
				blog(LOG_WARNING, "[obs-qr-stream-key] OLC API failed, skipping join URL");
				return;
			}

			QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(res->body));
			if (!doc.isObject()) return;

			QJsonObject root = doc.object();
			if (!root["success"].toBool()) return;

			QJsonArray dataList = root["data"].toObject()["dataList"].toArray();

			// Find class matching the applied stream key
			QString joinUrl;
			QString topic;
			QString startTime;
			QString endTime;

			for (const QJsonValue &val : dataList) {
				QJsonObject cls = val.toObject();
				if (cls["youTubeStreamKey"].toString() == appliedStreamKey) {
					joinUrl = cls["youTubeJoinUrl"].toString();
					topic = cls["topic"].toString();
					startTime = cls["startTime"].toString();
					endTime = cls["endTime"].toString();
					break;
				}
			}

			// If no exact match, use the first class with a join URL
			if (joinUrl.isEmpty() && !dataList.isEmpty()) {
				QJsonObject first = dataList[0].toObject();
				joinUrl = first["youTubeJoinUrl"].toString();
				topic = first["topic"].toString();
				startTime = first["startTime"].toString();
				endTime = first["endTime"].toString();
			}

			if (!joinUrl.isEmpty()) {
				blog(LOG_INFO, "[obs-qr-stream-key] Opening YouTube Studio: %s",
				     joinUrl.toUtf8().constData());
				QDesktopServices::openUrl(QUrl(joinUrl));

				QString message = QString(
					"Stream key applied and YouTube Studio is opening!\n\n"
					"Topic: %1\n"
					"Time: %2 - %3\n\n"
					"You are ready to go live!")
					.arg(topic)
					.arg(startTime)
					.arg(endTime);

				QMessageBox::information(mainWindow, "Adda Partner - Ready", message);
			} else {
				blog(LOG_INFO, "[obs-qr-stream-key] No YouTube join URL found in schedule");
				QMessageBox::information(mainWindow, "Adda Partner",
					"Stream key applied!\n\nNo scheduled YouTube class found to open.");
			}
		}, Qt::QueuedConnection);
	}).detach();
}

// ─── PAID CLASS: QR scan for stream key, then open YouTube Studio ─────────────
static void handlePaidClass()
{
	QMainWindow *mainWindow = static_cast<QMainWindow *>(obs_frontend_get_main_window());

	if (!relayClient) {
		relayClient = new QRRelayClient(mainWindow);
	}

	relayClient->stopPolling();

	blog(LOG_INFO, "[obs-qr-stream-key] Generating Paid Class QR code...");
	QString qrData = relayClient->generateQR();

	if (qrData.isEmpty()) {
		QMessageBox::critical(mainWindow, "Adda Partner",
			"Failed to generate QR code. Please check your internet "
			"connection and try again.");
		return;
	}

	QRDialog *qrDialog = new QRDialog("Paid Class - Scan to Stream", qrData, mainWindow);
	QPointer<QRDialog> qrDialogPtr(qrDialog);

	QMetaObject::Connection backConn;
	QMetaObject::Connection pairingConn;

	backConn = QObject::connect(qrDialog, &QRDialog::backRequested, [mainWindow, qrDialogPtr]() {
		if (qrDialogPtr) qrDialogPtr->reject();
		QTimer::singleShot(100, [mainWindow]() { showAuthSelectionDialog(); });
	});

	pairingConn = QObject::connect(
		relayClient, &QRRelayClient::pairingReceived, qrDialog,
		[mainWindow, qrDialogPtr, &pairingConn](const QString &streamKey,
							const QString &sceneCollection,
							const QString &classId) {
			blog(LOG_INFO, "[obs-qr-stream-key] Stream key received via QR scan");
			QObject::disconnect(pairingConn);

			if (!qrDialogPtr) return;

			try {
				applyStreamSettings(streamKey, "rtmp://a.rtmp.youtube.com/live2");
				blog(LOG_INFO, "[obs-qr-stream-key] Stream key applied (length: %d)",
				     streamKey.length());

				if (qrDialogPtr) qrDialogPtr->accept();

				// Fetch OLC schedule and open YouTube Studio
				fetchAndOpenYouTubeJoinUrl(mainWindow, streamKey);

			} catch (const std::exception &e) {
				blog(LOG_ERROR, "[obs-qr-stream-key] Exception: %s", e.what());
				QMessageBox::critical(mainWindow, "Adda Partner",
					QString("Failed to configure stream: %1").arg(e.what()));
			} catch (...) {
				QMessageBox::critical(mainWindow, "Adda Partner",
					"Failed to configure stream: Unknown error");
			}
		},
		Qt::QueuedConnection);

	relayClient->startPolling();
	qrDialog->exec();

	// Cleanup
	relayClient->stopPolling();
	QObject::disconnect(backConn);
	QObject::disconnect(pairingConn);
	qrDialog->deleteLater();
	blog(LOG_INFO, "[obs-qr-stream-key] Paid Class cleanup complete");
}

static void applyStreamSettings(const QString &streamKey, const QString &server)
{
	blog(LOG_INFO, "[obs-qr-stream-key] applyStreamSettings called");

	if (QThread::currentThread() != qApp->thread()) {
		blog(LOG_ERROR, "[obs-qr-stream-key] applyStreamSettings called from wrong thread!");
		return;
	}

	if (streamKey.isEmpty()) {
		blog(LOG_WARNING, "[obs-qr-stream-key] Empty stream key, skipping");
		return;
	}

	// Determine server URL - use provided or default to YouTube RTMP
	const char *serverStr = "rtmp://a.rtmp.youtube.com/live2";
	QByteArray serverBytes;
	if (!server.isEmpty()) {
		serverBytes = server.toUtf8();
		serverStr = serverBytes.constData();
	}

	// Create a fresh rtmp_custom service with the correct settings.
	// We MUST create a new service object rather than mutating the existing one,
	// because the existing service may be rtmp_common (YouTube, Twitch, etc.)
	// and changing the key/server without changing the type causes OBS's
	// LoadStream1Settings to crash when it tries to look up the service name
	// in the UI dropdown (null dereference in strcmp).
	obs_data_t *settings = obs_data_create();
	obs_data_set_string(settings, "server", serverStr);
	obs_data_set_string(settings, "key", streamKey.toUtf8().constData());

	blog(LOG_INFO, "[obs-qr-stream-key] Creating rtmp_custom service with server: %s, key length: %d",
	     serverStr, streamKey.length());

	obs_service_t *newService = obs_service_create(
		"rtmp_custom", "default_service", settings, nullptr);

	if (!newService) {
		blog(LOG_ERROR, "[obs-qr-stream-key] Failed to create new service object");
		obs_data_release(settings);
		return;
	}

	// Replace the current streaming service and save to disk.
	// This is the correct API - it updates OBSBasic::service properly.
	obs_frontend_set_streaming_service(newService);
	obs_frontend_save_streaming_service();

	obs_service_release(newService);
	obs_data_release(settings);

	blog(LOG_INFO, "[obs-qr-stream-key] Stream settings applied and saved successfully");
}

// ─── Clear stream key: wipe the saved key so the next session starts fresh ────
static void clearStreamKey()
{
	blog(LOG_INFO, "[obs-qr-stream-key] Clearing stream key for new session");

	obs_data_t *settings = obs_data_create();
	obs_data_set_string(settings, "server", "");
	obs_data_set_string(settings, "key", "");

	obs_service_t *emptyService = obs_service_create(
		"rtmp_custom", "default_service", settings, nullptr);

	if (emptyService) {
		obs_frontend_set_streaming_service(emptyService);
		obs_frontend_save_streaming_service();
		obs_service_release(emptyService);
	}

	obs_data_release(settings);
	blog(LOG_INFO, "[obs-qr-stream-key] Stream key cleared");
}

static void onFrontendEvent(enum obs_frontend_event event, void *)
{
	if (event == OBS_FRONTEND_EVENT_FINISHED_LOADING) {
		// NOW it's safe to access the frontend
		QMainWindow *mainWindow = static_cast<QMainWindow *>(obs_frontend_get_main_window());

		// ── Rebrand window title from "OBS Studio" to "Adda Partner" ──
		mainWindow->setWindowTitle("Adda Partner");

		// ── Set window/taskbar icon to Partnerlogo ──
		QString appDir = QApplication::applicationDirPath();
		QStringList iconPaths = {
			appDir + "/../../Partnerlogo.ico",
			appDir + "/../../Partnerlogo.png",
			appDir + "/../Partnerlogo.ico",
			appDir + "/Partnerlogo.ico"
		};
		for (const QString &iconPath : iconPaths) {
			if (QFile::exists(iconPath)) {
				QIcon appIcon(iconPath);
				mainWindow->setWindowIcon(appIcon);
				QApplication::setWindowIcon(appIcon);  // This sets the taskbar icon
				blog(LOG_INFO, "[obs-qr-stream-key] Taskbar icon set from: %s",
				     iconPath.toUtf8().constData());
				break;
			}
		}

		// Add menu action
		QMenu *toolsMenu = nullptr;
		QMenuBar *menuBar = mainWindow->menuBar();

		for (QAction *action : menuBar->actions()) {
			QMenu *menu = action->menu();
			if (menu && menu->title() == "Tools") {
				toolsMenu = menu;
				break;
			}
		}

		if (toolsMenu) {
			qrAuthAction = new QAction("Adda Partner", mainWindow);
			QObject::connect(qrAuthAction, &QAction::triggered, showAuthSelectionDialog);
			toolsMenu->addAction(qrAuthAction);
		}

		// Clear any leftover stream key from previous session, then show auth
		QTimer::singleShot(1000, []() {
			clearStreamKey();
			showAuthSelectionDialog();
		});
	}

	// ── When stream ends: clear key and prompt for next class ──────────
	if (event == OBS_FRONTEND_EVENT_STREAMING_STOPPED) {
		blog(LOG_INFO, "[obs-qr-stream-key] Stream ended — clearing key and showing auth dialog");
		QTimer::singleShot(1500, []() {
			clearStreamKey();
			showAuthSelectionDialog();
		});
	}
}

bool obs_module_load(void)
{
	blog(LOG_INFO, "[obs-qr-stream-key] Adda Partner plugin loaded (version 1.5.0)");

	// Register frontend event callback - DO NOT access UI here
	obs_frontend_add_event_callback(onFrontendEvent, nullptr);

	return true;
}

void obs_module_unload(void)
{
	blog(LOG_INFO, "[obs-qr-stream-key] Adda Partner plugin unloaded");

	if (addaClient) {
		addaClient->stopPolling();
		delete addaClient;
		addaClient = nullptr;
	}

	if (relayClient) {
		relayClient->stopPolling();
		delete relayClient;
		relayClient = nullptr;
	}

	obs_frontend_remove_event_callback(onFrontendEvent, nullptr);
}
