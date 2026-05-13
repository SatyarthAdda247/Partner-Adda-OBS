#pragma once

#include <QObject>
#include <QString>
#include <QTimer>
#include <QJsonObject>

class AddaLoginClient : public QObject {
	Q_OBJECT

public:
	explicit AddaLoginClient(QObject *parent = nullptr);
	~AddaLoginClient();

	QString generateQR();
	void startPolling();
	void stopPolling();

signals:
	void loginConfirmed(const QJsonObject &authData);

private slots:
	void doPoll();

private:
	static constexpr const char *kHost = "admin.adda247.com";
	static constexpr int kPort = 443;
	static constexpr const char *kGeneratePath = "/api/users/qr-login/generate";

	QString sessionId_;
	QTimer *pollTimer_;
};
