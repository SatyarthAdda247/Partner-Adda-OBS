#include "qr-dialog.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QFont>
#include <QTimer>
#include <QImage>
#include <obs-module.h>
#include "qrcodegen.hpp"

// Generate real QR code using qrcodegen library
QPixmap QRDialog::generateQRPixmap(const QString &data, int size)
{
	if (data.isEmpty()) {
		blog(LOG_WARNING, "[obs-qr-stream-key] Empty QR data provided");
		return QPixmap();
	}

	// Log the QR data for debugging
	blog(LOG_INFO, "[obs-qr-stream-key] Generating QR for data: %s (length: %d)",
	     data.toUtf8().constData(), data.length());

	try {
		// Validate data length (QR codes have limits)
		if (data.length() > 2953) {
			blog(LOG_ERROR, "[obs-qr-stream-key] QR data too long: %d characters (max 2953)",
			     data.length());
			return QPixmap();
		}

		// Generate QR code using qrcodegen library with HIGH error correction
		const qrcodegen::QrCode qr = qrcodegen::QrCode::encodeText(
			data.toUtf8().constData(), qrcodegen::QrCode::Ecc::HIGH);

		int qrSize = qr.getSize();
		blog(LOG_INFO, "[obs-qr-stream-key] QR code generated: %dx%d modules", qrSize, qrSize);

		// Calculate module size to fit the desired output size
		// Use larger module size for better scanning
		int moduleSize = size / qrSize;
		if (moduleSize < 2)
			moduleSize = 2; // Minimum 2 pixels per module

		int actualSize = qrSize * moduleSize;

		// Create QImage with the QR code
		QImage image(actualSize, actualSize, QImage::Format_RGB32);
		image.fill(Qt::white);

		QPainter painter(&image);
		painter.setPen(Qt::NoPen);
		painter.setBrush(Qt::black);
		painter.setRenderHint(QPainter::Antialiasing, false); // Sharp edges for QR

		// Draw QR code modules
		for (int y = 0; y < qrSize; y++) {
			for (int x = 0; x < qrSize; x++) {
				if (qr.getModule(x, y)) {
					painter.drawRect(x * moduleSize, y * moduleSize, moduleSize,
							 moduleSize);
				}
			}
		}

		painter.end();

		// Convert to pixmap and scale to desired size if needed
		QPixmap pixmap = QPixmap::fromImage(image);
		if (actualSize != size) {
			// Use SmoothTransformation for better quality
			pixmap = pixmap.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
		}

		blog(LOG_INFO, "[obs-qr-stream-key] QR code pixmap created: %dx%d", pixmap.width(),
		     pixmap.height());

		return pixmap;

	} catch (const qrcodegen::data_too_long &e) {
		blog(LOG_ERROR, "[obs-qr-stream-key] QR data too long: %s", e.what());
		return QPixmap();
	} catch (const std::exception &e) {
		blog(LOG_ERROR, "[obs-qr-stream-key] Failed to generate QR code: %s", e.what());
		return QPixmap();
	}
}

