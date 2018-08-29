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

#include "iA3DObjectVis.h"
#include "FeatureScout_export.h"

#include <vtkSmartPointer.h>

#include <QColor>

class iALookupTable;

class vtkActor;
class vtkPoints;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkUnsignedCharArray;

class FeatureScout_API iA3DLineObjectVis: public iA3DObjectVis
{
public:
	iA3DLineObjectVis( iAVtkWidgetClass* widget, vtkTable* objectTable, QSharedPointer<QMap<uint, uint> > columnMapping, QColor const & neutralColor );
	void show() override;
	void hide();
	void renderSelection( std::vector<size_t> const & sortedSelInds, int classID, QColor const & classColor, QStandardItem* activeClassItem ) override;
	void renderSingle( int labelID, int classID, QColor const & classColors, QStandardItem* activeClassItem ) override;
	void multiClassRendering( QList<QColor> const & colors, QStandardItem* rootItem, double alpha ) override;
	void renderOrientationDistribution ( vtkImageData* oi ) override;
	void renderLengthDistribution(  vtkColorTransferFunction* ctFun, vtkFloatArray* extents, double halfInc, int filterID, double const * range ) override;
	void updateValues( std::vector<std::vector<double> > const & values );
	vtkPolyData* getLinePolyData();
	void setContextAlpha(int contextAlpha);
	void setSelectionColor(QColor const & selectionColor);
	void setLookupTable( QSharedPointer<iALookupTable> lut, size_t paramIndex );
protected:
	vtkSmartPointer<vtkPolyData> m_linePolyData;
	vtkSmartPointer<vtkPolyDataMapper> m_mapper;
	vtkSmartPointer<vtkUnsignedCharArray> m_colors;
	vtkSmartPointer<vtkActor> m_actor;
	vtkSmartPointer<vtkPoints> m_points;
private:
	void setPolyPointColor(int ptIdx, QColor const & qcolor);
	void updatePolyMapper();

	int m_contextAlpha;
	QColor m_selectionColor;
	QSharedPointer<iALookupTable> m_lut;
	size_t m_colorParamIdx;
};

