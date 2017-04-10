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
#pragma

#include "FilteringLabelOverlapMeasuresImageFilter.h"

#include "itkImageRegionConstIterator.h"
#include "itkProgressReporter.h"

namespace fhw {

	template<typename TLabelImage>
	FilteringLabelOverlapMeasuresImageFilter<TLabelImage>
		::FilteringLabelOverlapMeasuresImageFilter() :
		m_ignoredLabel(-1)
	{
		// this filter requires two input images
		this->SetNumberOfRequiredInputs(2);
	}

	template<typename TLabelImage>
	void
		FilteringLabelOverlapMeasuresImageFilter<TLabelImage>
		::EnlargeOutputRequestedRegion(itk::DataObject *data)
	{
		Superclass::EnlargeOutputRequestedRegion(data);
		data->SetRequestedRegionToLargestPossibleRegion();
	}


	template<typename TLabelImage>
	void
		FilteringLabelOverlapMeasuresImageFilter<TLabelImage>
		::AllocateOutputs()
	{
		// Pass the source through as the output
		LabelImagePointer image =
			const_cast<TLabelImage *>(this->GetSourceImage());
		this->GraftOutput(image);
	}

	template<typename TLabelImage>
	void
		FilteringLabelOverlapMeasuresImageFilter<TLabelImage>
		::BeforeThreadedGenerateData()
	{
		itk::ThreadIdType numberOfThreads = this->GetNumberOfThreads();

		// Resize the thread temporaries
		this->m_LabelSetMeasuresPerThread.resize(numberOfThreads);

		// Initialize the temporaries
		for (itk::ThreadIdType n = 0; n < numberOfThreads; n++)
		{
			this->m_LabelSetMeasuresPerThread[n].clear();
		}

		// Initialize the final map
		this->m_LabelSetMeasures.clear();
	}

	template<typename TLabelImage>
	void
		FilteringLabelOverlapMeasuresImageFilter<TLabelImage>
		::AfterThreadedGenerateData()
	{
		// Run through the map for each thread and accumulate the set measures.
		for (itk::ThreadIdType n = 0; n < this->GetNumberOfThreads(); n++)
		{
			// iterate over the map for this thread
			for (MapConstIterator threadIt = this->m_LabelSetMeasuresPerThread[n].begin();
				threadIt != this->m_LabelSetMeasuresPerThread[n].end();
				++threadIt)
			{
				// does this label exist in the cumulative structure yet?
				MapIterator mapIt = this->m_LabelSetMeasures.find((*threadIt).first);
				if (mapIt == this->m_LabelSetMeasures.end())
				{
					// create a new entry
					typedef typename MapType::value_type MapValueType;
					mapIt = this->m_LabelSetMeasures.insert(MapValueType(
						(*threadIt).first, LabelSetMeasures())).first;
				}

				// accumulate the information from this thread
				(*mapIt).second.m_Source += (*threadIt).second.m_Source;
				(*mapIt).second.m_Target += (*threadIt).second.m_Target;
				(*mapIt).second.m_Union += (*threadIt).second.m_Union;
				(*mapIt).second.m_Intersection +=
					(*threadIt).second.m_Intersection;
				(*mapIt).second.m_SourceComplement +=
					(*threadIt).second.m_SourceComplement;
				(*mapIt).second.m_TargetComplement +=
					(*threadIt).second.m_TargetComplement;
			} // end of thread map iterator loop
		} // end of thread loop
	}

