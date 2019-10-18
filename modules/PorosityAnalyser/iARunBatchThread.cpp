/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iARunBatchThread.h"

#include "iACSVToQTableWidgetConverter.h"
#include "iAPorosityAnalyserModuleInterface.h"

#include <defines.h>
#include <iATypedCallHelper.h>
#include <io/iAITKIO.h>

// from Maximum Distance Toolkit
#include <itkMaximumDistance.h>

#include <itkAndImageFilter.h>
#include <itkBilateralImageFilter.h>
#include <itkBinaryContourImageFilter.h>
#include <itkBinaryThresholdImageFilter.h>
#include <itkCastImageFilter.h>
#include <itkConfidenceConnectedImageFilter.h>
#include <itkConnectedComponentImageFilter.h>
#include <itkConnectedThresholdImageFilter.h>
#include <itkCurvatureAnisotropicDiffusionImageFilter.h>
#include <itkCurvatureFlowImageFilter.h>
#include <itkExtractImageFilter.h>
#include <itkGradientAnisotropicDiffusionImageFilter.h>
#include <itkGradientMagnitudeImageFilter.h>
#include <itkHuangThresholdImageFilter.h>
#include <itkImageDuplicator.h>
#include <itkImageFileWriter.h>
#include <itkImageRegionConstIterator.h>
#include <itkImageToHistogramFilter.h>
#include <itkIntermodesThresholdImageFilter.h>
#include <itkInvertIntensityImageFilter.h>
#include <itkIsoDataThresholdImageFilter.h>
#include <itkKittlerIllingworthThresholdImageFilter.h>
#include <itkLabelGeometryImageFilter.h>
#include <itkLabelImageToShapeLabelMapFilter.h>
#include <itkLiThresholdImageFilter.h>
#include <itkMaximumEntropyThresholdImageFilter.h>
#include <itkMedianImageFilter.h>
#include <itkMinimumMaximumImageCalculator.h>
#include <itkMomentsThresholdImageFilter.h>
#include <itkMorphologicalWatershedImageFilter.h>
#include <itkNeighborhoodConnectedImageFilter.h>
#include <itkOtsuMultipleThresholdsImageFilter.h>
#include <itkOtsuThresholdImageFilter.h>
#include <itkRecursiveGaussianImageFilter.h>
#include <itkRelabelComponentImageFilter.h>
#include <itkRenyiEntropyThresholdImageFilter.h>
#include <itkRescaleIntensityImageFilter.h>
#include <itkRobustAutomaticThresholdImageFilter.h>
#include <itkShanbhagThresholdImageFilter.h>
#include <itkThresholdImageFilter.h>
#include <itkTriangleThresholdImageFilter.h>
#include <itkYenThresholdImageFilter.h>

#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QMessageBox>
#include <QTime>

//openMP
#ifndef __APPLE__
#ifndef __MACOSX
#include <omp.h>///TODO: gcc include omp //omp.h works with gcc 4.6
#endif
#endif

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
	ImagePointer maskImage;
	ImagePointer surroundingMaskImage;
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
void computeBinaryThreshold( ImagePointer & image, RunInfo & results, float upThr, bool releaseData = false )
{
	typedef itk::Image< T, DIM >   InputImageType;
	typedef itk::BinaryThresholdImageFilter <InputImageType, MaskImageType> BinaryThresholdImageFilterType;
	typename BinaryThresholdImageFilterType::Pointer binaryThresholdFilter = BinaryThresholdImageFilterType::New();
	InputImageType * input = dynamic_cast<InputImageType*>(image.GetPointer());

	//Use duplicator filter because thresholding is in-place
	typedef itk::ImageDuplicator< InputImageType > DuplicatorType;
	typename DuplicatorType::Pointer duplicator = DuplicatorType::New();
	duplicator->SetInputImage( input );
	duplicator->Update();

	binaryThresholdFilter->SetLowerThreshold( 0 );
	binaryThresholdFilter->SetUpperThreshold( upThr );
	binaryThresholdFilter->SetInsideValue( 1 );
	binaryThresholdFilter->SetOutsideValue( 0 );
	binaryThresholdFilter->SetInput( duplicator->GetOutput() );

	binaryThresholdFilter->Update();
	results.maskImage = binaryThresholdFilter->GetOutput();
	results.maskImage->Modified();
	if( releaseData )
		binaryThresholdFilter->ReleaseDataFlagOn();
}

