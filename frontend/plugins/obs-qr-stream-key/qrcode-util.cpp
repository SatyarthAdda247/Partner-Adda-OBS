#include "qrcode-util.hpp"

#include <qrcodegen.hpp>

namespace QRCodeUtil {

QImage generate(const QString &text, int pixelSize)
{
	if (text.isEmpty())
		return QImage();

	try {
		const qrcodegen::QrCode qr = qrcodegen::QrCode::encodeText(
			text.toUtf8().constData(),
			qrcodegen::QrCode::Ecc::MEDIUM);

		const int size = qr.getSize();
		const int imgSize = size * pixelSize;

		QImage image(imgSize, imgSize, QImage::Format_RGB32);
		image.fill(Qt::white);

		for (int y = 0; y < size; ++y) {
			for (int x = 0; x < size; ++x) {
				if (qr.getModule(x, y)) {
					// Fill pixelSize x pixelSize block
					for (int py = 0; py < pixelSize; ++py) {
						for (int px = 0; px < pixelSize; ++px) {
							image.setPixel(
								x * pixelSize + px,
								y * pixelSize + py,
								qRgb(0, 0, 0));
						}
					}
				}
			}
		}

		return image;
	} catch (...) {
		return QImage();
	}
}

} // namespace QRCodeUtil
