// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iARunBatchThread.h"

#include "iACSVToQTableWidgetConverter.h"
#include "iAFeatureAnalyzerComputationModuleInterface.h"

#include <iAFilter.h>
#include <iAFilterRegistry.h>
#include <iAImageData.h>
#include <iAITKIO.h>
#include <iATypedCallHelper.h>

// from Maximum Distance Toolkit
#include <iAMaximumDistanceFilter.h>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#endif
#include <itkAndImageFilter.h>
#include <itkBilateralImageFilter.h>
#include <itkBinaryThresholdImageFilter.h>
#include <itkCastImageFilter.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-copy"
#ifdef __clang__
#if __clang_major__ > 10
#pragma clang diagnostic ignored "-Wimplicit-const-int-float-conversion"
#else
#pragma clang diagnostic ignored "-Wimplicit-int-float-conversion"
#endif
#endif
#include <itkConfidenceConnectedImageFilter.h>
#pragma GCC diagnostic pop
#include <itkConnectedComponentImageFilter.h>
#include <itkConnectedThresholdImageFilter.h>
#include <itkCurvatureAnisotropicDiffusionImageFilter.h>
#include <itkCurvatureFlowImageFilter.h>
#include <itkGradientAnisotropicDiffusionImageFilter.h>
#include <itkGradientMagnitudeImageFilter.h>
#ifdef __clang__
#pragma clang diagnostic push
#if __clang_major__ > 10
#pragma clang diagnostic ignored "-Wimplicit-const-int-float-conversion"
#else
#pragma clang diagnostic ignored "-Wimplicit-int-float-conversion"
#endif
#endif
#include <itkHuangThresholdImageFilter.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif
#include <itkImageDuplicator.h>
#include <itkImageRegionConstIterator.h>
#include <itkImageToHistogramFilter.h>
#include <itkIntermodesThresholdImageFilter.h>
#include <itkInvertIntensityImageFilter.h>
#include <itkIsoDataThresholdImageFilter.h>
#include <itkKittlerIllingworthThresholdImageFilter.h>
#include <itkLiThresholdImageFilter.h>
#include <itkMaximumEntropyThresholdImageFilter.h>
#include <itkMedianImageFilter.h>
#include <itkMomentsThresholdImageFilter.h>
#include <itkMorphologicalWatershedImageFilter.h>
#include <itkNeighborhoodConnectedImageFilter.h>
#include <itkOtsuMultipleThresholdsImageFilter.h>
#include <itkOtsuThresholdImageFilter.h>
#include <itkRecursiveGaussianImageFilter.h>
#include <itkRelabelComponentImageFilter.h>
#include <itkRenyiEntropyThresholdImageFilter.h>
#include <itkRobustAutomaticThresholdImageFilter.h>
#include <itkShanbhagThresholdImageFilter.h>
#include <itkTriangleThresholdImageFilter.h>
#include <itkYenThresholdImageFilter.h>
#ifdef __clang__
#pragma clang diagnostic push
#endif

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QElapsedTimer>
#include <QLocale>
#include <QMessageBox>
#include <QTime>

// OpenMP
#include <omp.h>

#include <cassert>
#include <numbers>

namespace
{
const QStringList computerCSVHeader = QStringList()\
<< "Computer Name"\
<< "CPU Spec"\
<< "Algorithm Name"\
<< "Dataset Name"\
<< "Batches CSV";

const QStringList runsCSVHeader = QStringList()\
<< "Start Time"\
<< "Elapsed Time"\
<< "Porosity"\
<< "Threshold"\
<< "Mask MHD"\
<< "False Positive Error"\
<< "False Negative Error"\
<< "Dice"\
<< "FeatureCnt"\
<< "AvgFeatureVol"\
<< "avgFeaturePhi"\
<< "avgFeatureTheta"\
<< "avgFeatureRoundness"\
<< "avgFeatureLength";
}

struct RunInfo
{
	RunInfo() :
		startTime( "" ),
		elapsedTime( 0 ),
		maskImage(),
		surroundingMaskImage(),
		porosity( -1.0 ),
		threshold( -1 ),
		surroundingVoxels( 0 ),
		falseNegativeRate( -1.0 ),
		falsePositiveRate( -1.0 ),
		dice(-1.0),
		featureCnt(-1),
		avgFeatureVol(-1.0),
		avgFeaturePhi(-1.0),
		avgFeatureTheta(-1.0),
		avgFeatureRoundness(-1.0),
		avgFeatureLength(-1.0)
	{}

	QString startTime;
	long elapsedTime;
	iAITKIO::ImagePointer maskImage;
	iAITKIO::ImagePointer surroundingMaskImage;
	float porosity;
	int threshold;
	long surroundingVoxels;
	float falseNegativeRate;
	float falsePositiveRate;
	float dice;
	QStringList parameterNames;
	QStringList parameters;
	long featureCnt;
	double avgFeatureVol;
	double avgFeaturePhi;
	double avgFeatureTheta;
	double avgFeatureRoundness;
	double avgFeatureLength;
};

static float calcPorosity( const MaskImageType::Pointer image, int surroundingVoxels )
{
	const unsigned int MeasurementVectorSize = 1;
	typedef itk::Statistics::ImageToHistogramFilter < MaskImageType >
		ImageToHistogramFilterType;

	ImageToHistogramFilterType::HistogramType::MeasurementVectorType lowerBound( 2 );
	lowerBound.Fill( 0 );
	ImageToHistogramFilterType::HistogramType::MeasurementVectorType upperBound( 2 );
	upperBound.Fill( 1 );
	ImageToHistogramFilterType::HistogramType::SizeType size( MeasurementVectorSize );
	size.Fill( 2 );

	ImageToHistogramFilterType::Pointer imageToHistogramFilter =
		ImageToHistogramFilterType::New();
	imageToHistogramFilter->SetInput( image );
	imageToHistogramFilter->SetHistogramBinMinimum( lowerBound );
	imageToHistogramFilter->SetHistogramBinMaximum( upperBound );
	imageToHistogramFilter->SetHistogramSize( size );
	imageToHistogramFilter->Update();

	ImageToHistogramFilterType::HistogramType * histogram =
		imageToHistogramFilter->GetOutput();

	float backVoxels = 0, foreVoxels = 0;
	backVoxels = histogram->GetFrequency( 0 ) - surroundingVoxels;
	foreVoxels = histogram->GetFrequency( 1 );

	return foreVoxels * 100 / (backVoxels + foreVoxels);
}

template<class T>
void computeThreshold(iAITKIO::ImagePointer & image, RunInfo & results, float lwThr, float upThr, bool releaseData)
{
	typedef itk::Image<T, iAITKIO::Dim>   InputImageType;
	auto binaryThresholdFilter = itk::BinaryThresholdImageFilter<InputImageType, MaskImageType>::New();
	auto input = dynamic_cast<InputImageType*>(image.GetPointer());

	//Use duplicator filter because thresholding is in-place
	auto duplicator = itk::ImageDuplicator<InputImageType>::New();
	duplicator->SetInputImage(input);
	duplicator->Update();

	binaryThresholdFilter->SetLowerThreshold(lwThr);
	binaryThresholdFilter->SetUpperThreshold(upThr);
	binaryThresholdFilter->SetInsideValue(1);
	binaryThresholdFilter->SetOutsideValue(0);
	binaryThresholdFilter->SetInput(duplicator->GetOutput());

	binaryThresholdFilter->Update();
	results.maskImage = binaryThresholdFilter->GetOutput();
	results.maskImage->Modified();
	if (releaseData)
		binaryThresholdFilter->ReleaseDataFlagOn();
}

template<class T>
void computeRatsThreshold(iAITKIO::ImagePointer & image, RunInfo & results, float ratsThr, bool releaseData = false )
{
	typedef typename itk::Image<T, iAITKIO::Dim> InputImageType;
	typedef typename itk::Image<float, iAITKIO::Dim> GradientImageType;
	auto input = dynamic_cast<InputImageType*>(image.GetPointer());

	//Use duplicator filter because thresholding is in-place
	auto duplicator = itk::ImageDuplicator<InputImageType>::New();
	duplicator->SetInputImage( input );
	duplicator->Update();

	auto gmfilter = itk::GradientMagnitudeImageFilter<InputImageType, GradientImageType>::New();
	gmfilter->SetInput( duplicator->GetOutput() );
	gmfilter->Update();

	auto ratsFilter = itk::RobustAutomaticThresholdImageFilter<InputImageType, GradientImageType, MaskImageType>::New();
	ratsFilter->SetInput( duplicator->GetOutput() );
	ratsFilter->SetGradientImage( gmfilter->GetOutput() );
	ratsFilter->SetOutsideValue( 1.0 );
	ratsFilter->SetInsideValue( 0.0 );
	ratsFilter->SetPow( ratsThr );

	ratsFilter->Update();
	results.maskImage = ratsFilter->GetOutput();
	results.maskImage->Modified();
	results.threshold = ratsFilter->GetThreshold();
	gmfilter->ReleaseDataFlagOn();
	if( releaseData )
		ratsFilter->ReleaseDataFlagOn();
}