QRDialog::QRDialog(const QString &title, const QString &qrData, QWidget *parent)
	: QDialog(parent), remainingSeconds_(120)
{
	setWindowTitle(title);
	setModal(true);
	setMinimumSize(450, 620);

	// Initialize refresh timer
	refreshTimer_ = new QTimer(this);
	connect(refreshTimer_, &QTimer::timeout, this, &QRDialog::onRefreshTimerTick);

	QVBoxLayout *mainLayout = new QVBoxLayout(this);
	mainLayout->setSpacing(20);
	mainLayout->setContentsMargins(30, 30, 30, 30);

	// Title
	QLabel *titleLabel = new QLabel(title, this);
	QFont titleFont = titleLabel->font();
	titleFont.setPointSize(16);
	titleFont.setBold(true);
	titleLabel->setFont(titleFont);
	titleLabel->setAlignment(Qt::AlignCenter);
	mainLayout->addWidget(titleLabel);

	// Instructions
	QLabel *instructionLabel =
		new QLabel("Scan this QR code with your mobile app to authenticate:", this);
	instructionLabel->setWordWrap(true);
	instructionLabel->setAlignment(Qt::AlignCenter);
	instructionLabel->setStyleSheet("color: #666; margin-bottom: 10px;");
	mainLayout->addWidget(instructionLabel);

	// QR Code
	qrLabel_ = new QLabel(this);
	qrLabel_->setAlignment(Qt::AlignCenter);
	qrLabel_->setStyleSheet("background-color: white; border: 2px solid #ddd; border-radius: 8px; "
				"padding: 20px;");
	qrLabel_->setMinimumSize(340, 340);

	if (!qrData.isEmpty()) {
		QPixmap qrPixmap = generateQRPixmap(qrData, 300);
		qrLabel_->setPixmap(qrPixmap);
	}

	mainLayout->addWidget(qrLabel_, 0, Qt::AlignCenter);

	// Timer label (Time remaining: 01:59)
	timerLabel_ = new QLabel("Time remaining: 02:00", this);
	timerLabel_->setAlignment(Qt::AlignCenter);
	QFont timerFont = timerLabel_->font();
	timerFont.setPointSize(12);
	timerFont.setBold(true);
	timerLabel_->setFont(timerFont);
	timerLabel_->setStyleSheet("color: #4CAF50; margin-top: 10px;");
	mainLayout->addWidget(timerLabel_);

	// Refresh button (centered, below timer)
	refreshButton_ = new QPushButton("🔄 Refresh QR Code", this);
	refreshButton_->setMinimumHeight(40);
	refreshButton_->setStyleSheet(
		"QPushButton {"
		"  background-color: #2196F3;"
		"  color: white;"
		"  border: none;"
		"  border-radius: 6px;"
		"  font-size: 13px;"
		"  font-weight: bold;"
		"  padding: 10px 20px;"
		"}"
		"QPushButton:hover {"
		"  background-color: #1976D2;"
		"}");
	connect(refreshButton_, &QPushButton::clicked, this, &QRDialog::onRefreshButtonClicked);
	mainLayout->addWidget(refreshButton_, 0, Qt::AlignCenter);

	// Status label
	statusLabel_ = new QLabel("Waiting for authentication...", this);
	statusLabel_->setAlignment(Qt::AlignCenter);
	statusLabel_->setStyleSheet("color: #666; font-style: italic; margin-top: 10px;");
	mainLayout->addWidget(statusLabel_);

	mainLayout->addStretch();

	// Button layout (Back and Cancel)
	QHBoxLayout *buttonLayout = new QHBoxLayout();
	buttonLayout->setSpacing(10);

	// Back button
	backButton_ = new QPushButton("← Back", this);
	backButton_->setMinimumHeight(40);
	backButton_->setStyleSheet(
		"QPushButton {"
		"  background-color: #2196F3;"
		"  color: white;"
		"  border: none;"
		"  border-radius: 6px;"
		"  font-size: 12px;"
		"  padding: 8px;"
		"}"
		"QPushButton:hover {"
		"  background-color: #1976D2;"
		"}");

	connect(backButton_, &QPushButton::clicked, this, [this]() {
		stopRefreshTimer();
		emit backRequested();
		reject();
	});

	// Cancel button
	cancelButton_ = new QPushButton("Cancel", this);
	cancelButton_->setMinimumHeight(40);
	cancelButton_->setStyleSheet(
		"QPushButton {"
		"  background-color: #f0f0f0;"
		"  color: #333;"
		"  border: 1px solid #ccc;"
		"  border-radius: 6px;"
		"  font-size: 12px;"
		"  padding: 8px;"
		"}"
		"QPushButton:hover {"
		"  background-color: #e0e0e0;"
		"}");

	connect(cancelButton_, &QPushButton::clicked, this, [this]() {
		stopRefreshTimer();
		reject();
	});

	buttonLayout->addWidget(backButton_);
	buttonLayout->addWidget(cancelButton_);
	mainLayout->addLayout(buttonLayout);

	setLayout(mainLayout);
}

void QRDialog::updateQRCode(const QString &qrData)
{
	if (!qrData.isEmpty()) {
		QPixmap qrPixmap = generateQRPixmap(qrData, 300);
		qrLabel_->setPixmap(qrPixmap);
	}
}

void QRDialog::showSuccess(const QString &message)
{
	statusLabel_->setText(message);
	statusLabel_->setStyleSheet("color: #4CAF50; font-weight: bold; font-size: 14px;");

	// Don't auto-close - let user see the success message and close manually
	// User can click Cancel or wait for stream configuration to complete
}

void QRDialog::showError(const QString &message)
{
	statusLabel_->setText(message);
	statusLabel_->setStyleSheet("color: #f44336; font-weight: bold; font-size: 14px;");
}

void QRDialog::startRefreshTimer(int seconds)
{
	remainingSeconds_ = seconds;
	updateTimerDisplay();
	refreshTimer_->start(1000); // Update every second
	blog(LOG_INFO, "[obs-qr-stream-key] QR refresh timer started (%d seconds)", seconds);
}

void QRDialog::stopRefreshTimer()
{
	if (refreshTimer_) {
		refreshTimer_->stop();
	}
}

void QRDialog::onRefreshTimerTick()
{
	remainingSeconds_--;
	updateTimerDisplay();

	if (remainingSeconds_ <= 0) {
		// Timer expired - auto refresh
		blog(LOG_INFO, "[obs-qr-stream-key] QR timer expired, auto-refreshing...");
		stopRefreshTimer();
		emit refreshRequested();
	} else if (remainingSeconds_ <= 30) {
		// Last 30 seconds - change color to orange/red
		timerLabel_->setStyleSheet("color: #FF9800; font-weight: bold;");
	}
}

void QRDialog::onRefreshButtonClicked()
{
	blog(LOG_INFO, "[obs-qr-stream-key] Refresh button clicked");
	stopRefreshTimer();
	emit refreshRequested();
}

void QRDialog::updateTimerDisplay()
{
	int minutes = remainingSeconds_ / 60;
	int seconds = remainingSeconds_ % 60;
	QString timeStr = QString("Time remaining: %1:%2")
				  .arg(minutes, 2, 10, QChar('0'))
				  .arg(seconds, 2, 10, QChar('0'));
	timerLabel_->setText(timeStr);
}
