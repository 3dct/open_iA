// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAEntropy.h"

// in Toolkit/Ensemble
#include <iAEntropyImageFilter.h>

#include <defines.h>    // for DIM
#include <iAImageData.h>
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
	addParameter("Normalize", iAValueType::Boolean, true);
}


template <typename PixelType>
void entropy(iAFilter* filter, QVariantMap const & parameters)
{
	typedef itk::Image<PixelType, DIM> InputImageType;
	typedef iAEntropyImageFilter<InputImageType, InputImageType> EntropyFilter;
	auto entropyFilter = EntropyFilter::New();
	for (size_t i = 0; i < filter->inputCount(); ++i)
	{
		entropyFilter->SetInput(i, dynamic_cast<InputImageType*>(filter->imageInput(i)->itkImage()));
	}
	entropyFilter->SetNormalize(parameters["Normalize"].toBool());
	entropyFilter->Update();
	filter->addOutput(std::make_shared<iAImageData>(entropyFilter->GetOutput()));
}


void iAEntropy::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(entropy, inputScalarType(), this, parameters);
}
