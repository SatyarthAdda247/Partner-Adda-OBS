#pragma once

#include <QString>
#include <cstdint>

namespace NetworkUtil {

// Returns the first non-loopback, running IPv4 address found on any network
// interface. Returns an empty string if no suitable interface is found.
QString localIPv4Address();

// Returns a random available port in [49152, 65535].
uint16_t findAvailablePort();

} // namespace NetworkUtil
