/*=========================================================================
Author: $Author: arnaudgelas $  // Author of last commit
Version: $Rev: 567 $  // Revision of last commit
Date: $Date: 2009-08-17 11:47:32 -0400 (Mon, 17 Aug 2009) $  // Date of last commit
=========================================================================*/

/*=========================================================================
Authors: The GoFigure Dev. Team.
at Megason Lab, Systems biology, Harvard Medical school, 2009

Copyright (c) 2009, President and Fellows of Harvard College.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
Neither the name of the  President and Fellows of Harvard College
nor the names of its contributors may be used to endorse or promote
products derived from this software without specific prior written
permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#ifndef __itkAdaptiveOtsuThresholdImageFilter_txx
#define __itkAdaptiveOtsuThresholdImageFilter_txx

#include "itkAdaptiveOtsuThresholdImageFilter.h"

namespace itk
{
	//  Software Guide : BeginCodeSnippet
	template <class TInputImage, class TOutputImage>
	AdaptiveOtsuThresholdImageFilter<TInputImage, TOutputImage>::
		AdaptiveOtsuThresholdImageFilter()
	{
		m_Radius.Fill( 8 );
		m_NumberOfHistogramBins = 256;
		m_NumberOfControlPoints = 50;
		m_SplineOrder = 3;
		m_NumberOfLevels = 3;
		m_NumberOfSamples = 5000;
		m_OutsideValue = 0;
		m_InsideValue = 1;

		m_PointSet = nullptr;

		this->Superclass::SetNumberOfRequiredInputs( 1 );
		this->Superclass::SetNumberOfRequiredOutputs( 1 );
		this->Superclass::SetNthOutput( 0, OutputImageType::New() );
	}

	template <class TInputImage, class TOutputImage>
	void
		AdaptiveOtsuThresholdImageFilter<TInputImage, TOutputImage>
		::ComputeRandomPointSet()
	{
		InputConstImagePointer input  = this->GetInput();
		InputImageRegionType inputRegion = input->GetLargestPossibleRegion();
		InputSizeType inputSize = inputRegion.GetSize();

		InputIndexType startIndex, endIndex;
		PointSetPointType point;

		// Find a random number generator
		RandomIteratorType rIt( input, inputRegion );
		rIt.SetNumberOfSamples( m_NumberOfSamples );
		rIt.GoToBegin();

		m_PointSet = PointSetType::New();
		PointsContainerPointer
			pointscontainer = m_PointSet->GetPoints();
		pointscontainer->Reserve( m_NumberOfSamples );

		PointDataContainerPointer
			pointdatacontainer = PointDataContainer::New();
		pointdatacontainer->Reserve( m_NumberOfSamples );
		m_PointSet->SetPointData( pointdatacontainer );

		unsigned long i = 0;

		while( !rIt.IsAtEnd() )
		{
			startIndex = rIt.GetIndex();

			for( unsigned int j = 0; j < ImageDimension; j++ )
			{
				endIndex[j] = startIndex[j] + m_Radius[j] - 1;
				if( endIndex[j] >= static_cast< InputIndexValueType >( inputSize[j] ) )
				{
					startIndex[j] = inputSize[j] - m_Radius[j];
				}
			}

			input->TransformIndexToPhysicalPoint( startIndex, point );

			pointscontainer->SetElement( i, point );

			i++;
			++rIt;
		}
	}

	template <class TInputImage, class TOutputImage>
	void
		AdaptiveOtsuThresholdImageFilter<TInputImage, TOutputImage>
		::GenerateData()
	{
		// Allocate output
		this->AllocateOutputs();
		this->GetOutput()->FillBuffer( 0 );

		OutputImagePointer output = this->GetOutput();
		InputConstImagePointer input  = this->GetInput();
		InputImageRegionType inputRegion = input->GetLargestPossibleRegion();
		//InputSizeType inputSize = inputRegion.GetSize();

		InputIndexType startIndex;
		InputImageRegionType region;
		region.SetSize( m_Radius );

		PointSetPointType point;
		VectorPixelType V;

		if( !m_PointSet )
		{
			ComputeRandomPointSet();
		}

		PointsContainerPointer
			pointscontainer = m_PointSet->GetPoints();
		PointDataContainerPointer pointdatacontainer =  m_PointSet->GetPointData();
		for( unsigned long i = 0; i < m_NumberOfSamples; i++ )
		{
			point = pointscontainer->GetElement( i );
			input->TransformPhysicalPointToIndex( point, startIndex );
			region.SetIndex( startIndex );

			ROIFilterPointer roi = ROIFilterType::New();
			roi->SetInput( input );
			roi->SetRegionOfInterest( region );
			roi->Update();

			OtsuThresholdPointer otsu = OtsuThresholdType::New();
			typename HistogramGeneratorType::Pointer histGenerator = HistogramGeneratorType::New();
			histGenerator->SetInput(roi->GetOutput());

			typename HistogramGeneratorType::HistogramSizeType hsize(1);
			hsize[0] = m_NumberOfHistogramBins;
			histGenerator->SetHistogramSize( hsize );
			histGenerator->SetAutoMinimumMaximum( true );
			histGenerator->Update();

			otsu->SetInput( histGenerator->GetOutput() );
			otsu->Update();

			V[0] = static_cast<InputCoordType>( otsu->GetThreshold() );
			pointdatacontainer->SetElement( i, V );
		}

		typename SDAFilterType::ArrayType ncps;
		ncps.Fill( m_NumberOfControlPoints );

		SDAFilterPointer filter = SDAFilterType::New();
		filter->SetSplineOrder( m_SplineOrder );
		filter->SetNumberOfControlPoints( ncps );
		filter->SetNumberOfLevels( m_NumberOfLevels );

		// Define the parametric domain.
		filter->SetOrigin( input->GetOrigin() );
		filter->SetSpacing( input->GetSpacing() );
		filter->SetSize( inputRegion.GetSize() );
		filter->SetInput( m_PointSet );
		filter->Update();

		IndexFilterPointer componentExtractor = IndexFilterType::New();
		componentExtractor->SetInput( filter->GetOutput() );
		componentExtractor->SetIndex( 0 );
		componentExtractor->Update();
		m_Threshold = componentExtractor->GetOutput();

		OutputIteratorType Itt( m_Threshold, inputRegion );
		Itt.GoToBegin();

		OutputIteratorType oIt( output, inputRegion );
		oIt.GoToBegin();

		InputIteratorType iIt( input, inputRegion );
		iIt.GoToBegin();

		OutputPixelType p;
		while( !Itt.IsAtEnd() )
		{
			p = Itt.Get();
			if ( p < iIt.Get() )
			{
				oIt.Set( m_InsideValue  );
			}
			else
			{
				oIt.Set( m_OutsideValue );
			}
			++Itt;
			++oIt;
			++iIt;
		}
	}

	template <class TInputImage, class TOutputImage>
	void
		AdaptiveOtsuThresholdImageFilter<TInputImage, TOutputImage>::
		PrintSelf( std::ostream& os, Indent indent ) const
	{
		Superclass::PrintSelf(os,indent);
		os << indent << "Radius size: " << GetRadius() <<
			std::endl;
		os << indent << "Spline order: " << GetSplineOrder() <<
			std::endl;
		os << indent << "Number Of control points: " << GetNumberOfControlPoints() <<
			std::endl;
		os << indent << "Number of samples: " << GetNumberOfSamples() <<
			std::endl;
		os << indent << "Number Of levels: " << GetNumberOfLevels() <<
			std::endl;
		os << indent << "Number of histogram bins: " << GetNumberOfHistogramBins() <<
			std::endl;
		os << indent << "Inside value: " << GetInsideValue() <<
			std::endl;
		os << indent << "Outside value: " << GetOutsideValue() <<
			std::endl;
	}

} /* end namespace itk */

#endif