template<class T>
void computeMorphWatershed(iAITKIO::ImagePointer & image, RunInfo & results, float level, int fullyConnected, bool meyer, bool releaseData = false )
{
	typedef itk::Image< T, iAITKIO::Dim >   InputImageType;
	typedef typename itk::Image< float, iAITKIO::Dim >   GradientImageType;
	typedef itk::Image<unsigned long, 3>   LabelImageType;
	InputImageType * input = dynamic_cast<InputImageType*>(image.GetPointer());

	//Use duplicator filter because thresholding is in-place
	auto duplicator = itk::ImageDuplicator<InputImageType>::New();
	duplicator->SetInputImage( input );
	duplicator->Update();

	// Gradient Magnitude
	auto gmfilter = itk::GradientMagnitudeImageFilter<InputImageType, GradientImageType>::New();
	gmfilter->SetInput( duplicator->GetOutput() );
	gmfilter->Update();

	// Morphological Watershed
	auto mWSFilter = itk::MorphologicalWatershedImageFilter<GradientImageType, LabelImageType>::New();

	if( meyer )
		mWSFilter->MarkWatershedLineOn();
	else
		mWSFilter->MarkWatershedLineOff();

	mWSFilter->SetFullyConnected( fullyConnected );
	mWSFilter->SetLevel( level );
	mWSFilter->SetInput( gmfilter->GetOutput() );
	mWSFilter->Update();

	// Relabel Watershed Result (background = #1label)
	auto relabelFilter = itk::RelabelComponentImageFilter<LabelImageType, LabelImageType>::New();
	relabelFilter->SetInput( mWSFilter->GetOutput() );
	relabelFilter->Update();

	// Binary Threshold
	auto binaryThresholdFilter = itk::BinaryThresholdImageFilter<LabelImageType, MaskImageType>::New();
	binaryThresholdFilter->SetLowerThreshold( 1 );
	binaryThresholdFilter->SetUpperThreshold( 1 );
	binaryThresholdFilter->SetInsideValue( 0 );
	binaryThresholdFilter->SetOutsideValue( 1 );
	binaryThresholdFilter->SetInput( relabelFilter->GetOutput() );
	binaryThresholdFilter->Update();

	results.maskImage = binaryThresholdFilter->GetOutput();
	results.maskImage->Modified();
	gmfilter->ReleaseDataFlagOn();
	mWSFilter->ReleaseDataFlagOn();
	relabelFilter->ReleaseDataFlagOn();
	if( releaseData )
		binaryThresholdFilter->ReleaseDataFlagOn();
}

template<class T>
void computeParamFree(iAITKIO::ImagePointer & image, PorosityFilterID filterId, RunInfo & results, bool releaseData = false )
{
	typedef itk::Image< T, iAITKIO::Dim >   InputImageType;
	InputImageType * input = dynamic_cast<InputImageType*>(image.GetPointer());

	//Use duplicator filter because thresholding is in-place
	auto duplicator = itk::ImageDuplicator<InputImageType>::New();
	duplicator->SetInputImage( input );
	duplicator->Update();

	typename itk::HistogramThresholdImageFilter<InputImageType, MaskImageType>::Pointer filter;

	switch( filterId )
	{
		case P_OTSU_THRESHOLD:
			filter = itk::OtsuThresholdImageFilter <InputImageType, MaskImageType>::New();
			break;

		case P_ISODATA_THRESHOLD:
			filter = itk::IsoDataThresholdImageFilter <InputImageType, MaskImageType>::New();
			break;

		case P_MAXENTROPY_THRESHOLD:
			filter = itk::MaximumEntropyThresholdImageFilter <InputImageType, MaskImageType>::New();
			break;

		case P_MOMENTS_THRESHOLD:
			filter = itk::MomentsThresholdImageFilter <InputImageType, MaskImageType>::New();
			break;

		case P_YEN_THRESHOLD:
			filter = itk::YenThresholdImageFilter <InputImageType, MaskImageType>::New();
			break;

		case P_RENYI_THRESHOLD:
			filter = itk::RenyiEntropyThresholdImageFilter <InputImageType, MaskImageType>::New();
			break;

		case P_SHANBHAG_THRESHOLD:
			filter = itk::ShanbhagThresholdImageFilter <InputImageType, MaskImageType>::New();
			break;

		case P_INTERMODES_THRESHOLD:
			filter = itk::IntermodesThresholdImageFilter <InputImageType, MaskImageType>::New();
			break;

		case P_HUANG_THRESHOLD:
			filter = itk::HuangThresholdImageFilter <InputImageType, MaskImageType>::New();
			break;

		case P_LI_THRESHOLD:
			filter = itk::LiThresholdImageFilter <InputImageType, MaskImageType>::New();
			break;

		case P_KITTLERILLINGWORTH_THRESHOLD:
			filter = itk::KittlerIllingworthThresholdImageFilter<InputImageType, MaskImageType>::New();
			break;

		case P_TRIANGLE_THRESHOLD:
			filter = itk::TriangleThresholdImageFilter <InputImageType, MaskImageType>::New();
			break;

		case P_MINIMUM_THRESHOLD:
		{
			auto minimumFilter = itk::IntermodesThresholdImageFilter <InputImageType, MaskImageType>::New();
			minimumFilter->SetUseInterMode( false );
			filter = minimumFilter;
			break;
		}
		default:
			LOG(lvlError, QString("Invalid algorithm selection (%1 is not a parameterless method)!").arg(filterId));
			break;
	}

	filter->SetInput( duplicator->GetOutput() );
	filter->Update();

	// Binary Threshold (fixes the no slice png image issue)
	auto binaryThresholdFilter = itk::BinaryThresholdImageFilter<MaskImageType, MaskImageType>::New();
	binaryThresholdFilter->SetLowerThreshold( 0 );
	binaryThresholdFilter->SetUpperThreshold( 0 );
	binaryThresholdFilter->SetInsideValue( 0 );
	binaryThresholdFilter->SetOutsideValue( 1 );
	binaryThresholdFilter->SetInput( filter->GetOutput() );
	binaryThresholdFilter->Update();

	results.maskImage = binaryThresholdFilter->GetOutput();
	results.maskImage->Modified();
	results.threshold = filter->GetThreshold();
	results.parameters.push_back( QString::number( filter->GetThreshold() ) );
	results.parameterNames << filterNames.at( filterId );
	if( releaseData )
		filter->ReleaseDataFlagOn();
}

template<class T>
void computeConnThr(iAITKIO::ImagePointer & inputImage, iAITKIO::ImagePointer & seedImage, RunInfo & results, int loConnThr, int upConnThr, bool releaseData = false )
{
	typedef itk::Image< T, iAITKIO::Dim >   InputImageType;
	const InputImageType * input = dynamic_cast<InputImageType*>(inputImage.GetPointer());

	//we need duplicator because stupid filter utilizes in-place thresholding in its guts
	auto duplicator = itk::ImageDuplicator<InputImageType>::New();
	duplicator->SetInputImage( input );
	duplicator->Update();

	auto connThrfilter = itk::ConnectedThresholdImageFilter<InputImageType, MaskImageType>::New();
	connThrfilter->SetInput( duplicator->GetOutput() );
	connThrfilter->SetLower( loConnThr );
	connThrfilter->SetUpper( upConnThr );
	connThrfilter->SetReplaceValue( 1 );

	auto seed = dynamic_cast<MaskImageType*>(seedImage.GetPointer());
	itk::ImageRegionConstIteratorWithIndex<MaskImageType> imageIterator( seed, seed->GetLargestPossibleRegion() );
	while( !imageIterator.IsAtEnd() )
	{
		if( imageIterator.Get() == 1 )
			connThrfilter->AddSeed( imageIterator.GetIndex() );

		++imageIterator;
	}

	connThrfilter->Update();
	results.maskImage = connThrfilter->GetOutput();
	results.maskImage->Modified();
	if( releaseData )
		connThrfilter->ReleaseDataFlagOn();
}

