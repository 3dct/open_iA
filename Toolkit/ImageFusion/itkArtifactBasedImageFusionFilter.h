#ifndef __itkArtifactBasedImageFusionFilter_h
#define __itkArtifactBasedImageFusionFilter_h

#include "itkInPlaceImageFilter.h"
#include <itkImageRegionConstIterator.h>
#include <itkImageRegionIterator.h>


namespace itk
{
  
/** \class ArtifactBasedImageFusionFilter
 * \brief Implements pixel-wise fusion of two images.
 *
 * \ingroup IntensityImageFilters   Multithreaded
 */
template <class TInputImage1, class TInputImage2, class TInputImage3,
		  class TOutputImage >
class ArtifactBasedImageFusionFilter :
	public InPlaceImageFilter<TInputImage1, TOutputImage>
{
public:
  /** Standard class typedefs. */
  typedef ArtifactBasedImageFusionFilter				  Self;
  typedef InPlaceImageFilter<TInputImage1, TOutputImage>  Superclass;
  typedef SmartPointer<Self>                              Pointer;
  typedef SmartPointer<const Self>                        ConstPointer;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);
  
  /** Run-time type information (and related methods). */
  itkTypeMacro(ArtifactBasedImageFusionFilter, InPlaceImageFilter);

  /** Some convenient typedefs. */
  typedef TInputImage1                           Input1ImageType;
  typedef typename Input1ImageType::ConstPointer Input1ImagePointer;
  typedef typename Input1ImageType::RegionType   Input1ImageRegionType; 
  typedef typename Input1ImageType::PixelType    Input1ImagePixelType; 

  typedef typename NumericTraits<Input1ImagePixelType>::RealType Input1RealType;

  typedef TInputImage2                           Input2ImageType;
  typedef typename Input2ImageType::ConstPointer Input2ImagePointer;
  typedef typename Input2ImageType::RegionType   Input2ImageRegionType; 
  typedef typename Input2ImageType::PixelType    Input2ImagePixelType; 

  typedef TInputImage3                           Input3ImageType;
  typedef typename Input3ImageType::ConstPointer Input3ImagePointer;
  typedef typename Input3ImageType::RegionType   Input3ImageRegionType; 
  typedef typename Input3ImageType::PixelType    Input3ImagePixelType; 

  typedef TOutputImage                           OutputImageType;
  typedef typename OutputImageType::Pointer      OutputImagePointer;
  typedef typename OutputImageType::RegionType   OutputImageRegionType;
  typedef typename OutputImageType::PixelType    OutputImagePixelType;

  /** Connect one of the operands for pixel-wise fusion */
  void SetInput1( const TInputImage1 * image1);

  /** Connect one of the operands for pixel-wise fusion */
  void SetInput2( const TInputImage2 * image2);

  /** Connect one of the operands for pixel-wise fusion */
  void SetInput3( const TInputImage3 * image3);

  ///** Set the "m_Threshold" value. Default is 0. */
  //itkSetMacro(m_Threshold, Input1RealType);
  ///** Get the "m_Threshold" value. */
  //itkGetMacro(m_Threshold, Input1RealType);

  /** ImageDimension constants */
  itkStaticConstMacro(
	InputImage1Dimension, unsigned int, TInputImage1::ImageDimension);
  itkStaticConstMacro(
	InputImage2Dimension, unsigned int, TInputImage2::ImageDimension);
  itkStaticConstMacro(
	  InputImage3Dimension, unsigned int, TInputImage3::ImageDimension);
  itkStaticConstMacro(
	OutputImageDimension, unsigned int, TOutputImage::ImageDimension);




#ifdef ITK_USE_CONCEPT_CHECKING
  /** Begin concept checking */
  itkConceptMacro(SameDimensionCheck1,
	(Concept::SameDimension<itkGetStaticConstMacro(InputImage1Dimension),
							itkGetStaticConstMacro(OutputImageDimension)>));
  itkConceptMacro(SameDimensionCheck2,
	(Concept::SameDimension<itkGetStaticConstMacro(InputImage1Dimension),
							itkGetStaticConstMacro(InputImage2Dimension)>));
  itkConceptMacro(SameDimensionCheck3,
	  (Concept::SameDimension<itkGetStaticConstMacro(InputImage1Dimension),
	  itkGetStaticConstMacro(InputImage3Dimension)>));

  /** End concept checking */
#endif

protected:
  ArtifactBasedImageFusionFilter();
  virtual ~ArtifactBasedImageFusionFilter() {}

  /** ThresholdFusionFilter can be implemented as a multithreaded filter.
   * Therefore, this implementation provides a ThreadedGenerateData() routine
   * which is called for each processing thread. The output image data is
   * allocated automatically by the superclass prior to calling
   * ThreadedGenerateData().  ThreadedGenerateData can only write to the
   * portion of the output image specified by the parameter
   * "outputRegionForThread"
   *
   * \sa ImageToImageFilter::ThreadedGenerateData(),
   *     ImageToImageFilter::GenerateData()  */
  void ThreadedGenerateData(const OutputImageRegionType& outputRegionForThread,
							itk::ThreadIdType threadId ) override;

private:
  ArtifactBasedImageFusionFilter(const Self&); //purposely not implemented
  void operator=(const Self&); //purposely not implemented

  Input1RealType m_ThresholdRange[2];
  Input1RealType m_ThresholdRangeLength;
public:
	void SetThresholdRangeValue(Input1RealType ThresholdRange[2]) 
	{ 
		m_ThresholdRange[0] = ThresholdRange[0]; 
		m_ThresholdRange[1] = ThresholdRange[1];
		m_ThresholdRangeLength = m_ThresholdRange[1] - m_ThresholdRange[0];
	}
	Input1RealType * GetThresholdValue() { return m_ThresholdRange;}
};

} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkArtifactBasedImageFusionFilter.txx"
#endif

#endif
