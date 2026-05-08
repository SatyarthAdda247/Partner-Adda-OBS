#pragma once

#include <QObject>
#include <QString>
#include <QTimer>

#include <atomic>
#include <memory>
#include <thread>

/// Handles cloud relay pairing via the Adda247 backend.
///
/// Flow:
///   1. OBS calls registerSession() → backend stores token, returns session URL
///   2. QR code encodes the session URL (https://liveclasses.adda247.com/obs/pair?token=...)
///   3. Mobile app scans QR, POSTs stream key to backend
///   4. OBS polls backend every 2s via pollForResult()
///   5. When backend returns streamKey, emits pairingReceived signal
class QRRelayClient : public QObject {
	Q_OBJECT
public:
	explicit QRRelayClient(QObject *parent = nullptr);
	~QRRelayClient();

	/// Register a pairing session with the backend.
	/// Returns the QR URL to encode, or empty string on failure.
	QString registerSession(const QString &token, const QString &facultyId);

	/// Start polling the backend for a pairing result every 2 seconds.
	void startPolling(const QString &token);

	/// Stop polling.
	void stopPolling();

	/// Invalidate the session on the backend (called on dismiss/timeout).
	void invalidateSession(const QString &token);

signals:
	void pairingReceived(const QString &streamKey,
			     const QString &sceneCollection,
			     const QString &classId);
	void errorOccurred(const QString &message);

private slots:
	void doPoll();

private:
	static constexpr const char *kRelayBase =
		"https://liveclasses.adda247.com";
	static constexpr const char *kRegisterPath =
		"/api/v1/obs/session";
	static constexpr const char *kPollPath =
		"/api/v1/obs/session/";
	static constexpr const char *kInvalidatePath =
		"/api/v1/obs/session/";

	QTimer *pollTimer_ = nullptr;
	QString activeToken_;
	std::atomic<bool> polling_{false};
};