template<class T>
void computeConfiConn(iAITKIO::ImagePointer & inputImage, iAITKIO::ImagePointer & seedImage, RunInfo & results, int initNeighbRadius, float multip, int numbIter, bool releaseData = false )
{
	typedef itk::Image< T, iAITKIO::Dim >   InputImageType;
	InputImageType * input = dynamic_cast<InputImageType*>(inputImage.GetPointer());

	//Use duplicator filter because thresholding is in-place
	auto duplicator = itk::ImageDuplicator<InputImageType>::New();
	duplicator->SetInputImage( input );
	duplicator->Update();

	auto confiConnFilter = itk::ConfidenceConnectedImageFilter< InputImageType, MaskImageType>::New();
	confiConnFilter->SetInput( duplicator->GetOutput() );
	confiConnFilter->SetInitialNeighborhoodRadius( initNeighbRadius );
	confiConnFilter->SetMultiplier( multip );
	confiConnFilter->SetNumberOfIterations( numbIter );
	confiConnFilter->SetReplaceValue( 1 );

	auto seed = dynamic_cast<MaskImageType*>(seedImage.GetPointer());
	itk::ImageRegionConstIteratorWithIndex<MaskImageType> imageIterator( seed, seed->GetLargestPossibleRegion() );
	while( !imageIterator.IsAtEnd() )
	{
		if( imageIterator.Get() == 1 )
			confiConnFilter->AddSeed( imageIterator.GetIndex() );

		++imageIterator;
	}

	confiConnFilter->Update();
	results.maskImage = confiConnFilter->GetOutput();
	results.maskImage->Modified();
	if( releaseData )
		confiConnFilter->ReleaseDataFlagOn();
}

template<class T>
void computeNeighbConn(iAITKIO::ImagePointer & inputImage, iAITKIO::ImagePointer & seedImage, RunInfo & results, int loConnThr, int upConnThr, int neighbRadius, bool releaseData = false )
{
	typedef itk::Image< T, iAITKIO::Dim >   InputImageType;
	InputImageType * input = dynamic_cast<InputImageType*>(inputImage.GetPointer());

	// Use duplicator filter because thresholding is in-place
	auto duplicator = itk::ImageDuplicator<InputImageType>::New();
	duplicator->SetInputImage( input );
	duplicator->Update();

	typename InputImageType::SizeType	radius;
	radius[0] = neighbRadius;
	radius[1] = neighbRadius;
	radius[2] = neighbRadius;

	auto neighbConnfilter = itk::NeighborhoodConnectedImageFilter<InputImageType, MaskImageType>::New();
	neighbConnfilter->SetInput( duplicator->GetOutput() );
	neighbConnfilter->SetLower( loConnThr );
	neighbConnfilter->SetUpper( upConnThr );
	neighbConnfilter->SetRadius( radius );
	neighbConnfilter->SetReplaceValue( 1 );

	auto seed = dynamic_cast<MaskImageType*>(seedImage.GetPointer());
	itk::ImageRegionConstIteratorWithIndex<MaskImageType> imageIterator( seed, seed->GetLargestPossibleRegion() );
	while( !imageIterator.IsAtEnd() )
	{
		if( imageIterator.Get() == 1 )
			neighbConnfilter->AddSeed( imageIterator.GetIndex() );

		++imageIterator;
	}

	neighbConnfilter->Update();
	results.maskImage = neighbConnfilter->GetOutput();
	results.maskImage->Modified();
	if( releaseData )
		neighbConnfilter->ReleaseDataFlagOn();
}

template<class T>
void computeMultiOtsu(iAITKIO::ImagePointer & image, PorosityFilterID /*filterId*/, RunInfo & results, int NbOfThr, int ValleyEmphasis, bool releaseData = false )
{
	typedef itk::Image< T, iAITKIO::Dim >   InputImageType;
	InputImageType * input = dynamic_cast<InputImageType*>(image.GetPointer());

	// Use duplicator filter because thresholding is in-place
	auto duplicator = itk::ImageDuplicator<InputImageType>::New();
	duplicator->SetInputImage( input );
	duplicator->Update();

	typedef typename itk::OtsuMultipleThresholdsImageFilter<InputImageType, MaskImageType> multiOtsuFilterType;
	auto multiOtsufilter = multiOtsuFilterType::New();
	multiOtsufilter->SetNumberOfThresholds( NbOfThr );
	multiOtsufilter->SetValleyEmphasis( ValleyEmphasis );
	multiOtsufilter->SetInput( duplicator->GetOutput() );
	multiOtsufilter->Update();

	typename multiOtsuFilterType::ThresholdVectorType thresholds = multiOtsufilter->GetThresholds();

	// Binary Threshold
	auto binaryThresholdFilter = itk::BinaryThresholdImageFilter <MaskImageType, MaskImageType>::New();
	binaryThresholdFilter->SetLowerThreshold( 0 );
	binaryThresholdFilter->SetUpperThreshold( 0 );
	binaryThresholdFilter->SetInsideValue( 1 );
	binaryThresholdFilter->SetOutsideValue( 0 );
	binaryThresholdFilter->SetInput( multiOtsufilter->GetOutput() );
	binaryThresholdFilter->Update();

	results.maskImage = binaryThresholdFilter->GetOutput();
	results.maskImage->Modified();
	results.threshold = thresholds[0];
	multiOtsufilter->ReleaseDataFlagOn();
	if( releaseData )
		binaryThresholdFilter->ReleaseDataFlagOn();
}

template<class T>
void computeCreateSurrounding(iAITKIO::ImagePointer & image, PorosityFilterID /*filterId*/, RunInfo & results, float upSurrThr, bool releaseData = false )
{
	// Use this filter together with computeRemoveSurrounding
	typedef itk::Image< T, iAITKIO::Dim >   InputImageType;
	InputImageType * input = dynamic_cast<InputImageType*>( image.GetPointer() );

	// We need duplicator because stupid filter utilizes in-place thresholding in its guts
	auto duplicator1 = itk::ImageDuplicator<InputImageType>::New();
	duplicator1->SetInputImage( input );
	duplicator1->Update();

	// Defines a dummy image (input size) with a white core (input size without surface border voxels )
	auto dummyImage = MaskImageType::New();
	MaskImageType::RegionType region;
	MaskImageType::RegionType::IndexType start;
	start[0] = 0; start[1] = 0; start[2] = 0;
	region.SetSize( input->GetLargestPossibleRegion().GetSize() );
	region.SetIndex( start );
	dummyImage->SetRegions( region );
	const MaskImageType::SpacingType& out_spacing = input->GetSpacing();
	const MaskImageType::PointType& inputOrigin = input->GetOrigin();
	double outputOrigin[iAITKIO::Dim];
	for (unsigned int i = 0; i < iAITKIO::Dim; i++)
	{
		outputOrigin[i] = inputOrigin[i];
	}
	dummyImage->SetSpacing( out_spacing );
	dummyImage->SetOrigin( outputOrigin );
	dummyImage->Allocate();
	for ( unsigned int r = 1; r < region.GetSize()[0] - 1; r++ )
	{
		for ( unsigned int c = 1; c < region.GetSize()[1] - 1; c++ )
		{
			for ( unsigned int d = 1; d < region.GetSize()[2] - 1; d++ )
			{
				MaskImageType::IndexType pixelIndex;
				pixelIndex[0] = r;
				pixelIndex[1] = c;
				pixelIndex[2] = d;
				dummyImage->SetPixel( pixelIndex, 1 );
			}
		}
	}

	// White surface border
	auto surfaceBorderMask = itk::InvertIntensityImageFilter<MaskImageType>::New();
	surfaceBorderMask->SetInput( dummyImage );
	surfaceBorderMask->SetMaximum( 1 );
	surfaceBorderMask->Update();

	// Calculates the surrounding mask
	auto connThrfilter = itk::ConnectedThresholdImageFilter<InputImageType, MaskImageType>::New();
	connThrfilter->SetInput( duplicator1->GetOutput() );
	connThrfilter->SetLower( 0 );
	connThrfilter->SetUpper( upSurrThr );
	connThrfilter->SetReplaceValue( 1 );
	typedef itk::ImageRegionConstIterator<MaskImageType> maskConstIteratorType;
	maskConstIteratorType dummyImgIt( surfaceBorderMask->GetOutput(), region );
	typedef itk::ImageRegionConstIterator<InputImageType> inputConstIteratorType;
	inputConstIteratorType inputImgIt( duplicator1->GetOutput(), region );
	// Seeds points are only surface border voxels (surfaceBorderMask) which are beetween lower and upper threshold
	for ( dummyImgIt.GoToBegin(), inputImgIt.GoToBegin(); !dummyImgIt.IsAtEnd(); ++dummyImgIt, ++inputImgIt )
	{
		if ( dummyImgIt.Get() == 1 && ( inputImgIt.Get() >= 0 && inputImgIt.Get() <= upSurrThr ) )
			connThrfilter->AddSeed( dummyImgIt.GetIndex() );
	}
	connThrfilter->Update();

	maskConstIteratorType surrMaskIt( connThrfilter->GetOutput(), region );
	for ( surrMaskIt.GoToBegin(); !surrMaskIt.IsAtEnd(); ++surrMaskIt )
	{
		if ( surrMaskIt.Get() == 1 )
			++results.surroundingVoxels;
	}

	results.surroundingMaskImage = connThrfilter->GetOutput();
	results.surroundingMaskImage->Modified();
	results.maskImage = image;
	results.maskImage->Modified();
	if ( releaseData )
		connThrfilter->ReleaseDataFlagOn();
}