	template<typename TLabelImage>
	void
		FilteringLabelOverlapMeasuresImageFilter<TLabelImage>
		::ThreadedGenerateData(const RegionType& outputRegionForThread,
			itk::ThreadIdType threadId)
	{
		itk::ImageRegionConstIterator<LabelImageType> itS(this->GetSourceImage(),
			outputRegionForThread);
		itk::ImageRegionConstIterator<LabelImageType> itT(this->GetTargetImage(),
			outputRegionForThread);

		// support progress methods/callbacks
		itk::ProgressReporter progress(this, threadId,
			2 * outputRegionForThread.GetNumberOfPixels());

		for (itS.GoToBegin(), itT.GoToBegin(); !itS.IsAtEnd(); ++itS, ++itT)
		{
			LabelType sourceLabel = itS.Get();
			LabelType targetLabel = itT.Get();

			if (targetLabel != m_ignoredLabel)
			{
				// is the label already in this thread?
				MapIterator mapItS =
					this->m_LabelSetMeasuresPerThread[threadId].find(sourceLabel);
				MapIterator mapItT =
					this->m_LabelSetMeasuresPerThread[threadId].find(targetLabel);

				if (mapItS == this->m_LabelSetMeasuresPerThread[threadId].end())
				{
					// create a new label set measures object
					typedef typename MapType::value_type MapValueType;
					mapItS = this->m_LabelSetMeasuresPerThread[threadId].insert(
						MapValueType(sourceLabel, LabelSetMeasures())).first;
				}

				if (mapItT == this->m_LabelSetMeasuresPerThread[threadId].end())
				{
					// create a new label set measures object
					typedef typename MapType::value_type MapValueType;
					mapItT = this->m_LabelSetMeasuresPerThread[threadId].insert(
						MapValueType(targetLabel, LabelSetMeasures())).first;
				}

				(*mapItS).second.m_Source++;
				(*mapItT).second.m_Target++;

				if (sourceLabel == targetLabel)
				{
					(*mapItS).second.m_Intersection++;
					(*mapItS).second.m_Union++;
				}
				else
				{
					(*mapItS).second.m_Union++;
					(*mapItT).second.m_Union++;

					(*mapItS).second.m_SourceComplement++;
					(*mapItT).second.m_TargetComplement++;
				}
			}
			progress.CompletedPixel();
		}
	}

	/**
	 *  measures
	 */
	template<typename TLabelImage>
	typename FilteringLabelOverlapMeasuresImageFilter<TLabelImage>::RealType
		FilteringLabelOverlapMeasuresImageFilter<TLabelImage>
		::GetTotalOverlap() const
	{
		RealType numerator = 0.0;
		RealType denominator = 0.0;
		for (MapConstIterator mapIt = this->m_LabelSetMeasures.begin();
			mapIt != this->m_LabelSetMeasures.end(); ++mapIt)
		{
			// Do not include the background in the final value.
			if ((*mapIt).first == itk::NumericTraits<LabelType>::ZeroValue())
			{
				continue;
			}
			numerator += static_cast<RealType>((*mapIt).second.m_Intersection);
			denominator += static_cast<RealType>((*mapIt).second.m_Target);
		}

		if (itk::Math::ExactlyEquals(denominator, 0.0))
		{
			return itk::NumericTraits<RealType>::max();
		}
		else
		{
			return (numerator / denominator);
		}
	}

	template<typename TLabelImage>
	typename FilteringLabelOverlapMeasuresImageFilter<TLabelImage>::RealType
		FilteringLabelOverlapMeasuresImageFilter<TLabelImage>
		::GetTargetOverlap(LabelType label) const
	{
		MapConstIterator mapIt = this->m_LabelSetMeasures.find(label);
		if (mapIt == this->m_LabelSetMeasures.end())
		{
			itkWarningMacro("Label " << label << " not found.");
			return 0.0;
		}

		//std::cout << "GetTargetOverlap ::: 001" << std::endl;
		RealType value;

		if ((*mapIt).second.m_Target == 0)
		{
			value = itk::NumericTraits<RealType>::max();
		}
		else
		{
			value =
				static_cast<RealType>((*mapIt).second.m_Intersection) /
				static_cast<RealType>((*mapIt).second.m_Target);
		}
		return value;
	}

