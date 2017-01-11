/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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
 
#include "pch.h"
#include "iAHessianEigenanalysis.h"

#include "iAConnector.h"
#include "iAProgress.h"
#include "iATypedCallHelper.h"

#include <itkDerivativeImageFilter.h>
#include <itkLaplacianRecursiveGaussianImageFilter.h>
#include <itkLaplacianSegmentationLevelSetFunction.h>
#include <itkLaplacianImageFilter.h>
#include <itkZeroCrossingImageFilter.h>

#include <vtkImageData.h>

#include <QLocale>

/**
* template computeHessian
* 
* This template is used for calculating the hessian matrix
* \param	sigma			Sigma
* \param	hessianComputed Is the hessian matrix already computed.
* \param	p				Filter progress information. 
* \param	image			Input image. 
* \param	T				Input type 
* \return	int Status-Code. 
*/
template<class T> int computeHessian_template( int sigma, bool hessianComputed, int nr, iAProgress* p, iAConnector* image)
{
	/************************************** Data and type definitions *********************/
	typedef itk::Vector<double, 3>	VectorPixelType;
	VectorPixelType eigenTempVector;
	typedef itk::Image< T, 3 >	InputImageType;
	typedef	itk::HessianRecursiveGaussianImageFilter<InputImageType >	HessianFilterType;
	typedef	typename HessianFilterType::OutputImageType	HessianImageType;

	typedef	typename HessianImageType::PixelType HessianPixelType;
	typedef	itk::FixedArray< double, HessianPixelType::Dimension > EigenValueArrayType;
	typedef	itk::Image< EigenValueArrayType, HessianImageType::ImageDimension > EigenValueImageType;
	typedef	itk::SymmetricEigenAnalysisImageFilter< HessianImageType, EigenValueImageType >     EigenAnalysisFilterType;

	itkStaticConstMacro(Dimension,unsigned int,3);
	typedef	float	MeshPixelType;
	typedef itk::ImageAdaptor<  EigenValueImageType, EigenValueAccessor< EigenValueArrayType > > ImageAdaptorType;
	typedef itk::Image< MeshPixelType, Dimension > EachEigenValueImageType;
	typedef itk::CastImageFilter< ImageAdaptorType, EachEigenValueImageType >  CastImageFilterType;
	/************************************** Data and type definitions end *****************/

	/************************************** Hessian part **********************************/
	typename HessianFilterType::Pointer	m_Hessian;	// In m_Hessian werden die Matrizen f�r jedes Voxel gepsichert. MA
	m_Hessian = HessianFilterType::New();	// Objekt f�r die Matrizen deklariert. MA

	m_Hessian->SetInput( dynamic_cast< InputImageType * >( image->GetITKImage() ) );
	m_Hessian->SetSigma(sigma);
	if(!hessianComputed)
		m_Hessian->Update();
	/************************************** Hessian part end ******************************/

	/************************************** Eigen values part *****************************/
	// Compute eigen values.. order them in ascending order
	typename EigenAnalysisFilterType::Pointer m_EigenFilter;

	m_EigenFilter = EigenAnalysisFilterType::New();
	m_EigenFilter->SetDimension( HessianPixelType::Dimension );
	m_EigenFilter->SetInput( m_Hessian->GetOutput() );
	typename HessianImageType::Pointer hessianImage = m_Hessian->GetOutput();
	m_EigenFilter->OrderEigenValuesBy( EigenAnalysisFilterType::FunctorType::OrderByValue );
	/************************************** Eigen values part end **********************************/

	/************************************** Eigen analysis **********************************/
	// Conversion or extraction of data from class itk::Image<class itk::FixedArray<double,3>,3> * __ptr64 to class itk::Image<float,3> * __ptr64
	typename ImageAdaptorType::Pointer               m_EigenAdaptor1;
	typename ImageAdaptorType::Pointer               m_EigenAdaptor2;
	typename ImageAdaptorType::Pointer               m_EigenAdaptor3;

	typename CastImageFilterType::Pointer            m_EigenCastfilter1;
	typename CastImageFilterType::Pointer            m_EigenCastfilter2;
	typename CastImageFilterType::Pointer            m_EigenCastfilter3;

	typename EigenValueImageType::Pointer eigenImage1;

	// Create an adaptor and plug the output to the parametric space
	m_EigenAdaptor1 = ImageAdaptorType::New();
	EigenValueAccessor< EigenValueArrayType > accessor1;
	accessor1.SetEigenIdx( 0 );
	m_EigenAdaptor1->SetImage( m_EigenFilter->GetOutput() );
	m_EigenAdaptor1->SetPixelAccessor( accessor1 );

	m_EigenAdaptor2 = ImageAdaptorType::New();
	EigenValueAccessor< EigenValueArrayType > accessor2;
	accessor2.SetEigenIdx( 1 );
	m_EigenAdaptor2->SetImage( m_EigenFilter->GetOutput() );
	m_EigenAdaptor2->SetPixelAccessor( accessor2 );

	m_EigenAdaptor3 = ImageAdaptorType::New();
	EigenValueAccessor< EigenValueArrayType > accessor3;
	accessor3.SetEigenIdx( 2 );
	m_EigenAdaptor3->SetImage( m_EigenFilter->GetOutput() );
	m_EigenAdaptor3->SetPixelAccessor( accessor3 );

	eigenImage1 = m_EigenFilter->GetOutput();
	typename EigenValueImageType::Pointer eigenRaRbS = m_EigenFilter->GetOutput();

	// m_EigenCastfilter1 will give the eigen values with the maximum eigen
	// value. m_EigenCastfilter3 will give the eigen values with the 
	// minimum eigen value.
	m_EigenCastfilter1 = CastImageFilterType::New();
	m_EigenCastfilter1->SetInput( m_EigenAdaptor3 );
	m_EigenCastfilter2 = CastImageFilterType::New();
	m_EigenCastfilter2->SetInput( m_EigenAdaptor2 );
	m_EigenCastfilter3 = CastImageFilterType::New();
	m_EigenCastfilter3->SetInput( m_EigenAdaptor1 );
	/************************************** Eigen analysis part end **********************************/


	/************************************** Define output **********************************/
	if(nr==1 || nr==0){
		p->Observe( m_EigenCastfilter1 );
		m_EigenCastfilter1->Update();
		image->SetImage( m_EigenCastfilter1->GetOutput() );
		m_EigenCastfilter1->ReleaseDataFlagOn();
	}
	else if(nr==2){
		p->Observe( m_EigenCastfilter2 );
		m_EigenCastfilter2->Update();
		image->SetImage( m_EigenCastfilter2->GetOutput() );
		m_EigenCastfilter2->ReleaseDataFlagOn();
	}
	else if(nr==3){
		p->Observe( m_EigenCastfilter3 );
		m_EigenCastfilter3->Update();
		image->SetImage( m_EigenCastfilter3->GetOutput() );
		m_EigenCastfilter3->ReleaseDataFlagOn();
	}
	/************************************** Define output end **********************************/
	image->Modified();

	/************************************** Iteration through eigenvalues to compute Ra, Rb and S values **********************************/
	typedef itk::ImageRegionIterator<EigenValueImageType> EigenIteratorType;
	EigenIteratorType eigenImageIterator(eigenRaRbS,eigenRaRbS->GetLargestPossibleRegion());
	EigenIteratorType eigenImageIt(eigenImage1,eigenImage1->GetLargestPossibleRegion());

	// iterate through image and get each eigen value
	int j=0;
	eigenImageIterator.GoToBegin();
	for (eigenImageIt.GoToBegin(); !eigenImageIt.IsAtEnd() && !eigenImageIterator.IsAtEnd(); ++eigenImageIt)
	{
		std::cout << j << " -> " ;
		EigenValueArrayType eigenArray = eigenImageIt.Get();

		eigenTempVector[0] = fabs(eigenArray[1])/fabs(eigenArray[2]);
		eigenTempVector[1] = fabs(eigenArray[0])/(sqrt(fabs(eigenArray[1]*eigenArray[2])));
		eigenTempVector[2] = sqrt(pow(eigenArray[0],2)+pow(eigenArray[1],2)+pow(eigenArray[2],2));

		eigenImageIterator.Set(eigenTempVector);
		++eigenImageIterator;
		j++;
	}
	/************************************** Iteration through eigenvalues to compute Ra, Rb and S values end **********************************/

	return EXIT_SUCCESS;
}