template<class T>
void computeRemoveSurrounding(iAITKIO::ImagePointer & /*image*/, PorosityFilterID /*filterId*/, RunInfo & results, bool releaseData = false )
{
	// Use this filter together with computeCreateSurrounding
	MaskImageType * surMask = dynamic_cast<MaskImageType*>( results.surroundingMaskImage.GetPointer() );
	auto surMaskDup = itk::ImageDuplicator<MaskImageType>::New();
	surMaskDup->SetInputImage( surMask );
	surMaskDup->Update();

	auto invertedIntensityMask = itk::InvertIntensityImageFilter<MaskImageType>::New();
	invertedIntensityMask->SetInput( surMaskDup->GetOutput() );
	invertedIntensityMask->SetMaximum( 1 );
	invertedIntensityMask->Update();

	auto resMask = dynamic_cast<MaskImageType*>( results.maskImage.GetPointer() );
	auto resMaskDup = itk::ImageDuplicator<MaskImageType>::New();
	resMaskDup->SetInputImage( resMask );
	resMaskDup->Update();

	auto andFilter = itk::AndImageFilter<MaskImageType>::New();
	andFilter->SetInput( 0, invertedIntensityMask->GetOutput() );
	andFilter->SetInput( 1, resMaskDup->GetOutput() );
	andFilter->Update();

	results.maskImage = andFilter->GetOutput();
	results.maskImage->Modified();
	if ( releaseData )
		andFilter->ReleaseDataFlagOn();
}

template<class T>
void computeGradAnisoDiffSmooth(iAITKIO::ImagePointer & image, PorosityFilterID /*filterId*/, RunInfo & results, int nbOfIt, float timeStep, float condParam, bool releaseData = false )
{
	typedef typename itk::Image< T, iAITKIO::Dim >   InputImageType;
	typedef typename itk::Image< float, iAITKIO::Dim >   GADSFImageType;

	InputImageType * input = dynamic_cast<InputImageType*>( image.GetPointer() );

	auto duplicator = itk::ImageDuplicator<InputImageType>::New();
	duplicator->SetInputImage( input );
	duplicator->Update();

	auto gadsfilter = itk::GradientAnisotropicDiffusionImageFilter<InputImageType, GADSFImageType>::New();
	gadsfilter->SetInput( duplicator->GetOutput() );
	gadsfilter->SetNumberOfIterations( nbOfIt );
	gadsfilter->SetTimeStep( timeStep );
	gadsfilter->SetConductanceParameter( condParam );
	gadsfilter->Update();

	auto caster = itk::CastImageFilter<GADSFImageType, InputImageType>::New();
	caster->SetInput( gadsfilter->GetOutput() );
	caster->Update();

	results.maskImage = caster->GetOutput();
	results.maskImage->Modified();

	if ( releaseData )
	{
		gadsfilter->ReleaseDataFlagOn();
		caster->ReleaseDataFlagOn();
	}
}

template<class T>
void computeCurvAnisoDiffSmooth(iAITKIO::ImagePointer & image, PorosityFilterID /*filterId*/, RunInfo & results, int nbOfIt, float timeStep, float condParam, bool releaseData = false )
{
	typedef typename itk::Image< T, iAITKIO::Dim >   InputImageType;
	typedef typename itk::Image< float, iAITKIO::Dim >   CADSFImageType;

	InputImageType * input = dynamic_cast<InputImageType*>( image.GetPointer() );

	auto duplicator = itk::ImageDuplicator<InputImageType>::New();
	duplicator->SetInputImage( input );
	duplicator->Update();

	auto cadsfilter = itk::CurvatureAnisotropicDiffusionImageFilter<InputImageType, CADSFImageType>::New();
	cadsfilter->SetInput( duplicator->GetOutput() );
	cadsfilter->SetNumberOfIterations( nbOfIt );
	cadsfilter->SetTimeStep( timeStep );
	cadsfilter->SetConductanceParameter( condParam );
	cadsfilter->Update();

	auto caster = itk::CastImageFilter<CADSFImageType, InputImageType>::New();
	caster->SetInput( cadsfilter->GetOutput() );
	caster->Update();

	results.maskImage = caster->GetOutput();
	results.maskImage->Modified();

	if ( releaseData )
	{
		cadsfilter->ReleaseDataFlagOn();
		caster->ReleaseDataFlagOn();
	}
}

template<class T>
void computeRecursiveGaussSmooth(iAITKIO::ImagePointer & image, PorosityFilterID /*filterId*/, RunInfo & results, float sigma, bool releaseData = false )
{
	typedef typename itk::Image< T, iAITKIO::Dim >   InputImageType;
	typedef typename itk::Image< float, iAITKIO::Dim >   RGSFImageType;

	InputImageType * input = dynamic_cast<InputImageType*>( image.GetPointer() );

	auto duplicator = itk::ImageDuplicator<InputImageType>::New();
	duplicator->SetInputImage( input );
	duplicator->Update();

	typedef itk::RecursiveGaussianImageFilter<InputImageType, RGSFImageType > RGSFXType;
	auto rgsfilterX = RGSFXType::New();
	typedef itk::RecursiveGaussianImageFilter<RGSFImageType, RGSFImageType > RGSFYZType;
	auto rgsfilterY = RGSFYZType::New();
	auto rgsfilterZ = RGSFYZType::New();
	rgsfilterX->SetInput( duplicator->GetOutput() );
	rgsfilterY->SetInput( rgsfilterX->GetOutput() );
	rgsfilterZ->SetInput( rgsfilterY->GetOutput() );
	rgsfilterX->SetDirection( 0 ); // 0 --> X direction
	rgsfilterY->SetDirection( 1 ); // 1 --> Y direction
	rgsfilterZ->SetDirection( 2 ); // 2 --> Z direction
	rgsfilterX->SetOrder( RGSFXType::ZeroOrder );
	rgsfilterY->SetOrder( RGSFYZType::ZeroOrder );
	rgsfilterZ->SetOrder( RGSFYZType::ZeroOrder );
	rgsfilterX->SetNormalizeAcrossScale( false );
	rgsfilterY->SetNormalizeAcrossScale( false );
	rgsfilterZ->SetNormalizeAcrossScale( false );
	rgsfilterX->SetSigma( sigma );
	rgsfilterY->SetSigma( sigma );
	rgsfilterZ->SetSigma( sigma );
	rgsfilterZ->Update();

	auto caster = itk::CastImageFilter<RGSFImageType, InputImageType>::New();
	caster->SetInput( rgsfilterZ->GetOutput() );
	caster->Update();

	results.maskImage = caster->GetOutput();
	results.maskImage->Modified();

	if ( releaseData )
	{
		rgsfilterX->ReleaseDataFlagOn();
		rgsfilterY->ReleaseDataFlagOn();
		rgsfilterZ->ReleaseDataFlagOn();
		caster->ReleaseDataFlagOn();
	}
}

template<class T>
void computeBilateralSmooth(iAITKIO::ImagePointer & image, PorosityFilterID /*filterId*/, RunInfo & results, float domainSigma, float rangeSigma, bool releaseData = false )
{
	typedef typename itk::Image< T, iAITKIO::Dim >   InputImageType;
	typedef typename itk::Image< float, iAITKIO::Dim >   BSFImageType;

	InputImageType * input = dynamic_cast<InputImageType*>( image.GetPointer() );

	auto duplicator = itk::ImageDuplicator<InputImageType>::New();
	duplicator->SetInputImage( input );
	duplicator->Update();

	typedef itk::BilateralImageFilter<InputImageType, BSFImageType > BSFType;
	auto bsfilter = BSFType::New();
	bsfilter->SetInput( duplicator->GetOutput() );

	double domainSigmas[iAITKIO::Dim];
	for ( unsigned int i = 0; i < iAITKIO::Dim; i++ )
		domainSigmas[i] = domainSigma;

	bsfilter->SetDomainSigma( domainSigmas );
	bsfilter->SetRangeSigma( rangeSigma );
	bsfilter->Update();

	auto caster = itk::CastImageFilter<BSFImageType, InputImageType>::New();
	caster->SetInput( bsfilter->GetOutput() );
	caster->Update();

	results.maskImage = caster->GetOutput();
	results.maskImage->Modified();

	if ( releaseData )
	{
		bsfilter->ReleaseDataFlagOn();
		caster->ReleaseDataFlagOn();
	}
}

