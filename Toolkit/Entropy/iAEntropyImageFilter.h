// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <itkImageToImageFilter.h>

 /** \class iAEntropyImageFilter
  *
  * \brief Calculates the pixelwise entropy out of a collection of input images
  *
  * \par INPUTS
  * All inputs to this filter must be floating point images representing a
  * distribution (i.e. their sum should be 1)
  * Input volumes must all contain the same size RequestedRegions.
  *
  * \par OUTPUTS
  * The filter produces a single output volume. Each output pixel contains the
  * entropy of the given pixel over all input volumes.
  * Higher values indicate higher "uncertainty", i.e. an equal distribution of
  * probabilities; lower values indicate that less evenly distributed
  * probabilities; a value of 0 indicates that the probability of one input is
  * 1, while the others are all 0.
  *
  * \par PARAMETERS
  * Normalize whether the output entropy should be normalized to the interval
  * [0..1]
  *
  * \author Bernhard Froehler, FH Wels
  */


template< typename TInputImage, typename TOutputImage = TInputImage >
class iAEntropyImageFilter :
	public itk::ImageToImageFilter< TInputImage, TOutputImage >
{
public:
	/** Standard class typedefs. */
	typedef iAEntropyImageFilter                                 Self;
	typedef itk::ImageToImageFilter< TInputImage, TOutputImage > Superclass;
	typedef itk::SmartPointer< Self >                            Pointer;
	typedef itk::SmartPointer< const Self >                      ConstPointer;

	/** Method for creation through the object factory. */
	itkNewMacro(Self);

	/** Run-time type information (and related methods) */
	itkTypeMacro(iAEntropyImageFilter, ImageToImageFilter);

	/** Extract some information from the image types.  Dimensionality
	 * of the two images is assumed to be the same. */
	typedef typename TOutputImage::PixelType OutputPixelType;
	typedef typename TInputImage::PixelType  InputPixelType;

	/** Extract some information from the image types.  Dimensionality
	 * of the two images is assumed to be the same. */
	itkStaticConstMacro(InputImageDimension, int, TInputImage::ImageDimension);
	itkStaticConstMacro(ImageDimension, int, TOutputImage::ImageDimension);

	/** Image typedef support */
	typedef TInputImage                           InputImageType;
	typedef TOutputImage                          OutputImageType;
	typedef typename InputImageType::ConstPointer InputImagePointer;
	typedef typename OutputImageType::Pointer     OutputImagePointer;

	typedef unsigned long                         LabelCountType;

	/** Superclass typedefs. */
	typedef typename Superclass::OutputImageRegionType OutputImageRegionType;

	void SetNormalize(bool on)
	{
		m_normalize = on;
	}
	bool GetNormalize() const
	{
		return m_normalize;
	}

#ifdef ITK_USE_CONCEPT_CHECKING
	// Begin concept checking
	itkConceptMacro(SameDimensionCheck,
		(itk::Concept::SameDimension< InputImageDimension, ImageDimension >));
	// End concept checking
#endif

protected:
	iAEntropyImageFilter();
	virtual ~iAEntropyImageFilter() {}

	/** Determine maximum label value in all input images and initialize
	 * global data. */
	void BeforeThreadedGenerateData() override;

	void ThreadedGenerateData
	(const OutputImageRegionType & outputRegionForThread, itk::ThreadIdType threadId) override;

	void PrintSelf(std::ostream &, itk::Indent) const override;
private:
	iAEntropyImageFilter(const Self &) =delete;
	void operator=(const Self &) =delete;

	bool m_normalize;
};

#ifndef ITK_MANUAL_INSTANTIATION
#include "iAEntropyImageFilter.hxx"
#endif
