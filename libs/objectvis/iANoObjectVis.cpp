// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iANoObjectVis.h"

iANoObjectVis::iANoObjectVis(): iAObjectVis(std::shared_ptr<iAObjectsData>())
{
	std::fill(m_dummyBounds, m_dummyBounds + 3, 0);
	std::fill(m_dummyBounds +3, m_dummyBounds + 6, 1);
}

void iANoObjectVis::renderSelection(std::vector<size_t> const & /*sortedSelInds*/, int /*classID*/, QColor const & /*classColor*/, QStandardItem* /*activeClassItem*/)
{}

void iANoObjectVis::renderSingle(IndexType /*selectedObjID*/, int /*classID*/, QColor const & /*classColor*/, QStandardItem* /*activeClassItem*/)
{}

void iANoObjectVis::multiClassRendering(QList<QColor> const & /*classColors*/, QStandardItem* /*rootItem*/, double /*alpha*/)
{}

void iANoObjectVis::renderOrientationDistribution(vtkImageData* /*oi*/)
{}

void iANoObjectVis::renderLengthDistribution(vtkColorTransferFunction* /*cTFun*/, vtkFloatArray* /*extents*/, double /*halfInc*/, int /*filterID*/, double const * /*range*/)
{}

double const * iANoObjectVis::bounds()
{
	return m_dummyBounds;
}

std::shared_ptr<iAObjectVisActor> iANoObjectVis::createActor(vtkRenderer* ren)
{
	return std::make_shared<iAObjectVisActor>(ren);
}