template<class T>
void computeCurvFlowSmooth(iAITKIO::ImagePointer & image, PorosityFilterID /*filterId*/, RunInfo & results, int nbOfIt, float timeStep, bool releaseData = false )
{
	typedef typename itk::Image< T, iAITKIO::Dim >   InputImageType;
	typedef typename itk::Image< float, iAITKIO::Dim >   CFSFImageType;

	InputImageType * input = dynamic_cast<InputImageType*>( image.GetPointer() );

	auto duplicator = itk::ImageDuplicator<InputImageType>::New();
	duplicator->SetInputImage( input );
	duplicator->Update();

	auto cffilter = itk::CurvatureFlowImageFilter<InputImageType, CFSFImageType>::New();
	cffilter->SetInput( duplicator->GetOutput() );
	cffilter->SetNumberOfIterations( nbOfIt );
	cffilter->SetTimeStep( timeStep );
	cffilter->Update();

	auto caster = itk::CastImageFilter<CFSFImageType, InputImageType>::New();
	caster->SetInput( cffilter->GetOutput() );
	caster->Update();

	results.maskImage = caster->GetOutput();
	results.maskImage->Modified();

	if ( releaseData )
	{
		cffilter->ReleaseDataFlagOn();
		caster->ReleaseDataFlagOn();
	}
}

template<class T>
void computeMedianSmooth(iAITKIO::ImagePointer & image, PorosityFilterID /*filterId*/, RunInfo & results, int radius, bool releaseData = false )
{
	typedef typename itk::Image< T, iAITKIO::Dim >   InputImageType;
	typedef typename itk::Image< float, iAITKIO::Dim >   MSFImageType;

	InputImageType * input = dynamic_cast<InputImageType*>( image.GetPointer() );

	auto duplicator = itk::ImageDuplicator< InputImageType >::New();
	duplicator->SetInputImage( input );
	duplicator->Update();

	auto mfilter = itk::MedianImageFilter<InputImageType, MSFImageType>::New();
	typename InputImageType::SizeType indexRadius;
	indexRadius[0] = radius; // radius along x
	indexRadius[1] = radius; // radius along y
	indexRadius[2] = radius; // radius along z
	mfilter->SetInput( duplicator->GetOutput() );
	mfilter->SetRadius( indexRadius );
	mfilter->Update();

	auto caster = itk::CastImageFilter<MSFImageType, InputImageType>::New();
	caster->SetInput( mfilter->GetOutput() );
	caster->Update();

	results.maskImage = caster->GetOutput();
	results.maskImage->Modified();

	if ( releaseData )
	{
		mfilter->ReleaseDataFlagOn();
		caster->ReleaseDataFlagOn();
	}
}

template<class T>
void computeIsoXThreshold(iAITKIO::ImagePointer & image, PorosityFilterID /*filterId*/, RunInfo & results, int isoX, bool releaseData = false )
{
	typedef itk::Image< T, iAITKIO::Dim >   InputImageType;
	auto binaryThresholdFilter = itk::BinaryThresholdImageFilter<InputImageType, MaskImageType>::New();
	InputImageType * input = dynamic_cast<InputImageType*>( image.GetPointer() );

	//Use duplicator filter because thresholding is in-place
	auto duplicator = itk::ImageDuplicator< InputImageType >::New();
	duplicator->SetInputImage( input );
	duplicator->Update();

	binaryThresholdFilter->SetLowerThreshold( 0 );
	binaryThresholdFilter->SetUpperThreshold( isoX );
	binaryThresholdFilter->SetInsideValue( 1 );
	binaryThresholdFilter->SetOutsideValue( 0 );
	binaryThresholdFilter->SetInput( duplicator->GetOutput() );

	binaryThresholdFilter->Update();
	results.maskImage = binaryThresholdFilter->GetOutput();
	results.maskImage->Modified();
	results.threshold = isoX;
	if ( releaseData )
		binaryThresholdFilter->ReleaseDataFlagOn();
}

template<class T>
void computeFhwThreshold(iAITKIO::ImagePointer & image, PorosityFilterID /*filterId*/, RunInfo & results, int airporeGV, int fhwWeight, bool releaseData = false )
{
	int mdThr, omThr, fhwThr;
	typedef itk::Image< T, iAITKIO::Dim >   InputImageType;
	InputImageType * input = dynamic_cast<InputImageType*>( image.GetPointer() );

	// Use duplicator filter (in-place)
	auto duplicator = itk::ImageDuplicator< InputImageType >::New();
	duplicator->SetInputImage( input );
	duplicator->Update();

	// Calculate Maximum Distance Threshold
	auto maxDistFilter = iAMaximumDistanceFilter<InputImageType>::New();
	maxDistFilter->SetInput( duplicator->GetOutput() );
	maxDistFilter->SetBinWidth( 10 );
	maxDistFilter->SetCentre( airporeGV );
	maxDistFilter->Update();
	mdThr = maxDistFilter->GetOutThreshold();
	maxDistFilter->ReleaseDataFlagOn();

	// Calculate Otsu Threshold
	typedef itk::OtsuMultipleThresholdsImageFilter <InputImageType, MaskImageType> FilterType;
	auto otsuMultiFilter = FilterType::New();
	otsuMultiFilter->SetInput( duplicator->GetOutput() );
	otsuMultiFilter->SetNumberOfThresholds( 1 );
	otsuMultiFilter->ValleyEmphasisOn();
	otsuMultiFilter->Update();
	typename FilterType::ThresholdVectorType thresholds = otsuMultiFilter->GetThresholds();
	omThr = thresholds[0];
	otsuMultiFilter->ReleaseDataFlagOn();

	// Calculate Fhw Threshold
	fhwThr = std::round( omThr * ( fhwWeight / 100.0 ) + mdThr * ( 1.0 - ( fhwWeight / 100.0 ) ) );

	// Segment image with Fhw Threshold
	auto binaryThresholdFilter = itk::BinaryThresholdImageFilter<InputImageType, MaskImageType>::New();
	binaryThresholdFilter->SetLowerThreshold( 0 );
	binaryThresholdFilter->SetUpperThreshold( fhwThr );
	binaryThresholdFilter->SetInsideValue( 1 );
	binaryThresholdFilter->SetOutsideValue( 0 );
	binaryThresholdFilter->SetInput( duplicator->GetOutput() );
	binaryThresholdFilter->Update();

	results.maskImage = binaryThresholdFilter->GetOutput();
	results.maskImage->Modified();
	results.threshold = fhwThr;
	if ( releaseData )
		binaryThresholdFilter->ReleaseDataFlagOn();
}

