#include "auth-selection-dialog.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QFont>
#include <QSpacerItem>

AuthSelectionDialog::AuthSelectionDialog(QWidget *parent) : QDialog(parent)
{
	setWindowTitle("Adda Partner");
	setModal(true);
	setFixedSize(460, 360);

	// Match OBS Studio dark theme
	setStyleSheet(
		"AuthSelectionDialog {"
		"  background-color: #272727;"
		"}"
	);

	QVBoxLayout *mainLayout = new QVBoxLayout(this);
	mainLayout->setSpacing(0);
	mainLayout->setContentsMargins(36, 30, 36, 24);

	// Title
	QLabel *titleLabel = new QLabel("Adda Partner", this);
	titleLabel->setAlignment(Qt::AlignCenter);
	titleLabel->setStyleSheet("font-size: 18px; font-weight: 600; color: #ffffff;");
	mainLayout->addWidget(titleLabel);

	mainLayout->addSpacing(6);

	// Subtitle
	QLabel *subtitleLabel = new QLabel("Select class type", this);
	subtitleLabel->setAlignment(Qt::AlignCenter);
	subtitleLabel->setStyleSheet("font-size: 12px; color: #808080;");
	mainLayout->addWidget(subtitleLabel);

	mainLayout->addSpacing(28);

	// ── Horizontal layout for side-by-side square buttons ──────────────
	QHBoxLayout *buttonRow = new QHBoxLayout();
	buttonRow->setSpacing(20);
	buttonRow->setContentsMargins(0, 0, 0, 0);

	// Shared button style - OBS-matching dark cards
	QString cardStyle =
		"QPushButton {"
		"  background-color: #333333;"
		"  color: #ffffff;"
		"  border: 1px solid #444444;"
		"  border-radius: 10px;"
		"  font-size: 13px;"
		"  font-weight: 600;"
		"  padding: 12px;"
		"}"
		"QPushButton:hover {"
		"  background-color: #3a3a3a;"
		"  border-color: %1;"
		"}"
		"QPushButton:pressed {"
		"  background-color: #2a2a2a;"
		"}";

	// ── PAID CLASS ─────────────────────────────────────────────────────
	QPushButton *paidBtn = new QPushButton("Paid Class", this);
	paidBtn->setFixedSize(180, 130);
	paidBtn->setCursor(Qt::PointingHandCursor);
	paidBtn->setStyleSheet(cardStyle.arg("#2d7ff9"));
	connect(paidBtn, &QPushButton::clicked, this, [this]() {
		emit adminLoginSelected();
		accept();
	});
	buttonRow->addWidget(paidBtn);

	// ── YOUTUBE CLASS ──────────────────────────────────────────────────
	QPushButton *ytBtn = new QPushButton("YouTube Class", this);
	ytBtn->setFixedSize(180, 130);
	ytBtn->setCursor(Qt::PointingHandCursor);
	ytBtn->setStyleSheet(cardStyle.arg("#cc0000"));
	connect(ytBtn, &QPushButton::clicked, this, [this]() {
		emit youtubeLiveSelected();
		accept();
	});
	buttonRow->addWidget(ytBtn);

	mainLayout->addLayout(buttonRow);

	// ── Spacer pushes Close to bottom ──────────────────────────────────
	mainLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));

	// ── Close ──────────────────────────────────────────────────────────
	QPushButton *closeBtn = new QPushButton("Close", this);
	closeBtn->setFixedHeight(32);
	closeBtn->setCursor(Qt::PointingHandCursor);
	closeBtn->setStyleSheet(
		"QPushButton {"
		"  background-color: transparent;"
		"  color: #808080;"
		"  border: 1px solid #444444;"
		"  border-radius: 6px;"
		"  font-size: 12px;"
		"}"
		"QPushButton:hover {"
		"  color: #ffffff;"
		"  border-color: #666666;"
		"}"
	);
	connect(closeBtn, &QPushButton::clicked, this, &QDialog::reject);
	mainLayout->addWidget(closeBtn);
}
