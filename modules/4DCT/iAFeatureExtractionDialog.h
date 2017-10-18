#ifndef FEATUREEXTRACTIONDIALOG_H
#define FEATUREEXTRACTIONDIALOG_H

#include "ui_iAFeatureExtraction.h"
// proj
#include "iACommonInput.h"

class iAFeatureExtractionDialog : public iACommonInput
{
	Q_OBJECT
public:
				iAFeatureExtractionDialog(QWidget* parent = 0);
				~iAFeatureExtractionDialog();

	QString		getInputImg();
	QString		getOutputFile();

private:
	Ui::iAFeatureExtractionInput	ui;
};

#endif // FEATUREEXTRACTIONDIALOG_H