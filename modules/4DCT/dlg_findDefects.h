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
 
#ifndef DLG_FINDDEFECTS_H
#define DLG_FINDDEFECTS_H
// Ui
#include "ui_iA4DCTFindDefectsDialog.h"
// iA
//#include "iA4DCTStageData.h"
#include "iA4DCTData.h"
// Qt
#include <QDialog>
#include <QString>
#include <QVector>

class QComboBox;
class QDialogButtonBox;
class QLabel;
class QLineEdit;
class QPushButton;

class dlg_findDefects : public QDialog, public Ui::FindDefectsDialog
{
	Q_OBJECT
public:
						dlg_findDefects( QWidget* parent = 0 );
						dlg_findDefects( iA4DCTData * data, QWidget* parent = 0 );
						~dlg_findDefects();

	int					getStageIndex();
	int					getIntensityImgIndex();
	int					getLabledImgIndex();
	QString				getOutputDir();

private:
	// internal data
	iA4DCTData *		m_data;

private slots:
	void				stageCurrentIndexChanged( int ind );
	void				setSaveDir();
};

#endif // DLG_FINDDEFECTS_H
