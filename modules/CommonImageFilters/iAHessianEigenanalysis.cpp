/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
#include "iAHessianEigenanalysis.h"

#include <defines.h>          // for DIM
#include <iAConnector.h>
#include <iAPixelAccessors.h>
#include <iAProgress.h>
#include <iATypedCallHelper.h>

#include <itkCastImageFilter.h>
#include <itkHessianRecursiveGaussianImageFilter.h>
#include <itkImageAdaptor.h>
#include <itkLaplacianRecursiveGaussianImageFilter.h>
#include <itkLaplacianImageFilter.h>
#include <itkSymmetricEigenAnalysisImageFilter.h>

template<class T> void hessianEigenAnalysis(iAFilter* filter, QMap<QString, QVariant> const & parameters)
{
	//typedef itk::Vector<double, 3> VectorPixelType;
	//VectorPixelType eigenTempVector;
	typedef itk::Image< T, 3 > InputImageType;
	typedef	itk::HessianRecursiveGaussianImageFilter<InputImageType >	HessianFilterType;
	typedef	typename HessianFilterType::OutputImageType	HessianImageType;

	typedef	typename HessianImageType::PixelType HessianPixelType;
	typedef	itk::FixedArray< double, HessianPixelType::Dimension > EigenValueArrayType;
	typedef	itk::Image< EigenValueArrayType, HessianImageType::ImageDimension > EigenValueImageType;
	typedef	itk::SymmetricEigenAnalysisImageFilter< HessianImageType, EigenValueImageType > EigenAnalysisFilterType;

	typedef	float EigenValuePixelType;
	typedef itk::ImageAdaptor<  EigenValueImageType, iAEigenValueAccessor< EigenValueArrayType > > ImageAdaptorType;
	typedef itk::Image< EigenValuePixelType, DIM > EachEigenValueImageType;
	typedef itk::CastImageFilter< ImageAdaptorType, EachEigenValueImageType >  CastImageFilterType;

	// Compute Hessian
	auto hessianFilter = HessianFilterType::New();
	hessianFilter->SetInput( dynamic_cast< InputImageType * >( filter->input()[0]->itkImage() ) );
	hessianFilter->SetSigma(parameters["Sigma"].toDouble());
	filter->progress()->observe(hessianFilter);
	hessianFilter->Update();

	// Compute eigen values, order them in ascending order
	auto eigenFilter = EigenAnalysisFilterType::New();
	eigenFilter->SetDimension( HessianPixelType::Dimension );
	eigenFilter->SetInput( hessianFilter->GetOutput() );
	//auto hessianImage = hessianFilter->GetOutput();
	eigenFilter->OrderEigenValuesBy( EigenAnalysisFilterType::FunctorType::OrderByValue );

	// Eigen analysis
	// Conversion or extraction of data
	//    from class itk::Image<class itk::FixedArray<double,3>,3> *
	//    to class itk::Image<float,3> *

	// Create an adaptor and plug the output to the parametric space
	auto eigenAdaptor1 = ImageAdaptorType::New();
	iAEigenValueAccessor< EigenValueArrayType > accessor1;
	accessor1.SetEigenIdx( 0 );
	eigenAdaptor1->SetImage( eigenFilter->GetOutput() );
	eigenAdaptor1->SetPixelAccessor( accessor1 );

	auto eigenAdaptor2 = ImageAdaptorType::New();
	iAEigenValueAccessor< EigenValueArrayType > accessor2;
	accessor2.SetEigenIdx( 1 );
	eigenAdaptor2->SetImage( eigenFilter->GetOutput() );
	eigenAdaptor2->SetPixelAccessor( accessor2 );

	auto eigenAdaptor3 = ImageAdaptorType::New();
	iAEigenValueAccessor< EigenValueArrayType > accessor3;
	accessor3.SetEigenIdx( 2 );
	eigenAdaptor3->SetImage( eigenFilter->GetOutput() );
	eigenAdaptor3->SetPixelAccessor( accessor3 );

	// m_EigenCastfilter1 will give the eigen values with the maximum eigen
	// value. m_EigenCastfilter3 will give the eigen values with the
	// minimum eigen value.
	auto eigenCastfilter1 = CastImageFilterType::New();
	eigenCastfilter1->SetInput( eigenAdaptor3 );
	eigenCastfilter1->Update();
	filter->addOutput(eigenCastfilter1->GetOutput());

	auto eigenCastfilter2 = CastImageFilterType::New();
	eigenCastfilter2->SetInput( eigenAdaptor2 );
	eigenCastfilter2->Update();
	filter->addOutput(eigenCastfilter2->GetOutput());

	auto eigenCastfilter3 = CastImageFilterType::New();
	eigenCastfilter3->SetInput( eigenAdaptor1 );
	eigenCastfilter3->Update();
	filter->addOutput(eigenCastfilter3->GetOutput());

/*
	// TODO: check if the following code in its current form does anything useful
	// Iteration through eigenvalues to compute Ra, Rb and S values
	auto eigenImage = eigenFilter->GetOutput();
	auto eigenRaRbS = eigenFilter->GetOutput(); // this is the same as eigenImage, right?
	typedef itk::ImageRegionIterator<EigenValueImageType> EigenIteratorType;
	EigenIteratorType eigenRaRbSIt(eigenRaRbS, eigenRaRbS->GetLargestPossibleRegion());
	EigenIteratorType eigenImageIt(eigenImage, eigenImage->GetLargestPossibleRegion());

	// iterate through image and get each eigen value
	//unsigned int j=0;
	eigenRaRbSIt.GoToBegin();
	for (eigenImageIt.GoToBegin(); !eigenImageIt.IsAtEnd() && !eigenRaRbSIt.IsAtEnd(); ++eigenImageIt)
	{
		//DEBUG_LOG(QString("%1 -> ").arg(j));
		EigenValueArrayType eigenArray = eigenImageIt.Get();

		eigenTempVector[0] = fabs(eigenArray[1])/fabs(eigenArray[2]);
		eigenTempVector[1] = fabs(eigenArray[0])/(sqrt(fabs(eigenArray[1]*eigenArray[2])));
		eigenTempVector[2] = sqrt(pow(eigenArray[0],2)+pow(eigenArray[1],2)+pow(eigenArray[2],2));

		eigenRaRbSIt.Set(eigenTempVector);
		++eigenRaRbSIt;
		//++j;
	}
*/
}

