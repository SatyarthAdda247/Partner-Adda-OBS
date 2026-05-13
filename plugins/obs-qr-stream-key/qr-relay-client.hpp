#pragma once

#include <QObject>
#include <QString>
#include <QTimer>
#include <atomic>

class QRRelayClient : public QObject {
	Q_OBJECT

public:
	explicit QRRelayClient(QObject *parent = nullptr);
	~QRRelayClient();

	QString generateQR();
	void startPolling();
	void stopPolling();

signals:
	void pairingReceived(const QString &streamKey, const QString &sceneCollection, const QString &classId);

private:
	QString registerSession(const QString &token, const QString &facultyId);
	void doPoll();

	QTimer *pollTimer_ = nullptr;
	QString activeToken_;
	std::atomic<bool> polling_{false};
};
