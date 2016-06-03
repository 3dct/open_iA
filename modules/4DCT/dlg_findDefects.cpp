/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 
#include "pch.h"
#include "dlg_findDefects.h"
// iA
#include "iA4DCTFileData.h"
#include "iA4DCTSettings.h"
// Qt
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

dlg_findDefects::dlg_findDefects( QWidget * parent /*= 0*/ ) :
QDialog(parent)
{
	setupUi(this);
	connect(cbStage, SIGNAL(currentIndexChanged(int)), this, SLOT(stageCurrentIndexChanged(int)));
	connect(pbSetOutputDir, SIGNAL(clicked()), this, SLOT(setSaveDir()));
}

dlg_findDefects::dlg_findDefects( iA4DCTData * data, QWidget* parent /*= 0*/ ) :
	dlg_findDefects(parent)
{
	m_data = data;

	for( auto d : *m_data ) {
		cbStage->addItem(QString::number(d->Force));
	}
}

dlg_findDefects::~dlg_findDefects()
{ /* empty */ }

int dlg_findDefects::getStageIndex()
{
	return cbStage->currentIndex();
}

int dlg_findDefects::getIntensityImgIndex()
{
	return cbIntensityImg->currentIndex();
}

int dlg_findDefects::getLabledImgIndex()
{
	return cbLabeledImg->currentIndex();
}

void dlg_findDefects::stageCurrentIndexChanged(int ind)
{
	if (m_data->size() <= 0)
		return;

	cbIntensityImg->clear();
	cbLabeledImg->clear();

	for (iA4DCTFileData f : m_data->at(ind)->Files) {
		cbIntensityImg->addItem(f.Name);
		cbLabeledImg->addItem(f.Name);
	}
}

void dlg_findDefects::setSaveDir()
{
	QSettings settings;
	QString path = QFileDialog::getExistingDirectory(
		this, 
		tr("Set output directory"),
		settings.value(S_4DCT_OUTPUT_FINDER_DIR).toString() );
	QDir dir(path);
	if (dir.exists()) {
		settings.setValue(S_4DCT_OUTPUT_FINDER_DIR, path);
		leOutputDir->setText(path);
	}
}

QString dlg_findDefects::getOutputDir()
{
	return leOutputDir->text();
}
