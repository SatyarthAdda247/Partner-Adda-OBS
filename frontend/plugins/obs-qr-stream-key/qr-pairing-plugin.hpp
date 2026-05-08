#pragma once

#include <QJsonObject>
#include <QObject>
#include <QTimer>
#include <atomic>
#include <memory>

class QRPairingServer;
class QRRelayClient;
class AddaLoginClient;
class QRPairingDialog;
class QRChoiceDialog;

class QRPairingPlugin : public QObject {
	Q_OBJECT
public:
	static QRPairingPlugin *instance();
	void init();
	void shutdown();

private slots:
	void showChoiceDialog();
	void onAdminChosen();
	void onYouTubeChosen();
	void onDialogRejected();
	void onPairingReceived(const QString &streamKey,
			       const QString &sceneCollection,
			       const QString &classId);
	void onAddaLoginConfirmed(const QJsonObject &authData);

private:
	void applyStreamKey(const QString &streamKey);
	void startTimeout();
	void shutdownNetworking();

	std::unique_ptr<QRPairingServer>  server_;
	std::unique_ptr<QRRelayClient>    relayClient_;
	std::unique_ptr<AddaLoginClient>  addaClient_;

	QRChoiceDialog  *choiceDialog_  = nullptr;
	QRPairingDialog *dialog_        = nullptr;
	QTimer          *timeoutTimer_  = nullptr;
	QString          activeToken_;
};
