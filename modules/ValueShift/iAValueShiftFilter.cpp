#include "iAValueShiftFilter.h"

#include "defines.h" // for DIM
#include "iAConnector.h"
#include "iAProgress.h"
#include "iATypedCallHelper.h"

#include <itkDerivativeImageFilter.h>

#include <itkImage.h>
#include <itkImageIterator.h>
#include <itkImageRegionIterator.h>

template<class T> 
void derivative(iAFilter* filter, QMap<QString, QVariant> const & params)
{

	using InputImageType = itk::Image<T, DIM> ;
	using OutputImageType = itk::Image<float, DIM> ;
	typedef itk::ImageRegionIterator<InputImageType> InImageIterator;
	typedef itk::ImageRegionIterator<OutputImageType> OutImageIterator;

	auto im = dynamic_cast<InputImageType*>(filter->input()[0]->itkImage());
	typename InputImageType::RegionType region = im->GetLargestPossibleRegion();


	typename OutputImageType::Pointer imgOut = OutputImageType::New();
	imgOut->SetRegions(region);
	imgOut->Allocate();
	imgOut->FillBuffer(0);


	InImageIterator it(im, im->GetRequestedRegion());
	OutImageIterator itOut(imgOut, imgOut->GetRequestedRegion());

	auto valueToReplace = params["ValueToReplace"].toInt();
	auto replaceValue = params["Replace"].toInt();

	for (it.GoToBegin(); !it.IsAtEnd(); ++it)
	{
		if ((int)it.Value() == valueToReplace)
		{
			itOut.SetIndex(it.GetIndex())  ;
			itOut.Set(0.0);
		}
		else if ((int)it.Value() > replaceValue)
		{
			itOut.SetIndex(it.GetIndex()) ;
			auto value = it.Value();
			itOut.Set(value + 1.0);
		}
		
	}
	filter->addOutput(imgOut);
}

void iAValueShiftFilter::performWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(derivative, input()[0]->itkScalarPixelType(), this, parameters);
}

IAFILTER_CREATE(iAValueShiftFilter)

iAValueShiftFilter::iAValueShiftFilter() :
	iAFilter("Value Shift", "Intensity",
		"Computes the directional derivative for each image element.<br/>"
		"The <em>order</em> of the derivative can be specified, as well as the desired <em>direction</em> (0=x, 1=y, 2=z).<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1DerivativeImageFilter.html\">"
		"Derivative Filter</a> in the ITK documentation.",1,1)
{
	addParameter("ValueToReplace", iAValueType::Discrete, 0, 0,65535);
	addParameter("Replace", iAValueType::Discrete, 0, 0, 65535);
}