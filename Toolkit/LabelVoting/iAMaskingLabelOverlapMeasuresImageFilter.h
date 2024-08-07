/*=========================================================================
*
*  Copyright Insight Software Consortium
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*         http://www.apache.org/licenses/LICENSE-2.0.txt
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
*
*=========================================================================*/
#pragma once

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#endif
#include <itkImageToImageFilter.h>
#include <itkNumericTraits.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include <unordered_map>

/** \class iAMaskingLabelOverlapMeasuresImageFilter
* \brief Computes overlap measures between the set same set of labels of
* pixels of two images.  Background is assumed to be 0.
* Based on LabelOverlapMeasuresImageFilter. In addition, it filters out
* certain labels for the result
*
*
* \author Nicholas J. Tustison
* \sa iAMaskingLabelOverlapMeasuresImageFilter
*
* \ingroup ITKImageStatistics
* \ingroup MultiThreaded
*/
template<typename TLabelImage>
class iAMaskingLabelOverlapMeasuresImageFilter :
	public itk::ImageToImageFilter<TLabelImage, TLabelImage>
{
public:
	/** Standard Self typedef */
	typedef iAMaskingLabelOverlapMeasuresImageFilter              Self;
	typedef itk::ImageToImageFilter<TLabelImage, TLabelImage>   Superclass;
	typedef itk::SmartPointer<Self>                             Pointer;
	typedef itk::SmartPointer<const Self>                       ConstPointer;

	/** Method for creation through the object factory. */
	itkNewMacro(Self);

	/** Runtime information support. */
	itkTypeMacro(iAMaskingLabelOverlapMeasuresImageFilter, ImageToImageFilter);

	/** Image related typedefs. */
	typedef TLabelImage                                   LabelImageType;
	typedef typename TLabelImage::Pointer                 LabelImagePointer;
	typedef typename TLabelImage::ConstPointer            LabelImageConstPointer;

	typedef typename TLabelImage::RegionType              RegionType;
	typedef typename TLabelImage::SizeType                SizeType;
	typedef typename TLabelImage::IndexType               IndexType;

	typedef typename TLabelImage::PixelType               LabelType;

	/** Type to use for computations. */
	typedef typename itk::NumericTraits<LabelType>::RealType RealType;

	/** \class LabelSetMeasures
	* \brief Metrics stored per label
	* \ingroup ITKImageStatistics
	*/
	class LabelSetMeasures
	{
	public:
		// default constructor
		LabelSetMeasures():
			m_Source(0),
			m_Target(0),
			m_Union(0),
			m_Intersection(0),
			m_SourceComplement(0),
			m_TargetComplement(0)
		{
		}
		unsigned long m_Source;
		unsigned long m_Target;
		unsigned long m_Union;
		unsigned long m_Intersection;
		unsigned long m_SourceComplement;
		unsigned long m_TargetComplement;
	};

	/** Type of the map used to store data per label */
	typedef std::unordered_map<LabelType, LabelSetMeasures> MapType;
	typedef typename MapType::iterator                    MapIterator;
	typedef typename MapType::const_iterator              MapConstIterator;

	/** Image related typedefs. */
	itkStaticConstMacro(ImageDimension, unsigned int,
		TLabelImage::ImageDimension);

	void SetIgnoredLabel(LabelType label)
	{
		m_ignoredLabel = label;
	}

	/** Set the source image. */
	void SetSourceImage(const LabelImageType * image)
	{
		this->SetNthInput(0, const_cast<LabelImageType *>(image));
	}

	/** Set the target image. */
	void SetTargetImage(const LabelImageType * image)
	{
		this->SetNthInput(1, const_cast<LabelImageType *>(image));
	}

	/** Get the source image. */
	const LabelImageType * GetSourceImage(void)
	{
		return this->GetInput(0);
	}

	/** Get the target image. */
	const LabelImageType * GetTargetImage(void)
	{
		return this->GetInput(1);
	}

	/** Get the label set measures */
	MapType GetLabelSetMeasures()
	{
		return this->m_LabelSetMeasures;
	}

	/**
	* tric overlap measures
	*/
	/** measures over all labels */
	RealType GetTotalOverlap() const;
	RealType GetUnionOverlap() const;
	RealType GetMeanOverlap() const;
	RealType GetVolumeSimilarity() const;
	RealType GetFalseNegativeError() const;
	RealType GetFalsePositiveError() const;
	/** measures over individual labels */
	RealType GetTargetOverlap(LabelType) const;
	RealType GetUnionOverlap(LabelType) const;
	RealType GetMeanOverlap(LabelType) const;
	RealType GetVolumeSimilarity(LabelType) const;
	RealType GetFalseNegativeError(LabelType) const;
	RealType GetFalsePositiveError(LabelType) const;
	/** alternative names */
	RealType GetJaccardCoefficient() const
	{
		return this->GetUnionOverlap();
	}
	RealType GetJaccardCoefficient(LabelType label) const
	{
		return this->GetUnionOverlap(label);
	}
	RealType GetDiceCoefficient() const
	{
		return this->GetMeanOverlap();
	}
	RealType GetDiceCoefficient(LabelType label) const
	{
		return this->GetMeanOverlap(label);
	}

	std::set<size_t> const & IgnoredIndices()
	{
		return m_ignoredIndices;
	}

	void SetIgnoredIndices(std::set<size_t> const & v)
	{
		m_ignoredIndices = v;
	}

protected:
	iAMaskingLabelOverlapMeasuresImageFilter();
	~iAMaskingLabelOverlapMeasuresImageFilter() {};
	void PrintSelf(std::ostream& os, itk::Indent indent) const override;

	/**
	* Pass the input through unmodified. Do this by setting the output to the
	* source this by setting the output to the source image in the
	* AllocateOutputs() method.
	*/
	void AllocateOutputs() override;

	void BeforeThreadedGenerateData() override;

	void AfterThreadedGenerateData() override;

	/** Multi-thread version GenerateData. */
	void ThreadedGenerateData(const RegionType&, itk::ThreadIdType) override;

	// Override since the filter produces all of its output
	void EnlargeOutputRequestedRegion(itk::DataObject *data) override;

private:
	iAMaskingLabelOverlapMeasuresImageFilter(const Self&) =delete;
	void operator=(const Self&) =delete;

	std::vector<MapType> m_LabelSetMeasuresPerThread;
	MapType              m_LabelSetMeasures;
	LabelType            m_ignoredLabel;
	std::set<size_t>     m_ignoredIndices;

};

#ifndef ITK_MANUAL_INSTANTIATION
#include "iAMaskingLabelOverlapMeasuresImageFilter.hxx"
#endif
