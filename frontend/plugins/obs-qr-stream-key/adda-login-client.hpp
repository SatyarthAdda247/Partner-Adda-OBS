#pragma once

#include <QObject>
#include <QTimer>
#include <QString>
#include <QJsonObject>

/// Handles Adda247 admin QR login flow.
///
/// Flow:
///   1. Call generateQR() → backend returns qrData + sessionId
///   2. Encode qrData as QR code (shown in dialog)
///   3. Poll status every 2s until confirmed
///   4. Emit loginConfirmed with full authData on success
class AddaLoginClient : public QObject {
	Q_OBJECT
public:
	explicit AddaLoginClient(QObject *parent = nullptr);
	~AddaLoginClient();

	/// Generate a new QR session. Returns the qrData string to encode,
	/// or empty string on failure.
	QString generateQR();

	/// Start polling for login confirmation.
	void startPolling();

	/// Stop polling.
	void stopPolling();

signals:
	// Emits the full authData object so caller can use any field
	void loginConfirmed(const QJsonObject &authData);
	void errorOccurred(const QString &message);

private slots:
	void doPoll();

private:
	static constexpr const char *kHost = "admin.adda247.com";
	static constexpr int kPort = 443;
	static constexpr const char *kGeneratePath =
		"/api/users/qr-login/generate";

	QTimer *pollTimer_ = nullptr;
	QString sessionId_;
};
