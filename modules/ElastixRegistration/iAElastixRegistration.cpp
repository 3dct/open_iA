#include "iAElastixRegistration.h"

#include "defines.h" // for DIM
#include "iAConnector.h"
#include "iAProgress.h"
#include "iATypedCallHelper.h"
#include <itkDerivativeImageFilter.h>

#include "elastixlib.h"
#include "transformixlib.h"
#include "itkParameterFileParser.h"
#include "itkImage.h"



template<class T> 
void derivative(iAFilter* filter, QMap<QString, QVariant> const & params)
{
	//typedef itk::Image<T, DIM> InputImageType;
	//typedef itk::Image<float, DIM> RealImageType;
	//typedef itk::DerivativeImageFilter< InputImageType, RealImageType > DIFType;

	//auto derFilter = DIFType::New();
	//derFilter->SetOrder(params["Order"].toUInt());
	//derFilter->SetDirection(params["Direction"].toUInt());
	//derFilter->SetInput( dynamic_cast< InputImageType * >(filter->input()[0]->itkImage()) );
	//filter->progress()->observe( derFilter );
	//derFilter->Update();
	//filter->addOutput(derFilter->GetOutput());


	using namespace elastix;
	typedef ELASTIX::ParameterMapType RegistrationParametersType;
	typedef itk::ParameterFileParser ParserType;


	// Create parser for transform parameters text file.
	ParserType::Pointer file_parser = ParserType::New();
	// Try parsing transform parameters text file.
	file_parser->SetParameterFileName(params["ParameterFile"].toString().toStdString());
	try
	{
		file_parser->ReadParameterFile();
	}
	catch (itk::ExceptionObject & e)
	{
		std::cout << e.what() << std::endl;
		// Do some error handling!
	}
	// Retrieve parameter settings as map.
	RegistrationParametersType parameters = file_parser->GetParameterMap();



	typedef itk::Image<T, DIM> ITKImageType;
	typedef std::vector<RegistrationParametersType> RegistrationParametersContainerType;

	ELASTIX* elastix = new ELASTIX();
	int error = 0;
	try
	{
		error = elastix->RegisterImages(
			dynamic_cast<ITKImageType*>(filter->input()[0]->itkImage()),
			dynamic_cast<ITKImageType*>(filter->input()[1]->itkImage()),
			parameters, // Parameter map read in previous code
			"C:\\temp\\LOG", // Directory where output is written, if enabled
			TRUE, // Enable/disable writing of elastix.log
			TRUE, // Enable/disable output to console
			nullptr, // Provide fixed image mask (optional, 0 = no mask)
			nullptr // Provide moving image mask (optional, 0 = no mask)
		);
	}
	catch (itk::ExceptionObject &err)
	{
		// Do some error handling.
	}


	ITKImageType * output_image;
	RegistrationParametersContainerType transform_parameters;


	if (error == 0)
	{
		if (elastix->GetResultImage().IsNotNull())
		{
			// Typedef the ITKImageType first...
			output_image = static_cast<ITKImageType *>(
				elastix->GetResultImage().GetPointer());
			filter->addOutput(output_image);
		}
		else
		{
			// Registration failure. Do some error handling.
		}
		// Get transform parameters of all registration steps.
		transform_parameters = elastix->GetTransformParameterMapList();
		// Clean up memory.
		delete elastix;
	}

	using namespace transformix;
	TRANSFORMIX* transformix = new TRANSFORMIX();
	//int error = 0;
	try
	{
		error = transformix->TransformImage(
			static_cast<typename itk::DataObject::Pointer>(output_image),
			transform_parameters, // Parameters resulting from elastix run
			"C:\\temp\\LOG", // Enable/disable writing of transformix.log
			TRUE,TRUE); // Enable/disable output to console
	}
	catch (itk::ExceptionObject &err)
	{
			// Do some error handling.
	}
	if (error == 0)
	{
		// Typedef the ITKImageType first...
		ITKImageType * output_image = static_cast<ITKImageType *>(
			transformix->GetResultImage().GetPointer());
		filter->addOutput(output_image);
	}
	else
	{
		// Do some error handling.
	}
	// Clean up memory.
	delete transformix;


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
		"Elastix Registration</a> in the ITK documentation.",2,1)
{
	addParameter("ParameterFile", FileNameOpen);
	addParameter("Direction", Discrete, 0, 0, DIM-1);
}