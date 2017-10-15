#include "iAFeatureExtractionDialog.h"

const QString S_FEATUREEXTRACTION_LAST_DIR = "LastDirFeatureExtraction";

iAFeatureExtractionDialog::iAFeatureExtractionDialog(QWidget* parent /* = 0 */)
	: iACommonInput(parent)
{
	ui.setupUi(this);
	ui.InputImage->setOptions(iASetPathWidget::Mode::openFile, tr("Open Image"), tr("Image files (*.mhd)"), tr("37VWR30DXKPFJE56MJDG"));
	ui.OutputImage->setOptions(iASetPathWidget::Mode::saveFile, tr("Open Image"), tr("Text files (*.txt)"), tr("OEM1QY6V3MZ4Z437UTQ9"));
}

iAFeatureExtractionDialog::~iAFeatureExtractionDialog()
{ /* not implemented */ }

QString iAFeatureExtractionDialog::getInputImg()
{
	return ui.InputImage->ui.Path->text();
}

QString iAFeatureExtractionDialog::getOutputFile()
{
	return ui.OutputImage->ui.Path->text();
}
