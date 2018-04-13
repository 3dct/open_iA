/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
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
