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

#include "iAguibase_export.h"

#include <QWidget>

class iADataSet;

class QTableWidget;
class QTableWidgetItem;

//! A list of datasets
class iAguibase_API iADataSetListWidget : public QWidget
{
	Q_OBJECT
public:
	iADataSetListWidget();
	int addDataSet(iADataSet const* dataset, size_t dataSetID, bool render3DChecked, bool render3DCheckable, bool render2D);
	void setName(size_t dataSetIdx, QString newName);
	void setPickableState(size_t dataSetIdx, bool pickable);

signals:
	void editDataSet(size_t dataSetIdx);
	void removeDataSet(size_t dataSetIdx);
	void set3DRendererVisibility(size_t dataSetIdx, bool visibility);
	void setBoundsVisibility(size_t dataSetIdx, bool visibility);
	void set2DVisibility(size_t dataSetIdx, bool visibility);
	void set3DMagicLensVisibility(size_t dataSetIdx, bool visibility);
	void setPickable(size_t dataSetIdx, bool pickable);
	void dataSetSelected(size_t dataSetIdx);

public slots:
	void enablePicking(bool enable);

private:
	void setChecked(QTableWidgetItem * item, int checked);
	int findDataSetIdx(size_t idx);

	QTableWidget* m_dataList;  //!< the actual table displaying datasets and controls for their visibility
};
