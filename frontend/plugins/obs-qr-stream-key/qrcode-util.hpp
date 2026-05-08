#pragma once

#include <QImage>
#include <QString>

namespace QRCodeUtil {

// Returns a QImage of the QR code rendered at pixelSize pixels per module.
// Returns a null QImage if text is empty or encoding fails.
QImage generate(const QString &text, int pixelSize = 4);

} // namespace QRCodeUtil
