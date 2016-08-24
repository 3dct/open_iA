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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
 
#ifndef DLG_SHOWDEFECTS_H
#define DLG_SHOWDEFECTS_H
// iA
//#include "iA4DCTStageData.h"
#include "iA4DCTData.h"
// Ui
#include "ui_iA4DCTShowDefectsDialog.h"
// Qt
#include <QDialog>
#include <QVector>

class QDialogButtonBox;
class QLabel;
class QComboBox;

class dlg_showDefects : public QDialog, public Ui::ShowDefectsDialog
{
	Q_OBJECT
public:
						dlg_showDefects( QWidget * parent = 0 );
						dlg_showDefects( iA4DCTData * data, QWidget * parent = 0 );
						~dlg_showDefects();

	int					getStageIndex();
	int					getIntensityImgIndex();
	int					getLabledImgIndex();
	int					getPulloutsFileIndex();
	int					getCracksFileIndex();
	int					getDebondingsFileIndex();
	int					getBreakagesFileIndex();

private slots:
	void				stageCurrentIndexChanged(int ind);

private:
	iA4DCTData *		m_data;
};

#endif // DLG_SHOWDEFECTS_H