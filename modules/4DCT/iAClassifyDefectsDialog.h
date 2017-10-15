#ifndef CLASSIFYDEFECTSDIALOG_H
#define CLASSIFYDEFECTSDIALOG_H

// qt
#include <QDialog>
// ui
#include "ui_iAClassifyDefectsDialog.h"

class iAClassifyDefectsDialog : public QDialog
{
	Q_OBJECT

public:
	iAClassifyDefectsDialog(QWidget* parent = 0);
	~iAClassifyDefectsDialog();
	Ui::ClassifyDefects ui;
};

#endif // CLASSIFYDEFECTSDIALOG_H