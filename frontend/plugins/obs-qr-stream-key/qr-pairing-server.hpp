#pragma once

#include <QObject>
#include <QString>

#include <atomic>
#include <mutex>
#include <thread>

#include <httplib.h>

class QRPairingServer : public QObject {
	Q_OBJECT
public:
	explicit QRPairingServer(QObject *parent = nullptr);
	~QRPairingServer();

	// Binds and starts listening on the given port in a background thread.
	// Returns false if the port is unavailable or the server fails to start.
	bool start(uint16_t port);

	// Stops the server and joins the background thread.
	void stop();

	// Returns the port the server is listening on (0 if not started).
	uint16_t port() const;

	// Thread-safe token management.
	void setToken(const QString &token);
	void invalidateToken();

signals:
	void pairingReceived(const QString &streamKey,
			     const QString &sceneCollection,
			     const QString &classId);
	void errorOccurred(const QString &message);

private:
	void handlePairRequest(const httplib::Request &req,
			       httplib::Response &res);

	httplib::Server svr_;
	std::thread serverThread_;
	std::atomic<bool> running_{false};
	QString token_;
	std::mutex tokenMutex_;
	uint16_t port_ = 0;
};
