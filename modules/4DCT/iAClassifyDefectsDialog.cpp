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
#include "iAClassifyDefectsDialog.h"

iAClassifyDefectsDialog::iAClassifyDefectsDialog(QWidget* parent/*= 0*/)
{
	ui.setupUi(this);
	ui.Fibers->setOptions(iASetPathWidget::Mode::openFile, tr("Open Image"), tr("Extracted fibers (*.csv)"), tr("1JURVXF8HZ5K0NE1QYJ9"));
	ui.Defects->setOptions(iASetPathWidget::Mode::openFile, tr("Open Image"), tr("Extracted defects (*.txt)"), tr("IVJO2BSWPQ9NC46ICB6S"));
	ui.Output->setOptions(iASetPathWidget::Mode::directory, tr("Output directory"), tr(""), tr("3HTMBPBWBR4WF4TC32SF"));
}

iAClassifyDefectsDialog::~iAClassifyDefectsDialog()
{ }