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
#pragma once

#include "iA3DColoredPolyObjectVis.h"

#include <vtkSmartPointer.h>

class iALookupTable;

class vtkActor;
class vtkOutlineFilter;
class vtkPoints;
class vtkPolyData;

class FeatureScout_API iA3DLineObjectVis: public iA3DColoredPolyObjectVis
{
public:
	iA3DLineObjectVis( iAVtkWidgetClass* widget, vtkTable* objectTable, QSharedPointer<QMap<uint, uint> > columnMapping, QColor const & neutralColor );
	void updateValues( std::vector<std::vector<double> > const & values );
	vtkPolyData* getLinePolyData();
	void setLookupTable( QSharedPointer<iALookupTable> lut, size_t paramIndex );
	void setSelection ( std::vector<size_t> const & sortedSelInds, bool selectionActive );
	void updateColorSelectionRendering();
	void setColor(QColor const & color);
	void showBoundingBox();
	void hideBoundingBox();
protected:
	vtkSmartPointer<vtkPolyData> m_linePolyData;
	vtkSmartPointer<vtkPoints> m_points;

	vtkSmartPointer<vtkOutlineFilter> m_outlineFilter;
	vtkSmartPointer<vtkPolyDataMapper> m_outlineMapper;
	vtkSmartPointer<vtkActor> m_outlineActor;
private:
	QSharedPointer<iALookupTable> m_lut;
	size_t m_colorParamIdx;
	std::vector<size_t> m_selection;
	bool m_selectionActive;
};

