// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAObjectVis.h"

class iATransferFunction;

class vtkPiecewiseFunction;
class vtkColorTransferFunction;

//! Visualizes the objects given in a table by referencing them in a labeled volume.
//!
//! This class basically manages the transfer functions used for coloring specific
//! object IDs when these are selected and for hiding the other objects.
//! Requires column mappings for length, phi and theta orientation for the distribution renderings.
class iAobjectvis_API iALabeledVolumeVis : public iAObjectVis
{
public:
	//! Create a labeled object visualization
	//! @param data the table with the characteristics of the objects
	//! @param color color transfer function used for displaying the labeled volume dataset (will be modified to reflect selections, colorings, etc.)
	//! @param opac opacity transfer function used for displaying the labeled volume dataset (will be modified to reflect selections, colorings, etc.)
	//! @param bounds bounds of the labeled volume dataset (used to be able to report the bounds of this visualization)
	iALabeledVolumeVis(iATransferFunction* tf, iAObjectsData const * data, double const * bounds );
	void renderSelection(std::vector<size_t> const & sortedSelInds, int classID, QColor const & classColor, QStandardItem* activeClassItem ) override;
	void renderSingle(IndexType selectedObjID, int classID, QColor const & classColor, QStandardItem* activeClassItem ) override;
	void multiClassRendering(QList<QColor> const & classColors, QStandardItem* rootItem, double alpha ) override;
	void renderOrientationDistribution(vtkImageData* oi ) override;
	void renderLengthDistribution(vtkColorTransferFunction* ctFun, vtkFloatArray* extents, double halfInc, int filterID, double const * range ) override;
	double const * bounds() override;
	std::shared_ptr<iAObjectVisActor> createActor(vtkRenderer* ren) override;

private:
	iATransferFunction* m_tf;
	double m_bounds[6];
};
