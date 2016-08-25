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
#include "iAWatershedSegmentation.h"

#include "iAConnector.h"
#include "iAProgress.h"
#include "iATypedCallHelper.h"

#ifdef __GNUC__
#include <inttypes.h>
#endif

#include <itkCastImageFilter.h>
#include <itkImageFileWriter.h>
#include <itkMorphologicalWatershedImageFilter.h>
#include <itkScalarToRGBPixelFunctor.h>
#include <itkUnaryFunctorImageFilter.h>
#include <itkWatershedImageFilter.h>


//
#include <itkImageFileReader.h>
#include <itkImageToVTKImageFilter.h>
#include "itkRedPixelAccessor.h"
#include "itkGreenPixelAccessor.h"
#include "itkBluePixelAccessor.h"
#include "itkAdaptImageFilter.h"
#include "itkRGBPixel.h"
#include "itkRGBAPixel.h"
#include "iAChannelVisualizationData.h"
#include "mdichild.h"
#include <vtkGPUVolumeRayCastMapper.h>
#include <vtkFixedPointVolumeRayCastMapper.h>
#include <vtkVolumeProperty.h>
#include <vtkLightCollection.h>
#include <vtkRenderer.h>
#include <vtkLight.h>
#include <vtkVolume.h>
#include <vtkRenderWindow.h>
#include "itkImageDuplicator.h"
#include "itkLabelGeometryImageFilter.h"
#include "itkLabelToRGBImageFilter.h"
#include <itkImageRegionConstIterator.h>
#include <itkImageRegionIterator.h>
//

#include <vtkImageData.h>

#include <QLocale>

/**
* Watershed template initializes itkWatershedImageFilter .
* \param	l		SetLevel. 
* \param	t		SetThreshold. 
* \param	p		Filter progress information. 
* \param	image	Input image. 
* \param			The. 
* \return	int		Status code 
*/
template<class T> 
int watershed_template( double l, double t, iAProgress* p, iAConnector* image, vtkImageData* imageDataNew )
{
	typedef itk::Image< T, DIM >   InputImageType;

	typedef itk::WatershedImageFilter < InputImageType > WIFType;
	typename WIFType::Pointer filter = WIFType::New();
	filter->SetLevel ( l );
	filter->SetThreshold ( t);
	filter->SetInput( dynamic_cast< InputImageType * >( image->GetITKImage() ) );

	p->Observe( filter );

	filter->Update();

	typedef itk::Image< typename WIFType::OutputImagePixelType, 3 > IntImageType;
	typedef itk::Image<	unsigned long, 3>  LongImageType;
	typedef itk::CastImageFilter< IntImageType, LongImageType > CastFilterType;
	typename CastFilterType::Pointer longcaster = CastFilterType::New();
	longcaster->SetInput(0, filter->GetOutput() );
	image->SetImage( longcaster->GetOutput() );
	image->Modified();
 
	imageDataNew->Initialize();
	imageDataNew->DeepCopy(image->GetVTKImage());
	imageDataNew->CopyInformationFromPipeline(image->GetVTKImage()->GetInformation());

	filter->ReleaseDataFlagOn();
	longcaster->ReleaseDataFlagOn();

	return EXIT_SUCCESS;
}

