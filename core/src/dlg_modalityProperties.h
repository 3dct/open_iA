/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
#pragma once

#include "iAVolumeSettings.h"

#include <QSharedPointer>

#include "ui_modalityProperties.h"
#include "qthelper/iAQTtoUIConnector.h"
typedef iAQTtoUIConnector<QDialog, Ui_modalityProperties> dlg_modalityPropertiesUI;

class iAModality;
class vtkRenderer;

class dlg_modalityProperties : public dlg_modalityPropertiesUI
{
	Q_OBJECT
public:
	dlg_modalityProperties(QWidget * parent, QSharedPointer<iAModality> modality);
	bool spacingChanged();
	double const * newSpacing();
public slots:
	void OKButtonClicked();
private:
	QSharedPointer<iAModality> m_modality;
	iAVolumeSettings m_volumeSettings;
	bool m_spacingChanged;
	double const * m_currentSpacing;
};