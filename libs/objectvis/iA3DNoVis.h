// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iA3DObjectVis.h"

class iAobjectvis_API iA3DNoVis : public iA3DObjectVis
{
public:
	iA3DNoVis();
	void renderSelection(std::vector<size_t> const & sortedSelInds, int classID, QColor const & classColor, QStandardItem* activeClassItem) override;
	void renderSingle(IndexType selectedObjID, int classID, QColor const & classColor, QStandardItem* activeClassItem) override;
	void multiClassRendering(QList<QColor> const & classColors, QStandardItem* rootItem, double alpha) override;
	void renderOrientationDistribution(vtkImageData* oi) override;
	void renderLengthDistribution(vtkColorTransferFunction* cTFun, vtkFloatArray* extents, double halfInc, int filterID, double const * range) override;
	double const * bounds() override;
	QSharedPointer<iA3DObjectActor> createActor(vtkRenderer* ren) override;

private:
	double m_dummyBounds[6];
};
