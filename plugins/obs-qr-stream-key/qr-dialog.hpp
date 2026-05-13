#pragma once

#include <QDialog>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QString>

class QRDialog : public QDialog {
	Q_OBJECT

public:
	explicit QRDialog(const QString &title, const QString &qrData, QWidget *parent = nullptr);

	void updateQRCode(const QString &qrData);
	void showSuccess(const QString &message);
	void showError(const QString &message);
	void startRefreshTimer(int seconds = 120); // 2 minutes default
	void stopRefreshTimer();

signals:
	void backRequested();
	void refreshRequested();

private slots:
	void onRefreshTimerTick();
	void onRefreshButtonClicked();

private:
	QLabel *qrLabel_;
	QLabel *statusLabel_;
	QLabel *timerLabel_;
	QPushButton *backButton_;
	QPushButton *refreshButton_;
	QPushButton *cancelButton_;
	QTimer *refreshTimer_;
	int remainingSeconds_;

	QPixmap generateQRPixmap(const QString &data, int size = 300);
	void updateTimerDisplay();
};
