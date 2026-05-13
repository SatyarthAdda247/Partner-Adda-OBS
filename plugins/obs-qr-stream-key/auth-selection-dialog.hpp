#pragma once

#include <QDialog>

class AuthSelectionDialog : public QDialog {
	Q_OBJECT

public:
	explicit AuthSelectionDialog(QWidget *parent = nullptr);

signals:
	void adminLoginSelected();
	void youtubeLiveSelected();
};
