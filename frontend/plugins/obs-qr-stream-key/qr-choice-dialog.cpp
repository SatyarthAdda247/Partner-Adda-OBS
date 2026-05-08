#include "qr-choice-dialog.hpp"
#include "ui_qr-choice-dialog.h"

QRChoiceDialog::QRChoiceDialog(QWidget *parent)
	: QDialog(parent), ui_(new Ui::QRChoiceDialog)
{
	ui_->setupUi(this);
	setWindowTitle("Adda Partner – Connect");
	setModal(true);

	// Style the dialog
	setStyleSheet(R"(
		QDialog {
			background-color: #1e2029;
		}
		QLabel#titleLabel {
			color: #ffffff;
			font-size: 20px;
			font-weight: bold;
			margin-bottom: 2px;
		}
		QLabel#subtitleLabel {
			color: #9ca3af;
			font-size: 13px;
		}
		QPushButton#adminButton {
			background-color: #dc2626;
			color: #ffffff;
			font-size: 15px;
			font-weight: bold;
			border: none;
			border-radius: 10px;
			padding: 12px 20px;
		}
		QPushButton#adminButton:hover {
			background-color: #ef4444;
		}
		QPushButton#adminButton:pressed {
			background-color: #b91c1c;
		}
		QPushButton#youtubeButton {
			background-color: #1e293b;
			color: #ffffff;
			font-size: 15px;
			font-weight: bold;
			border: 2px solid #374151;
			border-radius: 10px;
			padding: 12px 20px;
		}
		QPushButton#youtubeButton:hover {
			background-color: #374151;
			border-color: #6b7280;
		}
		QPushButton#youtubeButton:pressed {
			background-color: #111827;
		}
		QPushButton#cancelButton {
			background-color: transparent;
			color: #6b7280;
			font-size: 13px;
			border: none;
			padding: 4px;
		}
		QPushButton#cancelButton:hover {
			color: #9ca3af;
		}
	)");

	connect(ui_->adminButton, &QPushButton::clicked, this, [this]() {
		choice_ = Choice::Admin;
		accept();
	});

	connect(ui_->youtubeButton, &QPushButton::clicked, this, [this]() {
		choice_ = Choice::YouTube;
		accept();
	});

	connect(ui_->cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

QRChoiceDialog::~QRChoiceDialog()
{
	delete ui_;
}