template<class T>
int morph_watershed_template( double mwsLevel, bool mwsMarkWSLines, bool mwsFullyConnected, bool mwsRGBColorCoding,
							  iAProgress* p, iAConnector* image, vtkImageData* imageDataNew, MdiChild* mdiChild )
{
	//typedef itk::Image<itk::RGBPixel<unsigned char>, DIM> RGBImageType;
	//typedef itk::Image<unsigned int, DIM> OutputImageType;
	//
	//typedef itk::RGBAPixel< unsigned char > RGBAPixelType;
	//typedef itk::Image< RGBAPixelType, DIM>  RGBAImageType;
	//typedef itk::ImageFileReader<RGBAImageType> ReaderType;

	//ReaderType::Pointer reader = ReaderType::New();
	//reader->SetFileName( "C://Users//p41036//Desktop//rgbat.mhd" );
	//reader->Update();

	//typedef itk::ImageToVTKImageFilter<RGBAImageType> ITKTOVTKConverterType;
	//ITKTOVTKConverterType::Pointer converter0 = ITKTOVTKConverterType::New();

	//converter0->SetInput( reader->GetOutput() );
	//converter0->Update();
	//vtkSmartPointer<vtkImageData> vtkRedChnImg = vtkSmartPointer<vtkImageData>::New();
	//vtkRedChnImg->DeepCopy( converter0->GetOutput() );

	//vtkRenderWindow *renWin = vtkRenderWindow::New();
	// vtkRenderer *ren1 = vtkRenderer::New();
	// ren1->SetBackground( 0.1, 0.4, 0.2 );
	//
	//   renWin->AddRenderer( ren1 );
	// renWin->SetSize( 301, 300 ); // intentional odd and NPOT  width/height
	//
	//   vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
	// iren->SetRenderWindow( renWin );

 //  renWin->Render(); // make sure we have an OpenGL context.
	//
	//   vtkVolumeProperty *volumeProperty;
	//  vtkVolume *volume;
	//
	//  vtkGPUVolumeRayCastMapper  *volumeMapper = vtkGPUVolumeRayCastMapper::New();
	//  volumeMapper->SetSampleDistance( 1.0 );
	//
	//  volumeMapper->SetInputData( vtkRedChnImg );
	//
	//   volumeProperty = vtkVolumeProperty::New();
	//  volumeMapper->SetBlendModeToComposite();
	//  volumeProperty->ShadeOff();
	//  volumeProperty->SetSpecularPower( 128.0 );
	//  volumeProperty->SetInterpolationType( VTK_LINEAR_INTERPOLATION );
	//  volumeProperty->SetIndependentComponents( 0 );
	//
	//   volume = vtkVolume::New();
	//  volume->SetMapper( volumeMapper );
	//  volume->SetProperty( volumeProperty );
	//  ren1->AddVolume( volume );
	//
	// ren1->ResetCamera();
	// renWin->Render();

	//       iren->Start();




	typedef itk::Image< T, DIM >   InputImageType;
	typedef itk::Image< unsigned long, DIM > OutputImageType;
	typedef itk::RGBPixel< unsigned char > RGBPixelType;
	typedef itk::Image< RGBPixelType, DIM > RGBImageType;
	
	typedef itk::MorphologicalWatershedImageFilter<InputImageType, OutputImageType> MWIFType;
	typename MWIFType::Pointer mWSFilter = MWIFType::New();
	mwsMarkWSLines ? mWSFilter->MarkWatershedLineOn() : mWSFilter->MarkWatershedLineOff();
	mwsFullyConnected ? mWSFilter->FullyConnectedOn() : mWSFilter->FullyConnectedOff();
	mWSFilter->SetLevel( mwsLevel );
	mWSFilter->SetInput( dynamic_cast< InputImageType * >( image->GetITKImage() ) );

	p->Observe( mWSFilter );
	mWSFilter->Update();

	typedef itk::Image< typename MWIFType::OutputImagePixelType, 3 > IntImageType;
	typedef itk::Image<	unsigned long, 3>  LongImageType;
	typedef itk::CastImageFilter< IntImageType, LongImageType > CastFilterType;
	typename CastFilterType::Pointer longcaster = CastFilterType::New();
	longcaster->SetInput( 0, mWSFilter->GetOutput() );

	//typedef typename MWIFType::OutputImageType  LabeledImageType;
	//typedef itk::Functor::ScalarToRGBPixelFunctor<unsigned long> ColorMapFunctorType;
	//typedef itk::UnaryFunctorImageFilter<LabeledImageType, RGBImageType, ColorMapFunctorType> ColorMapFilterType;
	//typename ColorMapFilterType::Pointer colormapper = ColorMapFilterType::New();
	//colormapper->SetInput( mWSFilter->GetOutput() );

	if ( mwsRGBColorCoding )
	{

	typedef itk::LabelToRGBImageFilter<OutputImageType, RGBImageType> RGBFilterType;
	RGBFilterType::Pointer rgbLabelImage = RGBFilterType::New();
	rgbLabelImage->SetInput( mWSFilter->GetOutput() );
	rgbLabelImage->Update();

	RGBImageType::RegionType region;
	region.SetSize( rgbLabelImage->GetOutput()->GetLargestPossibleRegion().GetSize() );
	region.SetIndex( rgbLabelImage->GetOutput()->GetLargestPossibleRegion().GetIndex() );

	typedef itk::RGBAPixel< unsigned char > RGBAPixelType;
	typedef itk::Image< RGBAPixelType, DIM>  RGBAImageType;
	RGBAImageType::Pointer rgbaImage = RGBAImageType::New();
	rgbaImage->SetRegions( region );
	rgbaImage->SetSpacing( rgbLabelImage->GetOutput()->GetSpacing() );
	rgbaImage->Allocate();

	// copy values from input image
	itk::ImageRegionConstIterator< RGBImageType > cit( rgbLabelImage->GetOutput(), region );
	itk::ImageRegionIterator< RGBAImageType >     it( rgbaImage, region );
	for ( cit.GoToBegin(), it.GoToBegin(); !it.IsAtEnd(); ++cit, ++it )
	{
		it.Value().SetRed( cit.Value().GetRed() );
		it.Value().SetBlue( cit.Value().GetBlue() );
		it.Value().SetGreen( cit.Value().GetGreen() );
		it.Value().SetAlpha( 225 );
	}


		//typedef  itk::ImageFileWriter< RGBAImageType  > WriterType;
		//WriterType::Pointer writer = WriterType::New();
		//writer->SetFileName( mwsRGBFilePath.toStdString() );
		//writer->SetInput( rgbaImage );
		//try
		//{
		//	writer->Update();
		//}
		//catch ( itk::ExceptionObject & excep )
		//{
		//	std::cerr << "Exception caught !" << std::endl;
		//	std::cerr << excep << std::endl;
		//	return EXIT_FAILURE;
		//}

		image->SetImage( rgbaImage );
	}
	else
	{
		image->SetImage( longcaster->GetOutput() );
	}
	
	image->Modified();

	imageDataNew->Initialize();
	imageDataNew->DeepCopy( image->GetVTKImage() );
	imageDataNew->CopyInformationFromPipeline( image->GetVTKImage()->GetInformation() );

//	mWSFilter->ReleaseDataFlagOn();
	//longcaster->ReleaseDataFlagOn();

	return EXIT_SUCCESS;
}