/**
* template computeLaplacian
* This template is used for calculating the Laplacian of Gaussian (LoG)
* \param	sigma			Sigma
* \param	p				Filter progress information.
* \param	image			Input image.
* \param	T				Input type
* \return	int Status-Code.
*/

template<class T> int computeLaplacian_template(unsigned int sigma, iAProgress* p, iAConnector* image)
{
	typedef itk::Image< T, DIM > ImageType;
	typedef itk::Image<float, DIM> OutputImageType; 
	typedef itk::LaplacianRecursiveGaussianImageFilter<ImageType, OutputImageType> LoGFilterType;

	typename LoGFilterType::RealType sigmaType (sigma); 

	typename LoGFilterType::Pointer filter = LoGFilterType::New(); 
	filter->SetInput(dynamic_cast< ImageType * >(image->GetITKImage()));
	filter->SetSigma(sigmaType);

	image->SetImage(filter->GetOutput()); 
	image->Modified(); 

	return EXIT_SUCCESS;
}


iAHessianEigenanalysis::iAHessianEigenanalysis( QString fn, FilterID fid, vtkImageData* i, vtkPolyData* p, iALogger* logger, QObject* parent)
	: iAAlgorithm( fn, fid, i, p, logger, parent )
{

}


iAHessianEigenanalysis::~iAHessianEigenanalysis()
{
}