template<class T>
void computeGeneralThreshold(ImagePointer & image, RunInfo & results, float lwThr, float upThr, bool releaseData = false)
{
	typedef itk::Image< T, DIM >   InputImageType;
	typedef itk::BinaryThresholdImageFilter <InputImageType, MaskImageType> BinaryThresholdImageFilterType;
	typename BinaryThresholdImageFilterType::Pointer binaryThresholdFilter = BinaryThresholdImageFilterType::New();
	InputImageType * input = dynamic_cast<InputImageType*>(image.GetPointer());

	//Use duplicator filter because thresholding is in-place
	typedef itk::ImageDuplicator< InputImageType > DuplicatorType;
	typename DuplicatorType::Pointer duplicator = DuplicatorType::New();
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
void computeRatsThreshold( ImagePointer & image, RunInfo & results, float ratsThr, bool releaseData = false )
{
	typedef typename itk::Image< T, DIM >   InputImageType;
	typedef typename itk::Image< float, DIM >   GradientImageType;
	InputImageType * input = dynamic_cast<InputImageType*>(image.GetPointer());

	//Use duplicator filter because thresholding is in-place
	typedef itk::ImageDuplicator< InputImageType > DuplicatorType;
	typename DuplicatorType::Pointer duplicator = DuplicatorType::New();
	duplicator->SetInputImage( input );
	duplicator->Update();

	typedef itk::GradientMagnitudeImageFilter< InputImageType, GradientImageType > GMFType;
	typename GMFType::Pointer gmfilter = GMFType::New();
	gmfilter->SetInput( duplicator->GetOutput() );
	gmfilter->Update();

	typedef typename itk::RobustAutomaticThresholdImageFilter < InputImageType, GradientImageType, MaskImageType > RATIFType;
	typename RATIFType::Pointer ratsFilter = RATIFType::New();
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
void computeMorphWatershed( ImagePointer & image, RunInfo & results, float level, int fullyConnected, bool meyer, bool releaseData = false )
{
	typedef itk::Image< T, DIM >   InputImageType;
	typedef typename itk::Image< float, DIM >   GradientImageType;
	typedef itk::Image<unsigned long, 3>   LabelImageType;
	InputImageType * input = dynamic_cast<InputImageType*>(image.GetPointer());

	//Use duplicator filter because thresholding is in-place
	typedef itk::ImageDuplicator< InputImageType > DuplicatorType;
	typename DuplicatorType::Pointer duplicator = DuplicatorType::New();
	duplicator->SetInputImage( input );
	duplicator->Update();

	// Gradient Magnitude
	typedef itk::GradientMagnitudeImageFilter< InputImageType, GradientImageType > GMFType;
	typename GMFType::Pointer gmfilter = GMFType::New();
	gmfilter->SetInput( duplicator->GetOutput() );
	gmfilter->Update();

	// Morphological Watershed
	typedef itk::MorphologicalWatershedImageFilter<GradientImageType, LabelImageType> MorphologicalWatershedFilterType;
	typename MorphologicalWatershedFilterType::Pointer mWSFilter = MorphologicalWatershedFilterType::New();

	if( meyer )
		mWSFilter->MarkWatershedLineOn();
	else
		mWSFilter->MarkWatershedLineOff();

	mWSFilter->SetFullyConnected( fullyConnected );
	mWSFilter->SetLevel( level );
	mWSFilter->SetInput( gmfilter->GetOutput() );
	mWSFilter->Update();

	// Relabel Watershed Result (background = #1label)
	typedef itk::RelabelComponentImageFilter<LabelImageType, LabelImageType> RelabelConnectedComponentFilterType;
	typename RelabelConnectedComponentFilterType::Pointer relabelFilter = RelabelConnectedComponentFilterType::New();
	relabelFilter->SetInput( mWSFilter->GetOutput() );
	relabelFilter->Update();

	// Binary Threshold
	typedef itk::BinaryThresholdImageFilter <LabelImageType, MaskImageType> BinaryThresholdImageFilterType;
	typename BinaryThresholdImageFilterType::Pointer binaryThresholdFilter = BinaryThresholdImageFilterType::New();
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
void computeParamFree( ImagePointer & image, PorosityFilterID filterId, RunInfo & results, bool releaseData = false )
{
	typedef itk::Image< T, DIM >   InputImageType;
	InputImageType * input = dynamic_cast<InputImageType*>(image.GetPointer());

	//Use duplicator filter because thresholding is in-place
	typedef itk::ImageDuplicator< InputImageType > DuplicatorType;
	typename DuplicatorType::Pointer duplicator = DuplicatorType::New();
	duplicator->SetInputImage( input );
	duplicator->Update();

	typedef itk::HistogramThresholdImageFilter<InputImageType, MaskImageType> parameterFreeThrFilterType;
	typename parameterFreeThrFilterType::Pointer filter;

	switch( filterId )
	{
		case P_OTSU_THRESHOLD:
			typedef itk::OtsuThresholdImageFilter <InputImageType, MaskImageType> OtsuFilterType;
			filter = OtsuFilterType::New();
			break;

		case P_ISODATA_THRESHOLD:
			typedef itk::IsoDataThresholdImageFilter <InputImageType, MaskImageType> IsoDataFilterType;
			filter = IsoDataFilterType::New();
			break;

		case P_MAXENTROPY_THRESHOLD:
			typedef itk::MaximumEntropyThresholdImageFilter <InputImageType, MaskImageType> MaximumEntropyFilterType;
			filter = MaximumEntropyFilterType::New();
			break;

		case P_MOMENTS_THRESHOLD:
			typedef itk::MomentsThresholdImageFilter <InputImageType, MaskImageType> MomentsFilterType;
			filter = MomentsFilterType::New();
			break;

		case P_YEN_THRESHOLD:
			typedef itk::YenThresholdImageFilter <InputImageType, MaskImageType> YenFilterType;
			filter = YenFilterType::New();
			break;

		case P_RENYI_THRESHOLD:
			typedef itk::RenyiEntropyThresholdImageFilter <InputImageType, MaskImageType> RenyiFilterType;
			filter = RenyiFilterType::New();
			break;

		case P_SHANBHAG_THRESHOLD:
			typedef itk::ShanbhagThresholdImageFilter <InputImageType, MaskImageType> ShanbhagFilterType;
			filter = ShanbhagFilterType::New();
			break;

		case P_INTERMODES_THRESHOLD:
			typedef itk::IntermodesThresholdImageFilter <InputImageType, MaskImageType> IntermodesFilterType;
			filter = IntermodesFilterType::New();
			break;

		case P_HUANG_THRESHOLD:
			typedef itk::HuangThresholdImageFilter <InputImageType, MaskImageType> HuangFilterType;
			filter = HuangFilterType::New();
			break;

		case P_LI_THRESHOLD:
			typedef itk::LiThresholdImageFilter <InputImageType, MaskImageType> LiFilterType;
			filter = LiFilterType::New();
			break;

		case P_KITTLERILLINGWORTH_THRESHOLD:
			typedef itk::KittlerIllingworthThresholdImageFilter <InputImageType, MaskImageType> KittlerIllingworthFilterType;
			filter = KittlerIllingworthFilterType::New();
			break;

		case P_TRIANGLE_THRESHOLD:
			typedef itk::TriangleThresholdImageFilter <InputImageType, MaskImageType> TriangleFilterType;
			filter = TriangleFilterType::New();
			break;

		case P_MINIMUM_THRESHOLD:
		{
			typedef itk::IntermodesThresholdImageFilter <InputImageType, MaskImageType> MinimumFilterType;
			typename MinimumFilterType::Pointer minimumFilter = MinimumFilterType::New();
			minimumFilter->SetUseInterMode( false );
			filter = minimumFilter;
			break;
		}
	}

	filter->SetInput( duplicator->GetOutput() );
	filter->Update();
	
	// Binary Threshold (fixes the no slice png image issue)
	typedef itk::BinaryThresholdImageFilter <MaskImageType, MaskImageType> BinaryThresholdImageFilterType;
	typename BinaryThresholdImageFilterType::Pointer binaryThresholdFilter = BinaryThresholdImageFilterType::New();
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
void computeConnThr( ImagePointer & inputImage, ImagePointer & seedImage, RunInfo & results, int loConnThr, int upConnThr, bool releaseData = false )
{
	typedef itk::Image< T, DIM >   InputImageType;
	const InputImageType * input = dynamic_cast<InputImageType*>(inputImage.GetPointer());

	//we need duplicator because stupid filter utilizes in-place thresholding in its guts
	typedef itk::ImageDuplicator< InputImageType > DuplicatorType;
	typename DuplicatorType::Pointer duplicator = DuplicatorType::New();
	duplicator->SetInputImage( input );
	duplicator->Update();

	typedef itk::ConnectedThresholdImageFilter< InputImageType, MaskImageType > ConnThrFilterType;
	typename ConnThrFilterType::Pointer connThrfilter = ConnThrFilterType::New();
	connThrfilter->SetInput( duplicator->GetOutput() );
	connThrfilter->SetLower( loConnThr );
	connThrfilter->SetUpper( upConnThr );
	connThrfilter->SetReplaceValue( 1 );

	MaskImageType * seed = dynamic_cast<MaskImageType*>(seedImage.GetPointer());
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
void computeConfiConn( ImagePointer & inputImage, ImagePointer & seedImage, RunInfo & results, int initNeighbRadius, float multip, int numbIter, bool releaseData = false )
{
	typedef itk::Image< T, DIM >   InputImageType;
	InputImageType * input = dynamic_cast<InputImageType*>(inputImage.GetPointer());

	//Use duplicator filter because thresholding is in-place
	typedef itk::ImageDuplicator< InputImageType > DuplicatorType;
	typename DuplicatorType::Pointer duplicator = DuplicatorType::New();
	duplicator->SetInputImage( input );
	duplicator->Update();

	typedef itk::ConfidenceConnectedImageFilter< InputImageType, MaskImageType > ConfiConnFilterType;
	typename ConfiConnFilterType::Pointer confiConnFilter = ConfiConnFilterType::New();
	confiConnFilter->SetInput( duplicator->GetOutput() );
	confiConnFilter->SetInitialNeighborhoodRadius( initNeighbRadius );
	confiConnFilter->SetMultiplier( multip );
	confiConnFilter->SetNumberOfIterations( numbIter );
	confiConnFilter->SetReplaceValue( 1 );

	MaskImageType * seed = dynamic_cast<MaskImageType*>(seedImage.GetPointer());
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
void computeNeighbConn( ImagePointer & inputImage, ImagePointer & seedImage, RunInfo & results, int loConnThr, int upConnThr, int neighbRadius, bool releaseData = false )
{
	typedef itk::Image< T, DIM >   InputImageType;
	InputImageType * input = dynamic_cast<InputImageType*>(inputImage.GetPointer());

	// Use duplicator filter because thresholding is in-place
	typedef itk::ImageDuplicator< InputImageType > DuplicatorType;
	typename DuplicatorType::Pointer duplicator = DuplicatorType::New();
	duplicator->SetInputImage( input );
	duplicator->Update();

	typename InputImageType::SizeType	radius;
	radius[0] = neighbRadius;
	radius[1] = neighbRadius;
	radius[2] = neighbRadius;

	typedef itk::NeighborhoodConnectedImageFilter< InputImageType, MaskImageType > NeighbConnFilterType;
	typename NeighbConnFilterType::Pointer neighbConnfilter = NeighbConnFilterType::New();
	neighbConnfilter->SetInput( duplicator->GetOutput() );
	neighbConnfilter->SetLower( loConnThr );
	neighbConnfilter->SetUpper( upConnThr );
	neighbConnfilter->SetRadius( radius );
	neighbConnfilter->SetReplaceValue( 1 );

	MaskImageType * seed = dynamic_cast<MaskImageType*>(seedImage.GetPointer());
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
void computeMultiOtsu( ImagePointer & image, PorosityFilterID filterId, RunInfo & results, int NbOfThr, int ValleyEmphasis, bool releaseData = false )
{
	typedef itk::Image< T, DIM >   InputImageType;
	InputImageType * input = dynamic_cast<InputImageType*>(image.GetPointer());

	// Use duplicator filter because thresholding is in-place
	typedef itk::ImageDuplicator< InputImageType > DuplicatorType;
	typename DuplicatorType::Pointer duplicator = DuplicatorType::New();
	duplicator->SetInputImage( input );
	duplicator->Update();

	typedef typename itk::OtsuMultipleThresholdsImageFilter < InputImageType, MaskImageType > multiOtsuFilterType;
	typename multiOtsuFilterType::Pointer multiOtsufilter = multiOtsuFilterType::New();
	multiOtsufilter->SetNumberOfThresholds( NbOfThr );
	multiOtsufilter->SetValleyEmphasis( ValleyEmphasis );
	multiOtsufilter->SetInput( duplicator->GetOutput() );
	multiOtsufilter->Update();

	typename multiOtsuFilterType::ThresholdVectorType thresholds = multiOtsufilter->GetThresholds();

	// Binary Threshold
	typedef itk::BinaryThresholdImageFilter <MaskImageType, MaskImageType> BinaryThresholdImageFilterType;
	typename BinaryThresholdImageFilterType::Pointer binaryThresholdFilter = BinaryThresholdImageFilterType::New();
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
void computeCreateSurrounding( ImagePointer & image, PorosityFilterID filterId, RunInfo & results, float upSurrThr, bool releaseData = false )
{
	// Use this filter together with computeRemoveSurrounding
	typedef itk::Image< T, DIM >   InputImageType;
	InputImageType * input = dynamic_cast<InputImageType*>( image.GetPointer() );

	// We need duplicator because stupid filter utilizes in-place thresholding in its guts
	typedef itk::ImageDuplicator< InputImageType > DuplicatorType;
	typename DuplicatorType::Pointer duplicator1 = DuplicatorType::New();
	duplicator1->SetInputImage( input );
	duplicator1->Update();

	// Defines a dummy image (input size) with a white core (input size without surface border voxels )
	MaskImageType::Pointer dummyImage = MaskImageType::New();
	MaskImageType::RegionType region;
	MaskImageType::RegionType::IndexType start;
	start[0] = 0; start[1] = 0; start[2] = 0;
	region.SetSize( input->GetLargestPossibleRegion().GetSize() );
	region.SetIndex( start );
	dummyImage->SetRegions( region );
	const MaskImageType::SpacingType& out_spacing = input->GetSpacing();
	const MaskImageType::PointType& inputOrigin = input->GetOrigin();
	double outputOrigin[DIM];
	for ( unsigned int i = 0; i < DIM; i++ )
		outputOrigin[i] = inputOrigin[i];
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
	typedef itk::InvertIntensityImageFilter <MaskImageType> InvertIntensityImageFilterType;
	typename InvertIntensityImageFilterType::Pointer surfaceBorderMask = InvertIntensityImageFilterType::New();
	surfaceBorderMask->SetInput( dummyImage );
	surfaceBorderMask->SetMaximum( 1 );
	surfaceBorderMask->Update();

	// Calculates the surrounding mask
	typedef itk::ConnectedThresholdImageFilter< InputImageType, MaskImageType > ConnThrFilterType;
	typename ConnThrFilterType::Pointer connThrfilter = ConnThrFilterType::New();
	connThrfilter->SetInput( duplicator1->GetOutput() );
	connThrfilter->SetLower( 0 );
	connThrfilter->SetUpper( upSurrThr );
	connThrfilter->SetReplaceValue( 1 );
	typedef itk::ImageRegionConstIterator< MaskImageType > maskConstIteratorType;
	maskConstIteratorType dummyImgIt( surfaceBorderMask->GetOutput(), region );
	typedef itk::ImageRegionConstIterator< InputImageType > inputConstIteratorType;
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
void computeRemoveSurrounding( ImagePointer & image, PorosityFilterID filterId, RunInfo & results, bool releaseData = false )
{
	// Use this filter together with computeCreateSurrounding
	MaskImageType * surMask = dynamic_cast<MaskImageType*>( results.surroundingMaskImage.GetPointer() );
	typedef itk::ImageDuplicator< MaskImageType > DuplicatorType;
	typename DuplicatorType::Pointer surMaskDup = DuplicatorType::New();
	surMaskDup->SetInputImage( surMask );
	surMaskDup->Update();

	typedef itk::InvertIntensityImageFilter <MaskImageType> InvertIntensityImageFilterType;
	typename InvertIntensityImageFilterType::Pointer invertedIntensityMask = InvertIntensityImageFilterType::New();
	invertedIntensityMask->SetInput( surMaskDup->GetOutput() );
	invertedIntensityMask->SetMaximum( 1 );
	invertedIntensityMask->Update();

	MaskImageType * resMask = dynamic_cast<MaskImageType*>( results.maskImage.GetPointer() );
	typedef itk::ImageDuplicator< MaskImageType > DuplicatorType;
	typename DuplicatorType::Pointer resMaskDup = DuplicatorType::New();
	resMaskDup->SetInputImage( resMask );
	resMaskDup->Update();
	
	typedef itk::AndImageFilter <MaskImageType> AndImageFilterType;
	typename AndImageFilterType::Pointer andFilter = AndImageFilterType::New();
	andFilter->SetInput( 0, invertedIntensityMask->GetOutput() );
	andFilter->SetInput( 1, resMaskDup->GetOutput() );
	andFilter->Update();

	results.maskImage = andFilter->GetOutput();
	results.maskImage->Modified();
	if ( releaseData )
		andFilter->ReleaseDataFlagOn();
}

template<class T>
void computeGradAnisoDiffSmooth( ImagePointer & image, PorosityFilterID filterId, RunInfo & results, int nbOfIt, float timeStep, float condParam, bool releaseData = false )
{
	typedef typename itk::Image< T, DIM >   InputImageType;
	typedef typename itk::Image< float, DIM >   GADSFImageType;
	
	InputImageType * input = dynamic_cast<InputImageType*>( image.GetPointer() );
	
	typedef itk::ImageDuplicator< InputImageType > DuplicatorType;
	typename DuplicatorType::Pointer duplicator = DuplicatorType::New();
	duplicator->SetInputImage( input );
	duplicator->Update();
	
	typedef itk::GradientAnisotropicDiffusionImageFilter< InputImageType, GADSFImageType > GADSFType;
	typename GADSFType::Pointer gadsfilter = GADSFType::New();
	gadsfilter->SetInput( duplicator->GetOutput() );
	gadsfilter->SetNumberOfIterations( nbOfIt );
	gadsfilter->SetTimeStep( timeStep );
	gadsfilter->SetConductanceParameter( condParam );
	gadsfilter->Update();

	typedef itk::CastImageFilter< GADSFImageType, InputImageType > CastFilterType;
	typename CastFilterType::Pointer caster = CastFilterType::New();
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
void computeCurvAnisoDiffSmooth( ImagePointer & image, PorosityFilterID filterId, RunInfo & results, int nbOfIt, float timeStep, float condParam, bool releaseData = false )
{
	typedef typename itk::Image< T, DIM >   InputImageType;
	typedef typename itk::Image< float, DIM >   CADSFImageType;

	InputImageType * input = dynamic_cast<InputImageType*>( image.GetPointer() );

	typedef itk::ImageDuplicator< InputImageType > DuplicatorType;
	typename DuplicatorType::Pointer duplicator = DuplicatorType::New();
	duplicator->SetInputImage( input );
	duplicator->Update();

	typedef itk::CurvatureAnisotropicDiffusionImageFilter< InputImageType, CADSFImageType > CADSFType;
	typename CADSFType::Pointer cadsfilter = CADSFType::New();
	cadsfilter->SetInput( duplicator->GetOutput() );
	cadsfilter->SetNumberOfIterations( nbOfIt );
	cadsfilter->SetTimeStep( timeStep );
	cadsfilter->SetConductanceParameter( condParam );
	cadsfilter->Update();

	typedef itk::CastImageFilter< CADSFImageType, InputImageType > CastFilterType;
	typename CastFilterType::Pointer caster = CastFilterType::New();
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
void computeRecursiveGaussSmooth( ImagePointer & image, PorosityFilterID filterId, RunInfo & results, float sigma, bool releaseData = false )
{
	typedef typename itk::Image< T, DIM >   InputImageType;
	typedef typename itk::Image< float, DIM >   RGSFImageType;

	InputImageType * input = dynamic_cast<InputImageType*>( image.GetPointer() );

	typedef itk::ImageDuplicator< InputImageType > DuplicatorType;
	typename DuplicatorType::Pointer duplicator = DuplicatorType::New();
	duplicator->SetInputImage( input );
	duplicator->Update();

	typedef itk::RecursiveGaussianImageFilter<InputImageType, RGSFImageType > RGSFXType;
	typename RGSFXType::Pointer rgsfilterX = RGSFXType::New();
	typedef itk::RecursiveGaussianImageFilter<RGSFImageType, RGSFImageType > RGSFYZType;
	typename RGSFYZType::Pointer rgsfilterY = RGSFYZType::New();
	typename RGSFYZType::Pointer rgsfilterZ = RGSFYZType::New();
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

	typedef itk::CastImageFilter< RGSFImageType, InputImageType > CastFilterType;
	typename CastFilterType::Pointer caster = CastFilterType::New();
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
void computeBilateralSmooth( ImagePointer & image, PorosityFilterID filterId, RunInfo & results, float domainSigma, float rangeSigma, bool releaseData = false )
{
	typedef typename itk::Image< T, DIM >   InputImageType;
	typedef typename itk::Image< float, DIM >   BSFImageType;

	InputImageType * input = dynamic_cast<InputImageType*>( image.GetPointer() );

	typedef itk::ImageDuplicator< InputImageType > DuplicatorType;
	typename DuplicatorType::Pointer duplicator = DuplicatorType::New();
	duplicator->SetInputImage( input );
	duplicator->Update();

	typedef itk::BilateralImageFilter<InputImageType, BSFImageType > BSFType;
	typename BSFType::Pointer bsfilter = BSFType::New();
	bsfilter->SetInput( duplicator->GetOutput() );
	
	double domainSigmas[DIM];
	for ( unsigned int i = 0; i < DIM; i++ )
		domainSigmas[i] = domainSigma;
	
	bsfilter->SetDomainSigma( domainSigmas );
	bsfilter->SetRangeSigma( rangeSigma );
	bsfilter->Update();

	typedef itk::CastImageFilter< BSFImageType, InputImageType > CastFilterType;
	typename CastFilterType::Pointer caster = CastFilterType::New();
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
void computeCurvFlowSmooth( ImagePointer & image, PorosityFilterID filterId, RunInfo & results, int nbOfIt, float timeStep, bool releaseData = false )
{
	typedef typename itk::Image< T, DIM >   InputImageType;
	typedef typename itk::Image< float, DIM >   CFSFImageType;

	InputImageType * input = dynamic_cast<InputImageType*>( image.GetPointer() );

	typedef itk::ImageDuplicator< InputImageType > DuplicatorType;
	typename DuplicatorType::Pointer duplicator = DuplicatorType::New();
	duplicator->SetInputImage( input );
	duplicator->Update();

	typedef itk::CurvatureFlowImageFilter<InputImageType, CFSFImageType > CFFType;
	typename CFFType::Pointer cffilter = CFFType::New();
	cffilter->SetInput( duplicator->GetOutput() );
	cffilter->SetNumberOfIterations( nbOfIt );
	cffilter->SetTimeStep( timeStep );
	cffilter->Update();

	typedef itk::CastImageFilter< CFSFImageType, InputImageType > CastFilterType;
	typename CastFilterType::Pointer caster = CastFilterType::New();
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
void computeMedianSmooth( ImagePointer & image, PorosityFilterID filterId, RunInfo & results, int radius, bool releaseData = false )
{
	typedef typename itk::Image< T, DIM >   InputImageType;
	typedef typename itk::Image< float, DIM >   MSFImageType;

	InputImageType * input = dynamic_cast<InputImageType*>( image.GetPointer() );

	typedef itk::ImageDuplicator< InputImageType > DuplicatorType;
	typename DuplicatorType::Pointer duplicator = DuplicatorType::New();
	duplicator->SetInputImage( input );
	duplicator->Update();

	typedef itk::MedianImageFilter<InputImageType, MSFImageType > MSFType;
	typename MSFType::Pointer mfilter = MSFType::New();
	typename InputImageType::SizeType indexRadius;
	indexRadius[0] = radius; // radius along x
	indexRadius[1] = radius; // radius along y
	indexRadius[2] = radius; // radius along z
	mfilter->SetInput( duplicator->GetOutput() );
	mfilter->SetRadius( indexRadius );
	mfilter->Update();

	typedef itk::CastImageFilter< MSFImageType, InputImageType > CastFilterType;
	typename CastFilterType::Pointer caster = CastFilterType::New();
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
void computeIsoXThreshold( ImagePointer & image, PorosityFilterID filterId, RunInfo & results, int isoX, bool releaseData = false )
{
	typedef itk::Image< T, DIM >   InputImageType;
	typedef itk::BinaryThresholdImageFilter <InputImageType, MaskImageType> BinaryThresholdImageFilterType;
	typename BinaryThresholdImageFilterType::Pointer binaryThresholdFilter = BinaryThresholdImageFilterType::New();
	InputImageType * input = dynamic_cast<InputImageType*>( image.GetPointer() );

	//Use duplicator filter because thresholding is in-place
	typedef itk::ImageDuplicator< InputImageType > DuplicatorType;
	typename DuplicatorType::Pointer duplicator = DuplicatorType::New();
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
void computeFhwThreshold( ImagePointer & image, PorosityFilterID filterId, RunInfo & results, int airporeGV, int fhwWeight, bool releaseData = false )
{
	int mdThr, omThr, fhwThr;
	typedef itk::Image< T, DIM >   InputImageType;
	InputImageType * input = dynamic_cast<InputImageType*>( image.GetPointer() );

	// Use duplicator filter (in-place)
	typedef itk::ImageDuplicator< InputImageType > DuplicatorType;
	typename DuplicatorType::Pointer duplicator = DuplicatorType::New();
	duplicator->SetInputImage( input );
	duplicator->Update();

	// Calculate Maximum Distance Threshold
	typedef itk::MaximumDistance< InputImageType >   MaximumDistanceType;
	typename MaximumDistanceType::Pointer maxDistFilter = MaximumDistanceType::New();
	maxDistFilter->SetInput( duplicator->GetOutput() );
	maxDistFilter->SetBins( 10 );
	maxDistFilter->SetCentre( airporeGV );
	maxDistFilter->Update();
	mdThr = maxDistFilter->GetOutThreshold();
	maxDistFilter->ReleaseDataFlagOn();

	// Calculate Otsu Threshold
	typedef itk::OtsuMultipleThresholdsImageFilter <InputImageType, MaskImageType> FilterType;
	typename FilterType::Pointer otsuMultiFilter = FilterType::New();
	otsuMultiFilter->SetInput( duplicator->GetOutput() );
	otsuMultiFilter->SetNumberOfThresholds( 1 );
#if (ITK_MAJOR_VERSION > 4 || ITK_MINOR_VERSION > 5)
	otsuMultiFilter->ValleyEmphasisOn();
#endif
	otsuMultiFilter->Update(); 
	typename FilterType::ThresholdVectorType thresholds = otsuMultiFilter->GetThresholds();
	omThr = thresholds[0];
	otsuMultiFilter->ReleaseDataFlagOn();

	// Calculate Fhw Threshold
	fhwThr = round( omThr * ( fhwWeight / 100.0 ) + mdThr * ( 1.0 - ( fhwWeight / 100.0 ) ) );

	// Segment image with Fhw Threshold
	typedef itk::BinaryThresholdImageFilter <InputImageType, MaskImageType> BinaryThresholdImageFilterType;
	typename BinaryThresholdImageFilterType::Pointer binaryThresholdFilter = BinaryThresholdImageFilterType::New();
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
void runBatch( const QList<PorosityFilterID> & filterIds, ImagePointer & image, RunInfo & results, const QList<IParameterInfo*> & params )
{
	ImagePointer curImage = image;
	results.startTime = QLocale().toString( QDateTime::currentDateTime(), QLocale::ShortFormat );
	int pind = 0;
	foreach( PorosityFilterID fid, filterIds )
	{
		QTime t;
		t.start();
		bool releaseData = (fid != filterIds.last());
		switch( fid )
		{
			case P_BINARY_THRESHOLD:
				computeBinaryThreshold<T>( curImage, results, params[pind]->asFloat(), releaseData );
				break;
			case P_GENERAL_THRESHOLD:
				computeGeneralThreshold<T>(curImage, results, params[pind]->asFloat(), params[pind + 1]->asFloat(), releaseData);
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

void iARunBatchThread::Init(iAPorosityAnalyserModuleInterface * pmi, QString datasetFolder, 
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
		foreach( const QString l, runsCSVHeader )
			runsCSV.setItem( 0, col++, new QTableWidgetItem( l ) );
		foreach( const ParamNameType pnt, paramNames )
			runsCSV.setItem( 0, col++, new QTableWidgetItem( pnt.name() ) );
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

	iAITKIO::writeFile( maskFilename, results.maskImage, itk::ImageIOBase::CHAR, true );

	//Write mask image preview (png) 
	try
	{
		MaskImageType * mask = dynamic_cast<MaskImageType*>( results.maskImage.GetPointer() );
		if ( !mask )
			throw itk::ExceptionObject( "No mask!" );

		QFileInfo maskFI( maskFilename );
		if ( !QDir( maskFI.absoluteDir() ).mkdir( getMaskSliceDirName( maskFilename ) ) )
			throw std::runtime_error( "Could not create directory for slices!" );

		//TODO: move to common or helpers, duplicated in DatasetInfo
		MaskImageType::SizeType size = mask->GetLargestPossibleRegion().GetSize();
		unsigned char * bufferPtr = (unsigned char *) mask->GetBufferPointer();
		unsigned long sliceSize = size[0] * size[1];
		for ( unsigned int sliceNumber = 0; sliceNumber < size[2]; sliceNumber++ )
		{
			unsigned char * sBufferPtr = bufferPtr + sliceSize * sliceNumber;
			QImage img( size[0], size[1], QImage::Format_Indexed8 );
			for ( int y = 0; y < size[1]; y++ )
				memcpy( img.scanLine( size[1] - y - 1 ), sBufferPtr + y*size[0], size[0] );//we invert Y-axis, because VTK and Qt have different Y axis directions
			img.setColor( 0, qRgb( 0, 0, 0 ) );
			img.setColor( 1, qRgb( 255, 255, 255 ) );

			if ( !img.save( getSliceFilename( maskFilename, sliceNumber ) ) )
				throw std::runtime_error( "Could not save png!" );
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
	foreach( PorosityFilterID fid, filterIds )
		paramsNameType.append( FilterIdToParamList[fid] );
	int numParams = paramsNameType.size();

	QString masksDir = "masks";
	QDir( batchDir ).mkdir( masksDir );
	masksDir = batchDir + "/" + masksDir;

	bool randSampling = isRandomSampling( settingsCSV, row );

	QList<IParameterInfo*> params;
	for( int i = 0; i < numParams; i++ )
		params.push_back( getParameterInfo( paramsNameType.at( i ), settingsCSV, row, 3 + i ) );

	double totalNumSamples = 1.0;
	if( randSampling )
	{
		totalNumSamples = getNumRandomSamples( settingsCSV, row );
		randomlySampleParameters( params );
	}
	else
	{
		for( int i = 0; i < numParams; ++i )
			totalNumSamples *= params[i]->numSamples;
	}

	// initialize runsCSV data
	m_runsCSV.clear();
	initRunsCSVFile( m_runsCSV, batchDir, paramsNameType );
	// inintialize input datset
	ScalarPixelType pixelType;
	ImagePointer image = iAITKIO::readFile( datasetName, pixelType, true);

	//GT image (make sure it is the same likne MaskImageType (CHAR))
	ImagePointer gtMask;
	QString dsFN = QFileInfo( datasetName ).fileName();
	QString dsPath = QFileInfo( datasetName ).absolutePath();
	if( m_datasetGTs[dsFN] != "" )
	{
		ScalarPixelType maskPixType;
		QString gtMaskFile = dsPath + "/" + m_datasetGTs[dsFN];
		gtMask = iAITKIO::readFile( gtMaskFile, maskPixType, true);
	}
	emit batchProgress( 0 );

	for( int sampleNo = 0; sampleNo < totalNumSamples; ++sampleNo ) //iterate over parameters
	{
		while( m_pmi->ui()->rbPause->isChecked() )
			QCoreApplication::processEvents();

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
			ITK_TYPED_CALL(runBatch, pixelType, filterIds, image, results, params);
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
				size_t size = static_cast<size_t>(reg.GetSize()[0]) * reg.GetSize()[1] * reg.GetSize()[2];
				size_t tp = 0, fn = 0, fp = 0, tn = 0;
				MaskImageType::PixelType gt, m;

#pragma omp parallel for reduction(+:tp,fn,fp,tn)
				for ( size_t i = 0; i < size; ++i )
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
		foreach( const QString l, computerCSVHeader )
			m_computerCSVData.setItem( 0, col++, new QTableWidgetItem( l ) );
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
	qsrand( QTime::currentTime().msec() );
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

void iARunBatchThread::calcFeatureCharsForMask(RunInfo &results, QString currMaskFilePath)
{
	MaskImageType * mask = dynamic_cast<MaskImageType*>(results.maskImage.GetPointer());
	
	// Label image
	typedef itk::Image<long, DIM>  LabeledImageType;
	typedef itk::ConnectedComponentImageFilter<MaskImageType, LabeledImageType> ConnectedComponentImageFilterType;
	ConnectedComponentImageFilterType::Pointer connectedComponents = ConnectedComponentImageFilterType::New();
	connectedComponents->SetInput(mask);
	connectedComponents->FullyConnectedOn();
	connectedComponents->Update();

	// Save labeled image
	QString labeledMaskName = currMaskFilePath;
	labeledMaskName.insert(currMaskFilePath.lastIndexOf("."), "_labeled");
	iAITKIO::writeFile(labeledMaskName, connectedComponents->GetOutput(), itk::ImageIOBase::LONG, true);

	// Save features characteristics in csv file 
	double spacing = mask->GetSpacing()[0];
	double totalFeatureVol = 0, totalPhi = 0, totalTheta = 0, totalRoundness = 0, totalLength = 0;
	ofstream fout( getLocalEncodingFileName(currMaskFilePath.append(".csv")).c_str(), std::ofstream::out);

	// Header of pore csv file
	fout << "Spacing" << ',' << spacing << '\n'
		<< "Voids\n"
		<< '\n'
		<< '\n'
		<< "Label Id" << ','
		<< "X1" << ','
		<< "Y1" << ','
		<< "Z1" << ','
		<< "X2" << ','
		<< "Y2" << ','
		<< "Z2" << ','
		<< "a11" << ','
		<< "a22" << ','
		<< "a33" << ','
		<< "a12" << ','
		<< "a13" << ','
		<< "a23" << ','
		<< "DimX" << ','
		<< "DimY" << ','
		<< "DimZ" << ','
		<< "phi" << ','
		<< "theta" << ','
		<< "Xm" << ','
		<< "Ym" << ','
		<< "Zm" << ','
		<< "Volume" << ','
		<< "Roundness" << ','
		<< "FeretDiam" << ','
		<< "Flatness" << ','
		<< "VoxDimX" << ','
		<< "VoxDimY" << ','
		<< "VoxDimZ" << ','
		<< "MajorLength" << ','
		<< "MinorLength" << ','
		<< '\n';

	// Initalisation of itk::LabelGeometryImageFilter for calculating pore parameters
	typedef itk::LabelGeometryImageFilter< LabeledImageType > LabelGeometryImageFilterType;
	LabelGeometryImageFilterType::Pointer labelGeometryImageFilter = LabelGeometryImageFilterType::New();
	labelGeometryImageFilter->SetInput(connectedComponents->GetOutput());
	labelGeometryImageFilter->Update();

	// Initalisation of itk::LabelImageToShapeLabelMapFilter for calculating other pore parameters
	typedef unsigned long LabelType;
	typedef itk::ShapeLabelObject<LabelType, DIM>	ShapeLabelObjectType;
	typedef itk::LabelMap<ShapeLabelObjectType>	LabelMapType;
	typedef itk::LabelImageToShapeLabelMapFilter<LabeledImageType, LabelMapType> I2LType;
	I2LType::Pointer i2l = I2LType::New();
	i2l->SetInput(connectedComponents->GetOutput());
	i2l->SetComputePerimeter(false);
	i2l->SetComputeFeretDiameter(false);
	i2l->Update();

	LabelMapType *labelMap = i2l->GetOutput();
	LabelGeometryImageFilterType::LabelsType allLabels = labelGeometryImageFilter->GetLabels();
	LabelGeometryImageFilterType::LabelsType::iterator allLabelsIt;

	// Pore Characteristics calculation 
	for (allLabelsIt = allLabels.begin(); allLabelsIt != allLabels.end(); allLabelsIt++)
	{
		LabelGeometryImageFilterType::LabelPixelType labelValue = *allLabelsIt;
		if (labelValue == 0)	// label 0 = backround
			continue;

		std::vector<double> eigenvalue(3);
		std::vector<double> eigenvector(3);
		std::vector<double> centroid(3);
		int dimX, dimY, dimZ;
		double x1, x2, y1, y2, z1, z2, xm, ym, zm, phi, theta, a11, a22, a33, a12, a13, a23,
			majorlength, minorlength, half_length, dx, dy, dz;

		// Calculating start and and point of the pores's major principal axis
		eigenvalue = labelGeometryImageFilter->GetEigenvalues(labelValue);
		auto maxEigenvalue = std::max_element(std::begin(eigenvalue), std::end(eigenvalue));
		int maxEigenvaluePos = std::distance(std::begin(eigenvalue), maxEigenvalue);

		eigenvector[0] = labelGeometryImageFilter->GetEigenvectors(labelValue)[0][maxEigenvaluePos];
		eigenvector[1] = labelGeometryImageFilter->GetEigenvectors(labelValue)[1][maxEigenvaluePos];
		eigenvector[2] = labelGeometryImageFilter->GetEigenvectors(labelValue)[2][maxEigenvaluePos];
		centroid[0] = labelGeometryImageFilter->GetCentroid(labelValue)[0];
		centroid[1] = labelGeometryImageFilter->GetCentroid(labelValue)[1];
		centroid[2] = labelGeometryImageFilter->GetCentroid(labelValue)[2];
		half_length = labelGeometryImageFilter->GetMajorAxisLength(labelValue) / 2.0;
		x1 = centroid[0] + half_length * eigenvector[0];
		y1 = centroid[1] + half_length * eigenvector[1];
		z1 = centroid[2] + half_length * eigenvector[2];
		x2 = centroid[0] - half_length * eigenvector[0];
		y2 = centroid[1] - half_length * eigenvector[1];
		z2 = centroid[2] - half_length * eigenvector[2];

		// Preparing orientation and tensor calculation
		dx = x1 - x2;
		dy = y1 - y2;
		dz = z1 - z2;
		xm = (x1 + x2) / 2.0f;
		ym = (y1 + y2) / 2.0f;
		zm = (z1 + z2) / 2.0f;

		if (dz < 0)
		{
			dx = x2 - x1;
			dy = y2 - y1;
			dz = z2 - z1;
		}

		phi = asin(dy / sqrt(dx*dx + dy*dy));
		theta = acos(dz / sqrt(dx*dx + dy*dy + dz*dz));
		a11 = cos(phi)*cos(phi)*sin(theta)*sin(theta);
		a22 = sin(phi)*sin(phi)*sin(theta)*sin(theta);
		a33 = cos(theta)*cos(theta);
		a12 = cos(phi)*sin(theta)*sin(theta)*sin(phi);
		a13 = cos(phi)*sin(theta)*cos(theta);
		a23 = sin(phi)*sin(theta)*cos(theta);

		phi = (phi*180.0f) / vtkMath::Pi();
		theta = (theta*180.0f) / vtkMath::Pi();

		// Locating the phi value to quadrant
		if (dx < 0)
			phi = 180.0 - phi;

		if (phi < 0.0)
			phi = phi + 360.0;

		if (dx == 0 && dy == 0)
		{
			phi = 0.0;
			theta = 0.0;
			a11 = 0.0;
			a22 = 0.0;
			a12 = 0.0;
			a13 = 0.0;
			a23 = 0.0;
		}

		majorlength = labelGeometryImageFilter->GetMajorAxisLength(labelValue);
		minorlength = labelGeometryImageFilter->GetMinorAxisLength(labelValue);
		dimX = abs(labelGeometryImageFilter->GetBoundingBox(labelValue)[0] - labelGeometryImageFilter->GetBoundingBox(labelValue)[1]) + 1;
		dimY = abs(labelGeometryImageFilter->GetBoundingBox(labelValue)[2] - labelGeometryImageFilter->GetBoundingBox(labelValue)[3]) + 1;
		dimZ = abs(labelGeometryImageFilter->GetBoundingBox(labelValue)[4] - labelGeometryImageFilter->GetBoundingBox(labelValue)[5]) + 1;

		// Calculation of other pore characteristics and writing the csv file 
		ShapeLabelObjectType *labelObject = labelMap->GetNthLabelObject(labelValue - 1); // debug -1 delated	// labelMap index contaions first pore at 0 

		/* The equivalent radius is a radius of a circle with the same area as the object.
		The feret diameter is the diameter of circumscribing circle. So this measure has a maximum of 1.0 when the object is a perfect circle.
		http://public.kitware.com/pipermail/insight-developers/2011-April/018466.html */
		if (labelObject->GetFeretDiameter() == 0)
		{
			labelObject->SetRoundness(0.0);
		}
		else
		{
			labelObject->SetRoundness(labelObject->GetEquivalentSphericalRadius()
				/ (labelObject->GetFeretDiameter() / 2.0));
		}

		totalFeatureVol += labelGeometryImageFilter->GetVolume(labelValue) * pow(spacing, 3.0);
		totalPhi += phi;
		totalTheta += theta;
		totalRoundness += labelObject->GetRoundness();
		totalLength += majorlength * spacing;

		fout << labelValue << ','
			<< x1 * spacing << ',' 	// unit = microns
			<< y1 * spacing << ',' 	// unit = microns
			<< z1 * spacing << ',' 	// unit = microns
			<< x2 * spacing << ','		// unit = microns
			<< y2 * spacing << ','		// unit = microns
			<< z2 * spacing << ','		// unit = microns
			<< a11 << ','
			<< a22 << ','
			<< a33 << ','
			<< a12 << ','
			<< a13 << ','
			<< a23 << ','
			<< dimX * spacing << ','	// unit = microns
			<< dimY * spacing << ','	// unit = microns
			<< dimZ * spacing << ','	// unit = microns
			<< phi << ','				// unit = °
			<< theta << ','			// unit = °
			<< xm * spacing << ',' 	// unit = microns
			<< ym * spacing << ',' 	// unit = microns
			<< zm * spacing << ',' 	// unit = microns
			<< labelGeometryImageFilter->GetVolume(labelValue) * pow(spacing, 3.0) << ','	// unit = microns^3
			<< labelObject->GetRoundness() << ','
			<< labelObject->GetFeretDiameter() << ','	// unit = microns
			<< labelObject->GetFlatness() << ','
			<< dimX << ','		// unit = voxels
			<< dimY << ','		// unit = voxels
			<< dimZ << ','		// unit = voxels
			<< majorlength * spacing << ',' 	// unit = microns
			<< minorlength * spacing << ',' 	// unit = microns
			<< '\n';
	}
	fout.close();

	results.featureCnt = allLabels.size();
	results.avgFeatureVol = totalFeatureVol / results.featureCnt;
	results.avgFeaturePhi = totalPhi / results.featureCnt;
	results.avgFeatureTheta = totalTheta / results.featureCnt;
	results.avgFeatureRoundness = totalRoundness / results.featureCnt;
	results.avgFeatureLength = totalLength / results.featureCnt;
}