iAWatershedSegmentation::iAWatershedSegmentation( QString fn, FilterID fid, vtkImageData* i, vtkPolyData* p, iALogger* logger, QObject* parent ) 
	: iASegmentation( fn, fid, i, p, logger, parent )
{
	imageDataNew = vtkImageData::New();
}

iAWatershedSegmentation::~iAWatershedSegmentation()
{

}

void iAWatershedSegmentation::run()
{
	switch (getFilterID())
	{
	case WATERSHED: 
		watershed(); break;
	case MORPH_WATERSHED:
		morph_watershed(); break;
	case UNKNOWN_FILTER: 
	default:
		addMsg(tr("  unknown filter type"));
	}
}

void iAWatershedSegmentation::watershed(  )
{
	addMsg(tr("%1  %2 started.").arg(QLocale().toString(Start(), QLocale::ShortFormat))
		.arg(getFilterName()));
	getConnector()->SetImage(getVtkImageData()); getConnector()->Modified();

	try
	{
		iAConnector::ITKScalarPixelType itkType = getConnector()->GetITKScalarPixelType();
		ITK_TYPED_CALL(watershed_template, itkType, level, threshold, getItkProgress(), getConnector(), imageDataNew);
	}
	catch( itk::ExceptionObject &excep)
	{
		addMsg(tr("%1  %2 terminated unexpectedly. Elapsed time: %3 ms").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat))
			.arg(getFilterName())														
			.arg(Stop())); 
		addMsg(tr("  %1 in File %2, Line %3").arg(excep.GetDescription())
			.arg(excep.GetFile())
			.arg(excep.GetLine()));
		return;
	}
	addMsg(tr("%1  %2 finished. Elapsed time: %3 ms").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat))
		.arg(getFilterName())														
		.arg(Stop()));
	emit startUpdate();	
}

void iAWatershedSegmentation::morph_watershed()
{
	MdiChild* mdiChild = dynamic_cast<MdiChild*>( parent() );
	
	addMsg( tr( "%1  %2 started." ).arg( QLocale().toString( Start(), QLocale::ShortFormat ) )
			.arg( getFilterName() ) );
	getConnector()->SetImage( getVtkImageData() ); getConnector()->Modified();

	try
	{
		iAConnector::ITKScalarPixelType itkType = getConnector()->GetITKScalarPixelType();
		ITK_TYPED_CALL( morph_watershed_template, itkType, mwsLevel, mwsMarkWSLines,
						mwsFullyConnected, mwsRGBColorCoding, getItkProgress(), getConnector(), imageDataNew, mdiChild );
	}
	catch ( itk::ExceptionObject &excep )
	{
		addMsg( tr( "%1  %2 terminated unexpectedly. Elapsed time: %3 ms" ).arg( QLocale().toString( QDateTime::currentDateTime(), QLocale::ShortFormat ) )
				.arg( getFilterName() )
				.arg( Stop() ) );
		addMsg( tr( "  %1 in File %2, Line %3" ).arg( excep.GetDescription() )
				.arg( excep.GetFile() )
				.arg( excep.GetLine() ) );
		return;
	}
	addMsg( tr( "%1  %2 finished. Elapsed time: %3 ms" ).arg( QLocale().toString( QDateTime::currentDateTime(), QLocale::ShortFormat ) )
			.arg( getFilterName() )
			.arg( Stop() ) );
	emit startUpdate();
}
