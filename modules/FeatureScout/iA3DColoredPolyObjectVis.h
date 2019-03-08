/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#include "iA3DObjectVis.h"

#include <vtkSmartPointer.h>

#include <QColor>

class iALookupTable;

class vtkActor;
class vtkOutlineFilter;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkUnsignedCharArray;

class FeatureScout_API iA3DColoredPolyObjectVis : public iA3DObjectVis
{
public:
	static const int DefaultContextOpacity = 8;
	static const int DefaultSelectionOpacity = 128;
	iA3DColoredPolyObjectVis(iAVtkWidget* widget, vtkTable* objectTable, QSharedPointer<QMap<uint, uint> > columnMapping, QColor const & neutralColor, size_t pointsPerObject );
	void show() override;
	void hide();
	void renderSelection(std::vector<size_t> const & sortedSelInds, int classID, QColor const & classColor, QStandardItem* activeClassItem) override;
	void renderSingle(int labelID, int classID, QColor const & classColors, QStandardItem* activeClassItem) override;
	void multiClassRendering(QList<QColor> const & colors, QStandardItem* rootItem, double alpha) override;
	void renderOrientationDistribution(vtkImageData* oi) override;
	void renderLengthDistribution(vtkColorTransferFunction* ctFun, vtkFloatArray* extents, double halfInc, int filterID, double const * range) override;
	void setSelectionOpacity(int selectionAlpha);
	void setContextOpacity(int contextAlpha);
	bool visible() const;
	vtkSmartPointer<vtkActor> getActor();
	virtual vtkPolyData* getPolyData() =0;
	//!  @{ bounding box / bounds
	void showBoundingBox();
	void hideBoundingBox();
	double const * bounds() override;
	//! @}
	virtual void setSelection(std::vector<size_t> const & sortedSelInds, bool selectionActive);
	void setColor(QColor const & color);
	void setLookupTable(QSharedPointer<iALookupTable> lut, size_t paramIndex);
	void updateColorSelectionRendering();
protected:
	vtkSmartPointer<vtkPolyDataMapper> m_mapper;
	vtkSmartPointer<vtkUnsignedCharArray> m_colors;
	vtkSmartPointer<vtkActor> m_actor;
	size_t m_pointsPerObject;
	bool m_visible;
	int m_contextAlpha;
	int m_selectionAlpha;
	QColor m_baseColor;
	QColor m_selectionColor;
	vtkSmartPointer<vtkOutlineFilter> m_outlineFilter;
	vtkSmartPointer<vtkPolyDataMapper> m_outlineMapper;
	vtkSmartPointer<vtkActor> m_outlineActor;
	std::vector<size_t> m_selection;

	void setPolyPointColor(int ptIdx, QColor const & qcolor);
	void updatePolyMapper();
	void setupBoundingBox();
	void setupOriginalIds();

private:
	QSharedPointer<iALookupTable> m_lut;
	size_t m_colorParamIdx;
	bool m_selectionActive;
};
