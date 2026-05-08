#include "network-util.hpp"

#include <QNetworkInterface>
#include <QAbstractSocket>
#include <QTcpServer>

namespace NetworkUtil {

QString localIPv4Address()
{
	const auto interfaces = QNetworkInterface::allInterfaces();
	for (const QNetworkInterface &iface : interfaces) {
		// Skip loopback and non-running interfaces
		const auto flags = iface.flags();
		if (flags.testFlag(QNetworkInterface::IsLoopBack))
			continue;
		if (!flags.testFlag(QNetworkInterface::IsRunning))
			continue;

		const auto entries = iface.addressEntries();
		for (const QNetworkAddressEntry &entry : entries) {
			const QHostAddress addr = entry.ip();
			if (addr.protocol() == QAbstractSocket::IPv4Protocol)
				return addr.toString();
		}
	}
	return {};
}

uint16_t findAvailablePort()
{
	QTcpServer server;
	// Binding to port 0 lets the OS assign a free ephemeral port
	if (!server.listen(QHostAddress::LocalHost, 0))
		return 0;

	const uint16_t port = server.serverPort();
	server.close();

	// Validate the OS assigned a port in the ephemeral range
	if (port < 49152 || port > 65535)
		return 0;

	return port;
}

} // namespace NetworkUtil
