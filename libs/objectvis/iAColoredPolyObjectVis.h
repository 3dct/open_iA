// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAObjectVis.h"

#include <vtkSmartPointer.h>

#include <QColor>

class iAPolyObjectVisActor;
class iALookupTable;

class vtkAlgorithmOutput;
class vtkPolyData;
class vtkUnsignedCharArray;

//! Base class for polydata-based object visualizations that can be colored.
class iAobjectvis_API iAColoredPolyObjectVis : public iAObjectVis
{
public:
	static const int DefaultContextOpacity = 8;
	static const int DefaultSelectionOpacity = 128;
	iAColoredPolyObjectVis(std::shared_ptr<iAObjectsData> data, QColor const & neutralColor);

	//! @{ "legacy" methods for various selection/coloring options, specific to FeatureScout module
	void renderSelection(std::vector<size_t> const & sortedSelInds, int classID, QColor const & classColor, QStandardItem* activeClassItem) override;
	void renderSingle(IndexType selectedObjID, int classID, QColor const & classColors, QStandardItem* activeClassItem) override;
	void multiClassRendering(QList<QColor> const & colors, QStandardItem* rootItem, double alpha) override;
	void renderOrientationDistribution(vtkImageData* oi) override;
	void renderLengthDistribution(vtkColorTransferFunction* ctFun, vtkFloatArray* extents, double halfInc, int filterID, double const * range) override;
	//! @}
	void setSelectionOpacity(int selectionAlpha);
	void setContextOpacity(int contextAlpha);
	virtual vtkPolyData* polyData() = 0;
	virtual vtkPolyData* finalPolyData() = 0;

	double const * bounds() override;

	virtual void setSelection(std::vector<size_t> const & sortedSelInds, bool selectionActive);
	void setColor(QColor const & color);
	void setLookupTable(std::shared_ptr<iALookupTable> lut, size_t paramIndex);
	void updateColorSelectionRendering();
	virtual QString visualizationStatistics() const =0;
	//! extract one mesh per selected object
	virtual std::vector<vtkSmartPointer<vtkPolyData>> extractSelectedObjects(QColor c = QColor()) const = 0;
	//! Get the index of the first point of a given object.
	//! @param objIdx the index of the object.
	//! @return the index of the first point in the object.
	virtual IndexType objectStartPointIdx(IndexType objIdx) const;
	//! Get the number of points representing a given object.
	//! @param objIdx the index of the object.
	//! @return the number of points in the object.
	virtual IndexType objectPointCount(IndexType objIdx) const;
	//! Get the number of points in all objects.
	//! @return the number of points in all objects, i.e. the sum of objectPointCount over all object indices.
	IndexType allPointCount() const;

	//! Get the index of the first point of a given final object.
	//! @param objIdx the index of the object.
	//! @return the index of the first point in the final object.
	virtual IndexType finalObjectStartPointIdx(IndexType objIdx) const;
	//! Get the number of points representing a given final object.
	//! @param objIdx the index of the object.
	//! @return the number of points in the final object.
	virtual IndexType finalObjectPointCount(IndexType objIdx) const;
	//! Get the number of points in all final objects.
	//! @return the number of points in all objects, i.e. the sum of objectPointCount over all object indices.
	IndexType finalAllPointCount() const;

	//virtual vtkAlgorithmOutput* output();

	//! create "actor" class for visualizing this data collection
	std::shared_ptr<iAObjectVisActor> createActor(vtkRenderer* ren) override;

	//! same as createActor, but retrieve derived class more specific for visualizing
	//! a 3D colored poly data object; use this if you need to access methods
	//! from the iAPolyObjectVisActorclass which are not available through the
	//! iAObjectVisActor interface.
	std::shared_ptr<iAPolyObjectVisActor> createPolyActor(vtkRenderer* ren);

	std::vector<size_t> const& selection() const;

protected:
	vtkSmartPointer<vtkUnsignedCharArray> m_colors;
	int m_contextAlpha;
	int m_selectionAlpha;
	QColor m_baseColor;
	QColor m_selectionColor;
	std::vector<size_t> m_selection;

	//! Set an object to a specified color.
	//! @param objIdx index of the object.
	//! @param qcolor new color of the object.
	void setObjectColor(IndexType objIdx, QColor const & qcolor);
	//! Set up the mapping from object parts to object IDs.
	void setupOriginalIds();
	//! Set up the array of colors for each object.
	void setupColors();

private:

	std::shared_ptr<iALookupTable> m_lut;
	IndexType m_colorParamIdx;
	bool m_selectionActive;

	const IndexType DefaultPointsPerObject = 2;
};
