#include "iAElastixRegistration.h"

#include "defines.h" // for DIM
#include "iAConnector.h"
#include "iAProgress.h"
#include "iATypedCallHelper.h"

#include <itkDerivativeImageFilter.h>

template<class T> 
void derivative(iAFilter* filter, QMap<QString, QVariant> const & params)
{
	typedef itk::Image<T, DIM> InputImageType;
	typedef itk::Image<float, DIM> RealImageType;
	typedef itk::DerivativeImageFilter< InputImageType, RealImageType > DIFType;

	auto derFilter = DIFType::New();
	derFilter->SetOrder(params["Order"].toUInt());
	derFilter->SetDirection(params["Direction"].toUInt());
	derFilter->SetInput( dynamic_cast< InputImageType * >(filter->input()[0]->itkImage()) );
	filter->progress()->observe( derFilter );
	derFilter->Update();
	filter->addOutput(derFilter->GetOutput());
}

void iAElastixRegistration::performWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(derivative, inputPixelType(), this, parameters);
}

IAFILTER_CREATE(iAElastixRegistration)

iAElastixRegistration::iAElastixRegistration() :
	iAFilter("Elastix Registration", "Registration",
		"Computes the directional derivative for each image element.<br/>"
		"The <em>order</em> of the derivative can be specified, as well as the desired <em>direction</em> (0=x, 1=y, 2=z).<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1DerivativeImageFilter.html\">"
		"Elastix Registration</a> in the ITK documentation.")
{
	addParameter("Order", Discrete, 1, 1);
	addParameter("Direction", Discrete, 0, 0, DIM-1);
}