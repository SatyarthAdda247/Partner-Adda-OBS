#pragma once

#include <QDialog>
#include <QLabel>
#include <QPushButton>

namespace Ui {
class QRPairingDialog;
}

// Displays a single QR code (either Admin or YouTube stream key)
class QRPairingDialog : public QDialog {
	Q_OBJECT
public:
	enum class Mode { Admin, YouTube };

	explicit QRPairingDialog(Mode mode,
				 const QString &url,
				 QWidget *parent = nullptr);
	~QRPairingDialog() override;

	// Called async when QR data is ready (Admin mode)
	void setQRData(const QString &qrData);

	// Status updates
	void onAdminLoginConfirmed(const QString &email);
	void onPairingSuccess(const QString &classId);
	void onPairingError(const QString &message);

signals:
	void backRequested();

private:
	void generateQRImage(const QString &data, QLabel *target);
	void applyStyle();

	Ui::QRPairingDialog *ui_;
	Mode                 mode_;
};