	template<typename TLabelImage>
	typename FilteringLabelOverlapMeasuresImageFilter<TLabelImage>::RealType
		FilteringLabelOverlapMeasuresImageFilter<TLabelImage>
		::GetUnionOverlap() const
	{
		RealType numerator = 0.0;
		RealType denominator = 0.0;
		for (MapConstIterator mapIt = this->m_LabelSetMeasures.begin();
			mapIt != this->m_LabelSetMeasures.end(); ++mapIt)
		{
			// Do not include the background in the final value.
			if ((*mapIt).first == itk::NumericTraits<LabelType>::ZeroValue())
			{
				continue;
			}
			numerator += static_cast<RealType>((*mapIt).second.m_Intersection);
			denominator += static_cast<RealType>((*mapIt).second.m_Union);
		}

		if (itk::Math::ExactlyEquals(denominator, 0.0))
		{
			return itk::NumericTraits<RealType>::max();
		}
		else
		{
			return (numerator / denominator);
		}
	}

	template<typename TLabelImage>
	typename FilteringLabelOverlapMeasuresImageFilter<TLabelImage>::RealType
		FilteringLabelOverlapMeasuresImageFilter<TLabelImage>
		::GetUnionOverlap(LabelType label) const
	{
		MapConstIterator mapIt = this->m_LabelSetMeasures.find(label);
		if (mapIt == this->m_LabelSetMeasures.end())
		{
			itkWarningMacro("Label " << label << " not found.");
			return 0.0;
		}

		RealType value;
		if (itk::Math::ExactlyEquals((*mapIt).second.m_Union, 0.0))
		{
			value = itk::NumericTraits<RealType>::max();
		}
		else
		{
			value =
				static_cast<RealType>((*mapIt).second.m_Intersection) /
				static_cast<RealType>((*mapIt).second.m_Union);
		}

		return value;
	}

	template<typename TLabelImage>
	typename FilteringLabelOverlapMeasuresImageFilter<TLabelImage>::RealType
		FilteringLabelOverlapMeasuresImageFilter<TLabelImage>
		::GetMeanOverlap() const
	{
		RealType uo = this->GetUnionOverlap();
		return (2.0 * uo / (1.0 + uo));
	}

	template<typename TLabelImage>
	typename FilteringLabelOverlapMeasuresImageFilter<TLabelImage>::RealType
		FilteringLabelOverlapMeasuresImageFilter<TLabelImage>
		::GetMeanOverlap(LabelType label) const
	{
		RealType uo = this->GetUnionOverlap(label);
		return (2.0 * uo / (1.0 + uo));
	}

	template<typename TLabelImage>
	typename FilteringLabelOverlapMeasuresImageFilter<TLabelImage>::RealType
		FilteringLabelOverlapMeasuresImageFilter<TLabelImage>
		::GetVolumeSimilarity() const
	{
		RealType numerator = 0.0;
		RealType denominator = 0.0;
		for (MapConstIterator mapIt = this->m_LabelSetMeasures.begin();
			mapIt != this->m_LabelSetMeasures.end(); ++mapIt)
		{
			// Do not include the background in the final value.
			if ((*mapIt).first == itk::NumericTraits<LabelType>::ZeroValue())
			{
				continue;
			}
			numerator += ((static_cast<RealType>((*mapIt).second.m_Source) -
				static_cast<RealType>((*mapIt).second.m_Target)));
			denominator += ((static_cast<RealType>((*mapIt).second.m_Source) +
				static_cast<RealType>((*mapIt).second.m_Target)));
		}

		if (itk::Math::ExactlyEquals(denominator, 0.0))
		{
			return itk::NumericTraits<RealType>::max();
		}
		else
		{
			return (2.0 * numerator / denominator);
		}
	}

	template<typename TLabelImage>
	typename FilteringLabelOverlapMeasuresImageFilter<TLabelImage>::RealType
		FilteringLabelOverlapMeasuresImageFilter<TLabelImage>
		::GetVolumeSimilarity(LabelType label) const
	{
		MapConstIterator mapIt = this->m_LabelSetMeasures.find(label);
		if (mapIt == this->m_LabelSetMeasures.end())
		{
			itkWarningMacro("Label " << label << " not found.");
			return 0.0;
		}
		RealType value = 2.0 *
			(static_cast<RealType>((*mapIt).second.m_Source) -
				static_cast<RealType>((*mapIt).second.m_Target)) /
				(static_cast<RealType>((*mapIt).second.m_Source) +
					static_cast<RealType>((*mapIt).second.m_Target));
		return value;
	}