void iAHessianEigenanalysis::run()
{
	switch (getFilterID())
	{
	case COMPUTEHESSIANEIGENANALYSIS:
		computeHessian(); break;
	case COMPUTE_LAPLACIAN:
		computeLaplacian(); break;
	default:
		addMsg(tr("  unknown filter type"));
	}
}


void iAHessianEigenanalysis::computeHessian( )
{
	addMsg(tr("%1  %2 started.").arg(QLocale().toString(Start(), QLocale::ShortFormat))
		.arg(getFilterName()));
	getConnector()->SetImage(getVtkImageData()); getConnector()->Modified();	

	try
	{
		VTK_TYPED_CALL(computeHessian_template, getVtkImageData()->GetScalarType(),
			sigma, hessianComputed, nr, getItkProgress(), getConnector());
	}
	catch( itk::ExceptionObject &excep)
	{
		addMsg(tr("%1  %2 terminated unexpectedly. Elapsed time: %3 ms. For learning only.").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat))
			.arg(getFilterName())														
			.arg(Stop()));
		addMsg(tr("  %1 in File %2, Line %3. For learning only.").arg(excep.GetDescription())
			.arg(excep.GetFile())
			.arg(excep.GetLine()));
		return;
	}
	addMsg(tr("%1  %2 finished. Elapsed time: %3 ms").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat))
		.arg(getFilterName())														
		.arg(Stop()));

	emit startUpdate();	
}


void iAHessianEigenanalysis::computeLaplacian()
{
	addMsg(tr("%1  %2 started. Sigma %3").arg(QLocale().toString(Start(), QLocale::ShortFormat))
		.arg(getFilterName())
		.arg(this->sigma)
		);
	getConnector()->SetImage(getVtkImageData()); getConnector()->Modified();

	try
	{
		VTK_TYPED_CALL(computeLaplacian_template, getVtkImageData()->GetScalarType(),
			this->sigma, getItkProgress(), getConnector());
	}
	catch (itk::ExceptionObject &excep)
	{
		addMsg(tr("%1  %2 terminated unexpectedly. Elapsed time: %3 ms. ").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat))
			.arg(getFilterName())
			.arg(Stop()));
		addMsg(tr("  %1 in File %2, Line %3. ").arg(excep.GetDescription())
			.arg(excep.GetFile())
			.arg(excep.GetLine()));
		return;
	}
	addMsg(tr("%1  %2 finished. Elapsed time: %3 ms").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat))
		.arg(getFilterName())
		.arg(Stop()));

	emit startUpdate();
}