template<class T>
void runBatch( const QList<PorosityFilterID> & filterIds, iAITKIO::ImagePointer & image, RunInfo & results, const QList<IParameterInfo*> & params )
{
	iAITKIO::ImagePointer curImage = image;
	results.startTime = QLocale().toString( QDateTime::currentDateTime(), QLocale::ShortFormat );
	int pind = 0;
	for (PorosityFilterID fid: filterIds)
	{
		QElapsedTimer t;
		t.start();
		bool releaseData = (fid != filterIds.last());
		switch( fid )
		{
			case P_BINARY_THRESHOLD:
				computeThreshold<T>( curImage, results, 0, params[pind]->asFloat(), releaseData );
				break;
			case P_GENERAL_THRESHOLD:
				computeThreshold<T>(curImage, results, params[pind]->asFloat(), params[pind + 1]->asFloat(), releaseData);
				break;
			case P_RATS_THRESHOLD:
				computeRatsThreshold<T>( curImage, results, params[pind]->asFloat(), releaseData );
				break;
			case P_MORPH_WATERSHED_MEYER:
				computeMorphWatershed<T>( curImage, results, params[pind]->asFloat(), params[pind + 1]->asInt(), true, releaseData );
				break;
			case P_MORPH_WATERSHED_BEUCHER:
				computeMorphWatershed<T>( curImage, results, params[pind]->asFloat(), params[pind + 1]->asInt(), false, releaseData );
				break;
			case P_OTSU_THRESHOLD:
			case P_ISODATA_THRESHOLD:
			case P_MAXENTROPY_THRESHOLD:
			case P_MOMENTS_THRESHOLD:
			case P_YEN_THRESHOLD:
			case P_RENYI_THRESHOLD:
			case P_SHANBHAG_THRESHOLD:
			case P_INTERMODES_THRESHOLD:
			case P_HUANG_THRESHOLD:
			case P_LI_THRESHOLD:
			case P_KITTLERILLINGWORTH_THRESHOLD:
			case P_TRIANGLE_THRESHOLD:
			case P_MINIMUM_THRESHOLD:
				computeParamFree<T>( curImage, fid, results, releaseData );
				break;
			case P_CONNECTED_THRESHOLD:
				computeConnThr<T>( image, curImage, results, params[pind]->asInt(), params[pind + 1]->asInt(), releaseData );
				break;
			case P_CONFIDENCE_CONNECTED:
				computeConfiConn<T>( image, curImage, results, params[pind]->asInt(), params[pind + 1]->asFloat(), params[pind + 2]->asInt(), releaseData );
				break;
			case P_NEIGHBORHOOD_CONNECTED:
				computeNeighbConn<T>( image, curImage, results, params[pind]->asInt(), params[pind + 1]->asInt(), params[pind + 2]->asInt(), releaseData );
				break;
			case P_MULTIPLE_OTSU:
				computeMultiOtsu<T>( curImage, fid, results, params[pind]->asInt(), params[pind + 1]->asInt(), releaseData );
				break;
			case P_REMOVE_SURROUNDING:
				computeRemoveSurrounding<T>( curImage, fid, results, releaseData );
				break;
			case P_GRAD_ANISO_DIFF_SMOOTH:
				computeGradAnisoDiffSmooth<T>( curImage, fid, results, params[pind]->asInt(), params[pind + 1]->asFloat(), params[pind + 2]->asFloat(), releaseData );
				break;
			case P_CURV_ANISO_DIFF_SMOOTH:
				computeCurvAnisoDiffSmooth<T>( curImage, fid, results, params[pind]->asInt(), params[pind + 1]->asFloat(), params[pind + 2]->asFloat(), releaseData );
				break;
			case P_RECURSIVE_GAUSS_SMOOTH:
				computeRecursiveGaussSmooth<T>( curImage, fid, results, params[pind]->asFloat(), releaseData );
				break;
			case P_BILATERAL_SMOOTH:
				computeBilateralSmooth<T>( curImage, fid, results, params[pind]->asFloat(), params[pind + 1]->asFloat(), releaseData );
				break;
			case P_CURV_FLOW_SMOOTH:
				computeCurvFlowSmooth<T>( curImage, fid, results, params[pind]->asInt(), params[pind + 1]->asFloat(), releaseData );
				break;
			case P_MEDIAN_SMOOTH:
				computeMedianSmooth<T>( curImage, fid, results, params[pind]->asInt(), releaseData );
				break;
			case P_ISOX_THRESHOLD:
				computeIsoXThreshold<T>( curImage, fid, results, params[pind]->asInt(), releaseData );
				break;
			case P_FHW_THRESHOLD:
				computeFhwThreshold<T>( curImage, fid, results, params[pind]->asInt(), params[pind + 1]->asInt(), releaseData );
				break;
			case P_CREATE_SURROUNDING:
				computeCreateSurrounding<T>( curImage, fid, results, params[pind]->asFloat(), releaseData );
				break;
		}
		results.elapsedTime += t.elapsed();
		curImage = results.maskImage;
		pind += FilterIdToParamList[fid].size();
	}
}

void iARunBatchThread::Init(iAFeatureAnalyzerComputationModuleInterface * pmi, QString datasetFolder,
	bool rbNewPipelineDataNoPores, bool rbNewPipelineData)
{
	m_pmi = pmi;
	m_datasetsDescrFile = datasetFolder + "/" + "DatasetDescription.csv";
	m_rbNewPipelineDataNoPores = rbNewPipelineDataNoPores;
	m_rbNewPipelineData = rbNewPipelineData;

	m_dsDescr.clear();
	m_dsDescr.setRowCount( 0 ); m_dsDescr.setColumnCount( 0 );
	iACSVToQTableWidgetConverter::loadCSVFile( m_datasetsDescrFile, &m_dsDescr );
	m_datasetGTs.clear();
	for( int i = 1; i < m_dsDescr.rowCount(); i++ )
		m_datasetGTs[m_dsDescr.item( i, gtDatasetColInd )->text()] = m_dsDescr.item( i, gtGTSegmColumnIndex )->text();
}

void iARunBatchThread::executeNewBatches( QTableWidget & settingsCSV, QMap<int, bool> & isBatchNew )
{
	if ( m_rbNewPipelineDataNoPores || m_rbNewPipelineData )
	{
		int batchesToCompute = 0;
		for ( int row = 1; row < settingsCSV.rowCount(); ++row ) // 1 because we skip header
			if ( isBatchNew[row] ) ++batchesToCompute;

		emit totalProgress( 0 );
		emit batchProgress( 0 );
		emit currentBatch( "Batch Progress" );

		for ( int row = 1; row < settingsCSV.rowCount(); ++row ) // 1 because we skip header
		{
			if ( isBatchNew[row] )
			{
				emit currentBatch( QString("Batch %1 Progress").arg(row));
				//get files and directory paths
				QString algName, datasetName, batchesDir, batchDir;
				getAlgorithmAndDatasetNames( &settingsCSV, row, &algName, &datasetName );
				QList<PorosityFilterID> filterIds = parseFiltersFromString( algName );
				datasetName = m_pmi->DatasetFolder() + "/" + datasetName;
				batchesDir = m_pmi->ResultsFolder() + "/" + dirFromAlgAndDataset( &settingsCSV, row );
				QDir bsDir( batchesDir );
				bsDir.setFilter( QDir::AllDirs );
				batchDir = "batch" + QString::number( bsDir.entryList().size() - 1 );
				QDir( batchesDir ).mkdir( batchDir );
				batchDir = batchesDir + "/" + batchDir;

				//compute batch, mask.csv file, mask.mhd.csv (pore chars)
				executeBatch( filterIds, datasetName, batchDir, &settingsCSV, row );
				generateMasksCSVFile( batchDir, batchesDir );
			}
			emit totalProgress( row * 100.0 / batchesToCompute );
		}
	}
}

void iARunBatchThread::initRunsCSVFile( QTableWidget & runsCSV, QString batchDir, const QList<ParamNameType> & paramNames )
{
	int col = 0;
	QFile runsCSVFile( batchDir + "/runs.csv" );
	if (runsCSVFile.exists())
	{
		iACSVToQTableWidgetConverter::loadCSVFile(runsCSVFile.fileName(), &runsCSV);
	}
	else
	{
		//Insert a header
		runsCSV.setRowCount( 1 );
		runsCSV.setColumnCount( runsCSVHeader.size() + paramNames.size() );
		for (const QString & l : runsCSVHeader)
		{
			runsCSV.setItem(0, col++, new QTableWidgetItem(l));
		}
		for (const ParamNameType & pnt : paramNames)
		{
			runsCSV.setItem(0, col++, new QTableWidgetItem(pnt.name()));
		}
	}
	iACSVToQTableWidgetConverter::saveToCSVFile( runsCSV, runsCSVFile.fileName() );
}

