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

#ifndef __itkAdaptiveOtsuThresholdImageFilter_h
#define __itkAdaptiveOtsuThresholdImageFilter_h

#if defined(_MSC_VER)
#pragma warning ( disable : 4786 )
#endif

#ifdef __BORLANDC__
#define ITK_LEAN_AND_MEAN
#endif

#include "itkImageToImageFilter.h"
#include "itkOtsuThresholdCalculator.h"
#include "itkImageRegionIterator.h"
#include "itkImageToHistogramFilter.h"
#include "itkNumericTraits.h"
#include "itkVector.h"
#include "itkPointSet.h"
#include "itkConstNeighborhoodIterator.h"
#include "itkImageRegionIterator.h"
#include "itkImageRegionConstIterator.h"
#include "itkImageRandomNonRepeatingConstIteratorWithIndex.h"
#include "itkBSplineScatteredDataPointSetToImageFilter.h"
#include "itkVectorIndexSelectionCastImageFilter.h"
#include "itkRegionOfInterestImageFilter.h"

namespace itk {

template < class TInputImage, class TOutputImage >
class AdaptiveOtsuThresholdImageFilter :
  public ImageToImageFilter< TInputImage, TOutputImage >
{
public:

  typedef AdaptiveOtsuThresholdImageFilter                Self;
  typedef ImageToImageFilter< TInputImage, TOutputImage > Superclass;
  typedef SmartPointer<Self>                              Pointer;
  typedef SmartPointer<const Self>                        ConstPointer;

  itkStaticConstMacro(ImageDimension, unsigned int,
    TInputImage::ImageDimension);

  /** Method for creation through object factory */
  itkNewMacro(Self);

  /** Run-time type information */
  itkTypeMacro( AdaptiveOtsuThresholdImageFilter, ImageToImageFilter );

  /** Display */
  void PrintSelf( std::ostream& os, Indent indent ) const override;

  /** Image typedef support. */
  /** Convenient typedefs for simplifying declarations. */
  typedef TInputImage                             InputImageType;
  typedef typename InputImageType::Pointer        InputImagePointer;
  typedef typename InputImageType::ConstPointer   InputConstImagePointer;
  typedef typename InputImageType::PixelType      InputPixelType;
  typedef typename InputImageType::PointType      InputPointType;
  typedef typename InputPointType::CoordRepType   InputCoordType;
  typedef typename InputImageType::RegionType     InputImageRegionType;
  typedef typename InputImageType::IndexType      InputIndexType;
  typedef typename InputIndexType::IndexValueType  InputIndexValueType;
  typedef typename InputImageType::SizeType       InputSizeType;

  typedef TOutputImage                          OutputImageType;
  typedef typename OutputImageType::Pointer     OutputImagePointer;
  typedef typename OutputImageType::PixelType   OutputPixelType;
  typedef typename OutputImageType::RegionType  OutputImageRegionType;

  typedef ImageRegionConstIterator< InputImageType >      InputIteratorType;
  typedef ImageRegionIteratorWithIndex< OutputImageType > OutputIteratorType;

  typedef ImageRandomNonRepeatingConstIteratorWithIndex< InputImageType >
    RandomIteratorType;

  typedef RegionOfInterestImageFilter< InputImageType, InputImageType >
    ROIFilterType;
  typedef typename ROIFilterType::Pointer
    ROIFilterPointer;
  typedef itk::Statistics::ImageToHistogramFilter<InputImageType>  HistogramGeneratorType;
  typedef typename HistogramGeneratorType::HistogramType               HistogramType;
  typedef itk::OtsuThresholdCalculator<HistogramType>         OtsuThresholdType;

  typedef typename OtsuThresholdType::Pointer             OtsuThresholdPointer;

  typedef Vector< InputCoordType, 1 >         VectorType;
  typedef Image< VectorType, ImageDimension > VectorImageType;
  typedef typename VectorImageType::PixelType VectorPixelType;

  typedef PointSet< VectorPixelType, ImageDimension >      PointSetType;
  typedef typename PointSetType::Pointer                   PointSetPointer;
  typedef typename PointSetType::PointType                 PointSetPointType;
  typedef typename PointSetType::PointsContainerPointer    PointsContainerPointer;
  typedef typename PointSetType::PointDataContainerPointer PointDataContainerPointer;
  typedef typename PointSetType::PointDataContainer        PointDataContainer;

  typedef BSplineScatteredDataPointSetToImageFilter< PointSetType,
    VectorImageType > SDAFilterType;
  typedef typename SDAFilterType::Pointer SDAFilterPointer;

  typedef Image< InputCoordType, ImageDimension > CoordImageType;
  typedef typename CoordImageType::Pointer        CoordImagePointer;
  typedef VectorIndexSelectionCastImageFilter< VectorImageType,
    OutputImageType > IndexFilterType;
  typedef typename IndexFilterType::Pointer IndexFilterPointer;
//   typedef ImageRegionIteratorWithIndex< CoordImageType > CoordIteratorType;

  /** Set the radius of the neighborhood used to compute the median. */
  itkSetMacro(Radius, InputSizeType);

  /** Get the radius of the neighborhood used to compute the median */
  itkGetConstReferenceMacro(Radius, InputSizeType);

  itkSetMacro(NumberOfHistogramBins, unsigned int);
  itkGetConstMacro(NumberOfHistogramBins, unsigned int);

  itkSetMacro(NumberOfControlPoints, unsigned int);
  itkGetConstMacro(NumberOfControlPoints, unsigned int);

  itkSetMacro(NumberOfLevels, unsigned int);
  itkGetConstMacro(NumberOfLevels, unsigned int);

  itkSetMacro(NumberOfSamples, unsigned int);
  itkGetConstMacro(NumberOfSamples, unsigned int);

  itkSetMacro(SplineOrder, unsigned int);
  itkGetConstMacro(SplineOrder, unsigned int);

  itkSetMacro(OutsideValue, OutputPixelType);
  itkGetConstReferenceMacro(OutsideValue, OutputPixelType);

  itkSetMacro(InsideValue, OutputPixelType);
  itkGetConstReferenceMacro(InsideValue, OutputPixelType);

  OutputImagePointer GetThresholdImage()
    {
    return m_Threshold;
    }

  void SetPointSet( PointSetPointer pt )
    {
    m_PointSet = pt;
    }

protected:

  AdaptiveOtsuThresholdImageFilter();
  ~AdaptiveOtsuThresholdImageFilter() {}

  void ComputeRandomPointSet();
  void GenerateData() override;

  InputSizeType m_Radius;
  unsigned int m_NumberOfHistogramBins;
  unsigned int m_NumberOfControlPoints;
  unsigned int m_NumberOfLevels;
  unsigned int m_NumberOfSamples;
  unsigned int m_SplineOrder;
  OutputPixelType m_OutsideValue;
  OutputPixelType m_InsideValue;

  PointSetPointer m_PointSet;
  OutputImagePointer m_Threshold;

private:

  AdaptiveOtsuThresholdImageFilter( const Self&);   // intentionally not implemented
  void operator=(const Self&);          // intentionally not implemented
};

} /* namespace itk */

#include "itkAdaptiveOtsuThresholdImageFilter.txx"
#endif
