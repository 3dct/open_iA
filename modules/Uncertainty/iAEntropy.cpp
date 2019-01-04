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
#include "iAEntropy.h"

// in Toolkit/Ensemble
#include <EntropyImageFilter.h>

#include <defines.h>    // for DIM
#include <iAConnector.h>
#include <iATypedCallHelper.h>

iAEntropy::iAEntropy() :
	iAFilter("Entropy", "Uncertainty",
		"Computes the per-voxel entropy, interpreting the given images as a distribution.<br/>"
		"Given a number of input channels or images of same dimensions, this filter "
		"calculates the Entropy by interpreting the voxel values as distribution. "
		"The values for one voxel over all given input images must sum up to 1. "
		"The input must have a floating point pixel type (float/double). "
		"Use for example the probability output of a probabilistic segmentation as input. "
		"<em>Normalize</em> determines whether the output entropy should be normalized "
		"to be in the range [0..1]; if not normalized, the maximum entropy is given "
		"by -log(1/numberOfInputs) (with log = natural logarithm).<br/>"
		"For more information on entropy as a measure of uncertainty, see <a href="
		"\"http://ieeexplore.ieee.org/document/6415481/\">"
		"Visualization of Uncertainty without a Mean</a> by Kristin Potter et al.")
{
	AddParameter("Normalize", Boolean, true);
}

IAFILTER_CREATE(iAEntropy)


template <typename PixelType>
void entropy(iAFilter* filter, QMap<QString, QVariant> const & parameters)
{
	typedef itk::Image<PixelType, DIM> InputImageType;
	typedef fhw::EntropyImageFilter<InputImageType, InputImageType> EntropyFilter;
	auto entropyFilter = EntropyFilter::New();
	for (int i = 0; i < filter->Input().size(); ++i)
	{
		entropyFilter->SetInput(i, dynamic_cast<InputImageType*>(filter->Input()[i]->GetITKImage()));
	}
	entropyFilter->SetNormalize(parameters["Normalize"].toBool());
	entropyFilter->Update();
	filter->AddOutput(entropyFilter->GetOutput());
}


void iAEntropy::PerformWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(entropy, InputPixelType(), this, parameters);
}