void iARunBatchThread::saveResultsToRunsCSV( RunInfo & results, QString masksDir, QTableWidget & runsCSV, bool success /*= true */ )
{
	int lastRow = runsCSV.rowCount(), col = 0;
	runsCSV.insertRow( lastRow );
	runsCSV.setItem( lastRow, col++, new QTableWidgetItem( results.startTime ) );
	runsCSV.setItem( lastRow, col++, new QTableWidgetItem( QString::number( results.elapsedTime ) ) );
	runsCSV.setItem( lastRow, col++, new QTableWidgetItem( QString::number( results.porosity ) ) );
	runsCSV.setItem( lastRow, col++, new QTableWidgetItem( QString::number( results.threshold ) ) );
	QString maskName = "mask" + QString::number( lastRow ) + ".mhd";
	QString maskFilename = "";
	if ( success )
		maskFilename = masksDir + "/" + maskName;
	runsCSV.setItem( lastRow, col++, new QTableWidgetItem( maskName ) );
	//dice metric
	runsCSV.setItem( lastRow, col++, new QTableWidgetItem( QString::number( results.falsePositiveRate ) ) );
	runsCSV.setItem( lastRow, col++, new QTableWidgetItem( QString::number( results.falseNegativeRate ) ) );
	runsCSV.setItem( lastRow, col++, new QTableWidgetItem( QString::number( results.dice ) ) );
	//avg feature chars
	runsCSV.setItem(lastRow, col++, new QTableWidgetItem(QString::number(results.featureCnt)));
	runsCSV.setItem(lastRow, col++, new QTableWidgetItem(QString::number(results.avgFeatureVol)));
	runsCSV.setItem(lastRow, col++, new QTableWidgetItem(QString::number(results.avgFeaturePhi)));
	runsCSV.setItem(lastRow, col++, new QTableWidgetItem(QString::number(results.avgFeatureTheta)));
	runsCSV.setItem(lastRow, col++, new QTableWidgetItem(QString::number(results.avgFeatureRoundness)));
	runsCSV.setItem(lastRow, col++, new QTableWidgetItem(QString::number(results.avgFeatureLength)));
	//input params
	for ( int i = 0; i < results.parameters.size(); ++i )
		runsCSV.setItem( lastRow, col++, new QTableWidgetItem( results.parameters[i] ) );

	iAITKIO::writeFile( maskFilename, results.maskImage, iAITKIO::ScalarType::CHAR, true );

	//Write mask image preview (png)
	try
	{
		MaskImageType * mask = dynamic_cast<MaskImageType*>( results.maskImage.GetPointer() );
		if (!mask)
		{
			throw itk::ExceptionObject("No mask!");
		}

		QFileInfo maskFI( maskFilename );
		if (!QDir(maskFI.absoluteDir()).mkdir(getMaskSliceDirName(maskFilename)))
		{
			throw std::runtime_error("Could not create directory for slices!");
		}

		//TODO: move to common or helpers, duplicated in DatasetInfo
		MaskImageType::SizeType size = mask->GetLargestPossibleRegion().GetSize();
		unsigned char * bufferPtr = (unsigned char *) mask->GetBufferPointer();
		unsigned long sliceSize = size[0] * size[1];
		for ( unsigned int sliceNumber = 0; sliceNumber < size[2]; ++sliceNumber )
		{
			unsigned char * sBufferPtr = bufferPtr + sliceSize * sliceNumber;
			QImage img( size[0], size[1], QImage::Format_Indexed8 );
			assert(size[1] <= static_cast<itk::SizeValueType>(std::numeric_limits<int>::max()));
			for ( int y = 0; static_cast<itk::SizeValueType>(y) < size[1]; ++y )
			{
				memcpy( img.scanLine( size[1] - y - 1 ), sBufferPtr + y*size[0], size[0] );//we invert Y-axis, because VTK and Qt have different Y axis directions
			}
			img.setColor( 0, qRgb( 0, 0, 0 ) );
			img.setColor( 1, qRgb( 255, 255, 255 ) );

			if (!img.save(getSliceFilename(maskFilename, sliceNumber)))
			{
				throw std::runtime_error("Could not save png!");
			}
		}
	}
	catch ( itk::ExceptionObject & err )
	{
		QString tolog = tr( "  %1 in File %2, Line %3" )
			.arg( err.GetDescription() )
			.arg( err.GetFile() )
			.arg( err.GetLine() );
		m_pmi->log( tolog );
	}
	catch ( std::exception const & e )
	{
		m_pmi->log( e.what() );
	}
}

void iARunBatchThread::executeBatch( const QList<PorosityFilterID> & filterIds, QString datasetName, QString batchDir, QTableWidget * settingsCSV, int row )
{
	QList<ParamNameType> paramsNameType;
	for (PorosityFilterID fid: filterIds)
	{
		paramsNameType.append(FilterIdToParamList[fid]);
	}
	int numParams = paramsNameType.size();

	QString masksDir = "masks";
	QDir( batchDir ).mkdir( masksDir );
	masksDir = batchDir + "/" + masksDir;

	bool randSampling = isRandomSampling( settingsCSV, row );

	QList<IParameterInfo*> params;
	for (int i = 0; i < numParams; i++)
	{
		params.push_back(getParameterInfo(paramsNameType.at(i), settingsCSV, row, 3 + i));
	}

	double totalNumSamples = 1.0;
	if( randSampling )
	{
		totalNumSamples = getNumRandomSamples( settingsCSV, row );
		randomlySampleParameters( params );
	}
	else
	{
		for (int i = 0; i < numParams; ++i)
		{
			totalNumSamples *= params[i]->numSamples;
		}
	}

	// initialize runsCSV data
	m_runsCSV.clear();
	initRunsCSVFile( m_runsCSV, batchDir, paramsNameType );
	// inintialize input datset
	iAITKIO::ScalarType scalarType;
	iAITKIO::PixelType pixelType;
	auto image = iAITKIO::readFile( datasetName, pixelType, scalarType, true);
	assert(pixelType == iAITKIO::PixelType::SCALAR);
	//GT image (make sure it is the same likne MaskImageType (CHAR))
	iAITKIO::ImagePointer gtMask;
	QString dsFN = QFileInfo( datasetName ).fileName();
	QString dsPath = QFileInfo( datasetName ).absolutePath();
	if( m_datasetGTs[dsFN] != "" )
	{
		iAITKIO::ScalarType maskScalarType;
		iAITKIO::PixelType  maskPixelType;
		QString gtMaskFile = dsPath + "/" + m_datasetGTs[dsFN];
		gtMask = iAITKIO::readFile( gtMaskFile, maskPixelType, maskScalarType, true);
		assert(maskPixelType == iAITKIO::PixelType::SCALAR);
	}
	emit batchProgress( 0 );

	for( int sampleNo = 0; sampleNo < totalNumSamples; ++sampleNo ) //iterate over parameters
	{
		while (m_pmi->ui()->rbPause->isChecked())
		{
			QCoreApplication::processEvents();
		}

		RunInfo results;
		//fill in parameters info
		for( int i = 0; i < numParams; ++i )
		{
			results.parameters.push_back( params[i]->asString() );
			results.parameterNames << params[i]->name;
		}

		bool success = true;
		results.elapsedTime = 0;	// reset elapsed time

		try
		{
			ITK_TYPED_CALL(runBatch, scalarType, filterIds, image, results, params);
			//calculate porosity
			MaskImageType * mask = dynamic_cast<MaskImageType*>(results.maskImage.GetPointer());
			MaskImageType * gtImage = dynamic_cast<MaskImageType*>(gtMask.GetPointer());
			results.porosity = calcPorosity( mask, results.surroundingVoxels );

			if (m_rbNewPipelineData)
			{
				QString currMaskFilePath = masksDir + "/mask" + QString::number(sampleNo + 1) + ".mhd";
				calcFeatureCharsForMask(results, currMaskFilePath);
			}

			//Dice metric, false positve error, false negative error
			if ( m_datasetGTs[dsFN] != "" )
			{
				MaskImageType::RegionType reg = mask->GetLargestPossibleRegion();
				long long size = static_cast<long long>(reg.GetSize()[0]) * reg.GetSize()[1] * reg.GetSize()[2];
				size_t tp = 0, fn = 0, fp = 0, tn = 0;
				MaskImageType::PixelType gt, m;

#pragma omp parallel for reduction(+:tp,fn,fp,tn)
				for (long long i = 0; i < size; ++i )
				{
					gt = gtImage->GetBufferPointer()[i];
					m = mask->GetBufferPointer()[i];
					if (gt == 1 && m == 1) tp = tp + 1;
					if (gt == 1 && m == 0) fn = fn + 1;
					if (gt == 0 && m == 1) fp = fp + 1;
					if (gt == 0 && m == 0) tn = tn + 1;
				}

				results.falseNegativeRate = static_cast<float>(fn) / (tp + fn);
				results.falsePositiveRate = static_cast<float>(fp) / (tn + fp);
				results.dice = 2 * static_cast<float>(tp) / (2 * tp + fp + fn);
				emit batchProgress( ( sampleNo + 1 ) * 100 / totalNumSamples );
			}
		}
		catch( itk::ExceptionObject &excep )
		{
			m_pmi->log( tr( "Filter run terminated unexpectedly." ) );
			m_pmi->log( tr( "  %1 in File %2, Line %3" ).arg( excep.GetDescription() )
				.arg( excep.GetFile() )
				.arg( excep.GetLine() ) );
			success = false;
		}
		catch( ... )
		{
			m_pmi->log( tr( "Filter run terminated unexpectedly with unknown exception." ) );
			success = false;
		}

		try
		{
			saveResultsToRunsCSV( results, masksDir, m_runsCSV, success );
		}
		catch( itk::ExceptionObject &excep )
		{
			m_pmi->log( tr( "Writing the mask terminated unexpectedly." ) );
			m_pmi->log( tr( "  %1 in File %2, Line %3" ).arg( excep.GetDescription() )
				.arg( excep.GetFile() )
				.arg( excep.GetLine() ) );
		}

		if (randSampling)
		{
			randomlySampleParameters(params);
		}
		else
		{
			incrementParameterSet(params);
		}
	}
	iACSVToQTableWidgetConverter::saveToCSVFile( m_runsCSV, batchDir + "/runs.csv" );
}