void iAHessianEigenanalysis::performWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(hessianEigenAnalysis, inputPixelType(), this, parameters);
}

IAFILTER_CREATE(iAHessianEigenanalysis)

iAHessianEigenanalysis::iAHessianEigenanalysis() :
	iAFilter("Eigen analysis of Hessian", "Hessian and Eigenanalysis",
		"Computes the Eigen analysis of the Hessian of an image.<br/>"
		"Computes first the Hessian of an image, and then the eigen analysis "
		"of the Hessian, and outputs the three lambda images from this eigen"
		"analysis.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1HessianRecursiveGaussianImageFilter.html\">"
		"Hessian Recursive Gaussian Filter</a> and the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1SymmetricEigenAnalysisImageFilter.html\">"
		"Symmetric Eigen Analysis Filter</a> in the ITK documentation.", 1, 3)
{
	addParameter("Sigma", Continuous, 1.0);
}



template<class T> void Laplacian(iAFilter* filter, QMap<QString, QVariant> const & params)
{
	typedef itk::Image< T, DIM > ImageType;
	typedef itk::Image<float, DIM> OutputImageType;
	typedef itk::LaplacianRecursiveGaussianImageFilter<ImageType, OutputImageType> LoGFilterType;

	auto logFilter = LoGFilterType::New();
	logFilter->SetInput(dynamic_cast< ImageType * >(filter->input()[0]->itkImage()));
	logFilter->SetSigma(params["Sigma"].toDouble());
	logFilter->Update();
	filter->addOutput(logFilter->GetOutput());
}

void iALaplacian::performWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(Laplacian, inputPixelType(), this, parameters);
}

IAFILTER_CREATE(iALaplacian)

iALaplacian::iALaplacian() :
	iAFilter("Laplacian of Gaussian", "Hessian and Eigenanalysis",
		"Computes the Laplacian of Gaussian (LoG) of an image.<br/>"
		"Computes the Laplacian of Gaussian (LoG) of an image by convolution "
		"with the second derivative of a Gaussian. This filter is "
		"implemented using the recursive gaussian filters.<em>Sigma</em> is "
		"measured in the units of image spacing.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1LaplacianRecursiveGaussianImageFilter.html\">"
		"Laplacian Recursive Gaussian Filter</a> in the ITK documentation.")
{
	addParameter("Sigma", Continuous, 1.0);
}
