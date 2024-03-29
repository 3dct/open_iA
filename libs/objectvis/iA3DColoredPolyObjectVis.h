// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iA3DObjectVis.h"

#include <vtkSmartPointer.h>

#include <QColor>

class iA3DPolyObjectActor;
class iALookupTable;

class vtkAlgorithmOutput;
class vtkPolyData;
class vtkUnsignedCharArray;

class iAobjectvis_API iA3DColoredPolyObjectVis : public iA3DObjectVis
{
public:
	static const int DefaultContextOpacity = 8;
	static const int DefaultSelectionOpacity = 128;
	iA3DColoredPolyObjectVis(vtkTable* objectTable, QSharedPointer<QMap<uint, uint> > columnMapping, QColor const & neutralColor);

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
	void setLookupTable(QSharedPointer<iALookupTable> lut, size_t paramIndex);
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
	QSharedPointer<iA3DObjectActor> createActor(vtkRenderer* ren) override;

	//! same as createActor, but retrieve derived class more specific for visualizing
	//! a 3D colored poly data object; use this if you need to access methods
	//! from the iA3DPolyObjectActor class which are not available through the
	//! iA3DObjectActor interface.
	QSharedPointer<iA3DPolyObjectActor> createPolyActor(vtkRenderer* ren);

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

	QSharedPointer<iALookupTable> m_lut;
	IndexType m_colorParamIdx;
	bool m_selectionActive;

	const IndexType DefaultPointsPerObject = 2;
};