	template<typename TLabelImage>
	typename FilteringLabelOverlapMeasuresImageFilter<TLabelImage>::RealType
		FilteringLabelOverlapMeasuresImageFilter<TLabelImage>
		::GetFalseNegativeError() const
	{
		RealType numerator = 0.0;
		RealType denominator = 0.0;
		for (MapConstIterator mapIt = this->m_LabelSetMeasures.begin();
			mapIt != this->m_LabelSetMeasures.end(); ++mapIt)
		{
			// Do not include the background in the final value.
			if ((*mapIt).first == itk::NumericTraits<LabelType>::ZeroValue())
			{
				continue;
			}
			numerator += static_cast<RealType>((*mapIt).second.m_TargetComplement);
			denominator += static_cast<RealType>((*mapIt).second.m_Target);
		}

		if (itk::Math::ExactlyEquals(denominator, 0.0))
		{
			return itk::NumericTraits<RealType>::max();
		}
		else
		{
			return (numerator / denominator);
		}
	}

	template<typename TLabelImage>
	typename FilteringLabelOverlapMeasuresImageFilter<TLabelImage>::RealType
		FilteringLabelOverlapMeasuresImageFilter<TLabelImage>
		::GetFalseNegativeError(LabelType label) const
	{
		MapConstIterator mapIt = this->m_LabelSetMeasures.find(label);
		if (mapIt == this->m_LabelSetMeasures.end())
		{
			itkWarningMacro("Label " << label << " not found.");
			return 0.0;
		}

		RealType value;
		if (itk::Math::ExactlyEquals((*mapIt).second.m_Target, 0.0))
		{
			value = itk::NumericTraits<RealType>::max();
		}
		else
		{
			value =
				static_cast<RealType>((*mapIt).second.m_TargetComplement) /
				static_cast<RealType>((*mapIt).second.m_Target);
		}

		return value;
	}

	template<typename TLabelImage>
	typename FilteringLabelOverlapMeasuresImageFilter<TLabelImage>::RealType
		FilteringLabelOverlapMeasuresImageFilter<TLabelImage>
		::GetFalsePositiveError() const
	{
		RealType numerator = 0.0;
		RealType denominator = 0.0;
		for (MapConstIterator mapIt = this->m_LabelSetMeasures.begin();
			mapIt != this->m_LabelSetMeasures.end(); ++mapIt)
		{
			// Do not include the background in the final value.
			if ((*mapIt).first == itk::NumericTraits<LabelType>::ZeroValue())
			{
				continue;
			}
			numerator += static_cast<RealType>((*mapIt).second.m_SourceComplement);
			denominator += static_cast<RealType>((*mapIt).second.m_Source);
		}

		if (itk::Math::ExactlyEquals(denominator, 0.0))
		{
			return itk::NumericTraits<RealType>::max();
		}
		else
		{
			return (numerator / denominator);
		}
	}

	template<typename TLabelImage>
	typename FilteringLabelOverlapMeasuresImageFilter<TLabelImage>::RealType
		FilteringLabelOverlapMeasuresImageFilter<TLabelImage>
		::GetFalsePositiveError(LabelType label) const
	{
		MapConstIterator mapIt = this->m_LabelSetMeasures.find(label);
		if (mapIt == this->m_LabelSetMeasures.end())
		{
			itkWarningMacro("Label " << label << " not found.");
			return 0.0;
		}

		RealType value;
		if (itk::Math::ExactlyEquals((*mapIt).second.m_Source, 0.0))
		{
			value = itk::NumericTraits<RealType>::max();
		}
		else
		{
			value =
				static_cast<RealType>((*mapIt).second.m_SourceComplement) /
				static_cast<RealType>((*mapIt).second.m_Source);
		}

		return value;
	}

	template<typename TLabelImage>
	void
		FilteringLabelOverlapMeasuresImageFilter<TLabelImage>
		::PrintSelf(std::ostream& os, itk::Indent indent) const
	{
		// todo!!!
		Superclass::PrintSelf(os, indent);
	}


}// end namespace fhw
