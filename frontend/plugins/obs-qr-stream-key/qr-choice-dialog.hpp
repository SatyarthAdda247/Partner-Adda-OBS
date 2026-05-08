#pragma once

#include <QDialog>

namespace Ui {
class QRChoiceDialog;
}

// Shown on startup — lets the user pick Admin Login or YouTube Live
class QRChoiceDialog : public QDialog {
	Q_OBJECT
public:
	enum class Choice { None, Admin, YouTube };

	explicit QRChoiceDialog(QWidget *parent = nullptr);
	~QRChoiceDialog() override;

	Choice choice() const { return choice_; }

private:
	Ui::QRChoiceDialog *ui_;
	Choice              choice_ = Choice::None;
};
