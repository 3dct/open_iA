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
#include "dlg_showDefects.h"
// Qt
#include <QDialogButtonBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QComboBox>

dlg_showDefects::dlg_showDefects( QWidget * parent /*= 0*/ ) :
	QDialog(parent)
{
	setupUi(this);
	connect(cbStage, SIGNAL(currentIndexChanged(int)), this, SLOT(stageCurrentIndexChanged(int)));
}

dlg_showDefects::dlg_showDefects( iA4DCTData * data, QWidget * parent /*= 0*/ )
	: dlg_showDefects(parent)
{
	m_data = data;

	for ( auto d : *m_data ) {
		cbStage->addItem( QString::number( d->Force ) );
	}
}

dlg_showDefects::~dlg_showDefects()
{ /* not implemented */ }

void dlg_showDefects::stageCurrentIndexChanged(int ind)
{
	if ( m_data->size() <= 0 )
		return;

	cbIntensityImg->clear();
	cbLabeledImg->clear();
	cbPullout->clear();
	cbCracking->clear();
	cbFracture->clear();
	cbDebonding->clear();

	for (iA4DCTFileData f : m_data->at(ind)->Files) {
		cbIntensityImg->addItem(f.Name);
		cbLabeledImg->addItem(f.Name);
		cbPullout->addItem(f.Name);
		cbCracking->addItem(f.Name);
		cbFracture->addItem(f.Name);
		cbDebonding->addItem(f.Name);
	}
}

int dlg_showDefects::getStageIndex()
{
	return cbStage->currentIndex();
}

int dlg_showDefects::getIntensityImgIndex()
{
	return cbIntensityImg->currentIndex();
}

int dlg_showDefects::getLabledImgIndex()
{
	return cbLabeledImg->currentIndex();
}

int dlg_showDefects::getPulloutsFileIndex()
{
	return cbPullout->currentIndex();
}

int dlg_showDefects::getCracksFileIndex()
{
	return cbCracking->currentIndex();
}

int dlg_showDefects::getDebondingsFileIndex()
{
	return cbDebonding->currentIndex();
}

int dlg_showDefects::getBreakagesFileIndex()
{
	return cbFracture->currentIndex();
}
