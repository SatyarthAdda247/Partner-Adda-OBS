#include "qr-pairing-dialog.hpp"
#include "qrcode-util.hpp"
#include "ui_qr-pairing-dialog.h"

#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QTimer>

QRPairingDialog::QRPairingDialog(Mode mode,
				 const QString &url,
				 QWidget *parent)
	: QDialog(parent), ui_(new Ui::QRPairingDialog), mode_(mode)
{
	ui_->setupUi(this);
	applyStyle();

	if (mode_ == Mode::Admin) {
		setWindowTitle("Admin Login – Adda Partner");
		ui_->modeTitleLabel->setText("Admin Login");
		ui_->statusLabel->setText("Scan with your Adda247 app to login");
		ui_->urlLabel->setText("");
		// QR will be set later via setQRData() once API responds
	} else {
		setWindowTitle("YouTube Live – Adda Partner");
		ui_->modeTitleLabel->setText("YouTube class login");
		ui_->statusLabel->setText("Scan with Adda247 teacher app");
		ui_->urlLabel->setText(url);
		// Generate YouTube stream key QR immediately
		generateQRImage(url, ui_->qrLabel);
	}

	connect(ui_->skipButton, &QPushButton::clicked,
		this, &QDialog::reject);
	connect(ui_->backButton, &QPushButton::clicked,
		this, &QRPairingDialog::backRequested);
}

QRPairingDialog::~QRPairingDialog()
{
	delete ui_;
}

void QRPairingDialog::setQRData(const QString &qrData)
{
	if (!qrData.isEmpty())
		generateQRImage(qrData, ui_->qrLabel);
}

// Matches original exactly — pixelSize 4, no extra scaling
void QRPairingDialog::generateQRImage(const QString &data, QLabel *target)
{
	if (!target || data.isEmpty())
		return;

	// Use pixelSize=6 so every module is 6px — fully visible at any QR complexity
	QImage image = QRCodeUtil::generate(data, 6);
	if (image.isNull()) {
		target->setText("QR generation failed.\nUse URL below.");
	} else {
		target->setPixmap(QPixmap::fromImage(image));
	}
}

void QRPairingDialog::onAdminLoginConfirmed(const QString &email)
{
	if (ui_->statusLabel)
		ui_->statusLabel->setText("✓ Logged in: " + email);
}

void QRPairingDialog::onPairingSuccess(const QString &classId)
{
	Q_UNUSED(classId)
	if (ui_->statusLabel)
		ui_->statusLabel->setText("✓ Stream key applied!");
	QTimer::singleShot(800, this, &QDialog::accept);
}

void QRPairingDialog::onPairingError(const QString &message)
{
	if (ui_->statusLabel)
		ui_->statusLabel->setText("Error: " + message);
}

void QRPairingDialog::applyStyle()
{
	setStyleSheet(R"(
		QDialog {
			background-color: #1e2029;
		}
		QLabel#modeTitleLabel {
			color: #ffffff;
			font-size: 18px;
			font-weight: bold;
		}
		QLabel#statusLabel {
			color: #9ca3af;
			font-size: 13px;
		}
		QLabel#urlLabel {
			color: #6b7280;
			font-size: 11px;
		}
		QLabel#qrLabel {
			background-color: #ffffff;
			border-radius: 8px;
			padding: 8px;
		}
		QPushButton#skipButton {
			background-color: #374151;
			color: #d1d5db;
			border: none;
			border-radius: 8px;
			padding: 8px 20px;
			font-size: 13px;
		}
		QPushButton#skipButton:hover { background-color: #4b5563; }
		QPushButton#backButton {
			background-color: transparent;
			color: #6b7280;
			border: none;
			padding: 8px 12px;
			font-size: 13px;
		}
		QPushButton#backButton:hover { color: #9ca3af; }
	)");
}