void iARunBatchThread::updateComputerCSVFile( QTableWidget & settingsCSV )
{
	m_computerCSVData.clear();
	int col = 0, row = 0;
	QFile computerCSVFile( m_pmi->ResultsFolder() + "/" + m_pmi->ComputerName() + ".csv" );
	if (computerCSVFile.exists())
	{
		iACSVToQTableWidgetConverter::loadCSVFile(computerCSVFile.fileName(), &m_computerCSVData);
	}
	else
	{
		m_pmi->log( "\tCreating new computer CSV file" );
		//Insert a header
		m_computerCSVData.setRowCount( 1 );
		m_computerCSVData.setColumnCount( computerCSVHeader.size() );
		for (const QString & l : computerCSVHeader)
		{
			m_computerCSVData.setItem(0, col++, new QTableWidgetItem(l));
		}
	}

	//Update computer CSV with new entries
	for( row = 1; row < settingsCSV.rowCount(); ++row ) // 1 because we skip header
	{
		col = 0;
		QString algName, datasetName, dirName, batchesFile;
		getAlgorithmAndDatasetNames( &settingsCSV, row, &algName, &datasetName );
		dirName = dirFromAlgAndDataset( algName, datasetName );
		batchesFile = m_pmi->ResultsFolder() + "/" + dirName + "/batches.csv";

		if( existsBatchesRecord( &m_computerCSVData, algName, datasetName ) )
			continue;

		m_pmi->log( "\tAdding batches for " + dirName );
		QDir( m_pmi->ResultsFolder() ).mkdir( dirName );
		int lastRow = m_computerCSVData.rowCount();
		m_computerCSVData.insertRow( lastRow );
		m_computerCSVData.setItem( lastRow, col++, new QTableWidgetItem( m_pmi->ComputerName() ) );
		m_computerCSVData.setItem( lastRow, col++, new QTableWidgetItem( m_pmi->CpuBrand() ) );
		m_computerCSVData.setItem( lastRow, col++, new QTableWidgetItem( algName ) ); //algorithm name
		m_computerCSVData.setItem( lastRow, col++, new QTableWidgetItem( datasetName ) ); //dataset name
		m_computerCSVData.setItem( lastRow, col++, new QTableWidgetItem( dirName ) );
	}
	iACSVToQTableWidgetConverter::saveToCSVFile( m_computerCSVData, computerCSVFile.fileName() );
}

void iARunBatchThread::updateBatchesCSVFiles( QTableWidget & settingsCSV, QMap<int, bool> & isBatchNew )
{
	//update batches files
	for( int row = 1; row < settingsCSV.rowCount(); ++row ) // 1 because we skip header
	{
		QString dirName, batchesFile;
		getBatchesDirectoryAndFilename( &settingsCSV, row, m_pmi->ResultsFolder(), &dirName, &batchesFile );
		//fill in batches.csv
		isBatchNew[row] = updateBatchesCSVFile( settingsCSV, row, batchesFile );
		if( isBatchNew[row] )
			m_pmi->log( "\tAdded new batch in " + dirName );
	}
}

bool iARunBatchThread::updateBatchesCSVFile( QTableWidget & settingsCSV, int row, QString batchesFile )
{
	QFile batches( batchesFile );

	m_batchesData.clear();
	if( batches.exists() )
	{
		iACSVToQTableWidgetConverter::loadCSVFile( batches.fileName(), &m_batchesData );
		if( existsInBatches( settingsCSV, row, m_batchesData ) )
			return false;
	}
	else
	{
		m_pmi->log( "\tCreated new batches CSV file" );
		//Insert a header
		m_batchesData.setRowCount( 1 );
		m_batchesData.setColumnCount( settingsCSV.columnCount() - 2 );
		for( int col = 0; col < m_batchesData.columnCount(); ++col )
		{
			if( !settingsCSV.item( 0, col + 2 ) )
				break;
			m_batchesData.setItem( 0, col, new QTableWidgetItem( settingsCSV.item( 0, col + 2 )->text() ) );
		}
	}

	// data
	int lastRow = m_batchesData.rowCount();
	m_batchesData.insertRow( lastRow );
	for( int col = 0; col < m_batchesData.columnCount(); ++col )
	{
		if( !settingsCSV.item( row, col + 2 ) )
			break;
		QString str = settingsCSV.item( row, col + 2 )->text();
		m_batchesData.setItem( lastRow, col, new QTableWidgetItem( str ) );
	}

	iACSVToQTableWidgetConverter::saveToCSVFile( m_batchesData, batchesFile );
	return true;
}

void iARunBatchThread::generateMasksCSVFile( QString batchDir, QString batchesDir )
{
	m_masksData.clear();
	m_masksData.setColumnCount( 1 );
	m_masksData.setRowCount( 0 );
	QDirIterator dirIt( batchesDir, QDirIterator::Subdirectories );
	while ( dirIt.hasNext() )
	{
		dirIt.next();
		QFileInfo fi = QFileInfo( dirIt.filePath() );
		if ( fi.isFile() )
			if ( QString::compare( fi.suffix(), "mhd", Qt::CaseInsensitive ) == 0 )
			{
				QFileInfo maskCSVFile( fi.absoluteFilePath() + ".csv" );
				if ( maskCSVFile.exists() )
					continue;
				int lastRow = m_masksData.rowCount();
				m_masksData.insertRow( lastRow );
				m_masksData.setItem( lastRow, 0, new QTableWidgetItem( fi.absoluteFilePath() ) );
			}
	}
	iACSVToQTableWidgetConverter::saveToCSVFile( m_masksData, batchDir + "/" + "masks.csv" );
	m_pmi->log( tr( "File masks.csv created in %1" ).arg(batchDir) );
}

void iARunBatchThread::run()
{
	m_settingsCSV.clear();
	iACSVToQTableWidgetConverter::loadCSVFile( m_pmi->CSVFile(), &m_settingsCSV );
	if( !m_settingsCSV.rowCount() )
		return;
	QMap<int, bool> isBatchNew;

	m_pmi->log( "Updating computer CSV file" );
	updateComputerCSVFile( m_settingsCSV );

	m_pmi->log( "Updating batches CSV files" );
	updateBatchesCSVFiles( m_settingsCSV, isBatchNew );

	m_pmi->log( "Executing new batches" );
	executeNewBatches( m_settingsCSV, isBatchNew );
}

void iARunBatchThread::calcFeatureCharsForMask(RunInfo& results, QString currMaskFilePath)
{
	MaskImageType* mask = dynamic_cast<MaskImageType*>(results.maskImage.GetPointer());

	// Label image
	typedef itk::Image<long, iAITKIO::Dim>  LabeledImageType;
	typedef itk::ConnectedComponentImageFilter<MaskImageType, LabeledImageType> ConnectedComponentImageFilterType;
	auto connectedComponents = ConnectedComponentImageFilterType::New();
	connectedComponents->SetInput(mask);
	connectedComponents->FullyConnectedOn();
	connectedComponents->Update();

	// Save labeled image
	QString labeledMaskName = currMaskFilePath;
	labeledMaskName.insert(currMaskFilePath.lastIndexOf("."), "_labeled");
	iAITKIO::writeFile(labeledMaskName, connectedComponents->GetOutput(), iAITKIO::ScalarType::LONG, true);

	auto csvFileName = currMaskFilePath.append(".csv");
	auto computeCharacteristics = iAFilterRegistry::filter("Calculate Feature Characteristics");
	if (!computeCharacteristics)
	{
		LOG(lvlError, "Calculate Feature Characteristics Filter is not available!");
		throw std::runtime_error("Calculate Feature Characteristics Filter is not available!");
	}
	auto img = std::make_shared<iAImageData>(connectedComponents->GetOutput());
	computeCharacteristics->addInput(img);
	QVariantMap paramValues;
	paramValues["Output CSV filename"] = csvFileName;
	paramValues["Calculate Feret Diameter"] = false;
	paramValues["Calculate roundness"] = false;
	paramValues["Calculate advanced void parameters"] = false;
	paramValues["Calculate averages"] = true;
	if (!computeCharacteristics->run(paramValues))
	{
		LOG(lvlError, "Calculate Feature Characteristics Filter failed!");
		throw std::runtime_error("Calculate Feature Characteristics Filter failed!");
	}
	auto outputValues = computeCharacteristics->outputValues();
	results.featureCnt = static_cast<long>(outputValues["Number of objects"].toULongLong());
	results.avgFeatureVol = outputValues["Volume average"].toDouble();
	results.avgFeaturePhi = outputValues["Phi average"].toDouble();
	results.avgFeatureTheta = outputValues["Theta average"].toDouble();
	results.avgFeatureRoundness = outputValues["Roundness average"].toDouble();
	results.avgFeatureLength = outputValues["Length average"].toDouble();
}
