// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAObjectVis.h"

//! A "dummy" visualization for objects given in a table - no visualization at all (null object pattern).
class iAobjectvis_API iANoObjectVis : public iAObjectVis
{
public:
	iANoObjectVis();
	void renderSelection(std::vector<size_t> const & sortedSelInds, int classID, QColor const & classColor, QStandardItem* activeClassItem) override;
	void renderSingle(IndexType selectedObjID, int classID, QColor const & classColor, QStandardItem* activeClassItem) override;
	void multiClassRendering(QList<QColor> const & classColors, QStandardItem* rootItem, double alpha) override;
	void renderOrientationDistribution(vtkImageData* oi) override;
	void renderLengthDistribution(vtkColorTransferFunction* cTFun, vtkFloatArray* extents, double halfInc, int filterID, double const * range) override;
	double const * bounds() override;
	QSharedPointer<iAObjectVisActor> createActor(vtkRenderer* ren) override;

private:
	double m_dummyBounds[6];
};
