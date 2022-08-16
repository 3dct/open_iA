/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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

#include "iAgui_export.h"

#include <QWidget>

class iADataSet;

class QTableWidget;
class QTableWidgetItem;

//! A list of datasets
class iAgui_API iADataSetListWidget : public QWidget
{
	Q_OBJECT
public:
	iADataSetListWidget();
	void addDataSet(iADataSet* dataset);
	void setPickableState(int idx, bool pickable);

signals:
	void editDataSet(int idx);
	void removeDataSet(int idx);
	void set3DRendererVisibility(int idx, bool visibility);
	void setBoundsVisibility(int idx, bool visibility);
	void set2DVisibility(int idx, bool visibility);
	void set3DMagicLensVisibility(int idx, bool visibility);
	void setPickable(int idx, bool pickable);

public slots:
	void enablePicking(bool enable);

private:
	void setChecked(QTableWidgetItem * item, int checked);

	QTableWidget* m_dataList;  //!< the actual table displaying datasets and controls for their visibility
};
