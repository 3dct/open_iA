// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iA3DNoVis.h"

iA3DNoVis::iA3DNoVis(): iA3DObjectVis(nullptr, QSharedPointer<QMap<uint,uint>>())
{
	std::fill(m_dummyBounds, m_dummyBounds + 3, 0);
	std::fill(m_dummyBounds +3, m_dummyBounds + 6, 1);
}

void iA3DNoVis::renderSelection(std::vector<size_t> const & /*sortedSelInds*/, int /*classID*/, QColor const & /*classColor*/, QStandardItem* /*activeClassItem*/)
{}

void iA3DNoVis::renderSingle(IndexType /*selectedObjID*/, int /*classID*/, QColor const & /*classColor*/, QStandardItem* /*activeClassItem*/)
{}

void iA3DNoVis::multiClassRendering(QList<QColor> const & /*classColors*/, QStandardItem* /*rootItem*/, double /*alpha*/)
{}

void iA3DNoVis::renderOrientationDistribution(vtkImageData* /*oi*/)
{}

void iA3DNoVis::renderLengthDistribution(vtkColorTransferFunction* /*cTFun*/, vtkFloatArray* /*extents*/, double /*halfInc*/, int /*filterID*/, double const * /*range*/)
{}

double const * iA3DNoVis::bounds()
{
	return m_dummyBounds;
}

QSharedPointer<iA3DObjectActor> iA3DNoVis::createActor(vtkRenderer* ren)
{
	return QSharedPointer<iA3DObjectActor>::create(ren);
}
