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
/*
 This file was originally released as part of the ITK project,
 under the Apache license (see below).
 It has been adapted; for information on the specific changes,
 please compare this file to the file
 Modules\Segmentation\LabelVoting\include\itkLabelVotingImageFilter.hxx
 as included in the ITK release 4.10
*/
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

#include <itkImageToImageFilter.h>

 /** \class iAParametrizableLabelVotingImageFilter
  *
  * \brief This filter performs pixelwise voting among an arbitrary number
  * of input images, where each of them represents a segmentation of the same
  * scene (i.e., image).
  *
  * Label voting is a simple method of classifier combination applied to
  * image segmentation. Typically, the accuracy of the combined segmentation
  * exceeds the accuracy of any of the input segmentations. Voting is therefore
  * commonly used as a way of boosting segmentation performance.
  *
  * The use of label voting for combination of multiple segmentations is
  * described in
  *
  * T. Rohlfing and C. R. Maurer, Jr., "Multi-classifier framework for
  * atlas-based image segmentation," Pattern Recognition Letters, 2005.
  *
  * \par INPUTS
  * All input volumes to this filter must be segmentations of an image,
  * that is, they must have discrete pixel values where each value represents
  * a different segmented object.
  *
  * Input volumes must all contain the same size RequestedRegions. Not all
  * input images must contain all possible labels, but all label values must
  * have the same meaning in all images.
  *
  * \par OUTPUTS
  * The voting filter produces a single output volume. Each output pixel
  * contains the label that occurred most often among the labels assigned to
  * this pixel in all the input volumes, that is, the label that received the
  * maximum number of "votes" from the input pixels.. If the maximum number of
  * votes is not unique, i.e., if more than one label have a maximum number of
  * votes, an "undecided" label is assigned to that output pixel.
  *
  * By default, the label used for undecided pixels is the maximum label value
  * used in the input images plus one. Since it is possible for an image with
  * 8 bit pixel values to use all 256 possible label values, it is permissible
  * to combine 8 bit (i.e., byte) images into a 16 bit (i.e., short) output
  * image.
  *
  * \par PARAMETERS
  * The label used for "undecided" labels can be set using
  * SetLabelForUndecidedPixels. This functionality can be unset by calling
  * UnsetLabelForUndecidedPixels.
  *
  * \author Torsten Rohlfing, SRI International, Neuroscience Program
  *
  * \ingroup ITKLabelVoting
  */

enum OutputNumber
{
	AbsolutePercentage,
	DiffPercentage,
	Ratio,
	PixelEntropy
};

enum WeightType
{
	Equal,			//! every considered input gets equal vote
	LabelBased,     //! an input gets for each pixel a weight based on input idx and label at that pixel
	                //! (these weights need to be passed in via SetInputLabelWeights() method)
	Certainty,      //! an input is weighted based on the certainty (= inverse entropy) at that pixel
	                //! (requires probabilities to be set via
	FBGSBGDiff		//! an input is weighted based on the difference between first and second best guess
};

template< typename TInputImage, typename TOutputImage = TInputImage >
class iAParametrizableLabelVotingImageFilter :
	public itk::ImageToImageFilter< TInputImage, TOutputImage >
{
public:
	/** Standard class typedefs. */
	typedef iAParametrizableLabelVotingImageFilter                          Self;
	typedef itk::ImageToImageFilter< TInputImage, TOutputImage > Superclass;
	typedef itk::SmartPointer< Self >                            Pointer;
	typedef itk::SmartPointer< const Self >                      ConstPointer;

	/** Method for creation through the object factory. */
	itkNewMacro(Self);

	/** Run-time type information (and related methods) */
	itkTypeMacro(iAParametrizableLabelVotingImageFilter, ImageToImageFilter);

	/** Extract some information from the image types.  Dimensionality
	 * of the two images is assumed to be the same. */
	typedef typename TOutputImage::PixelType OutputPixelType;
	typedef typename TInputImage::PixelType  InputPixelType;

	/** Extract some information from the image types.  Dimensionality
	 * of the two images is assumed to be the same. */
	itkStaticConstMacro(InputImageDimension, int,
		TInputImage::ImageDimension);
	itkStaticConstMacro(ImageDimension, int,
		TOutputImage::ImageDimension);

	/** Typedefs */
	typedef TInputImage                           InputImageType;
	typedef TOutputImage                          OutputImageType;
	typedef typename InputImageType::ConstPointer InputImagePointer;
	typedef typename OutputImageType::Pointer     OutputImagePointer;
	typedef unsigned long                         LabelCountType;
	typedef typename Superclass::OutputImageRegionType OutputImageRegionType;
	typedef itk::Image<double, 3>                 DoubleImg;
	typedef itk::ImageRegionConstIterator<DoubleImg> ConstDblIt;

	/** Sets the percentage that has to be achieved as a minimum for the
	  majority vote to be accepted as majority
	  @param p the percentage (in the interval 0..1);
		  0 -> no minimum percentage
		  1 -> all images have to agree on a label
	*/
	void SetAbsoluteMinimumPercentage(double p)
	{
		m_AbsMinPercentage = p;
	}

	//! set a minimum threshold for the percentage difference between first and second best guess
	void SetMinimumDifferencePercentage(double p)
	{
		m_MinDiffPercentage = p;
	}

	//! set a minimum threshold on the ratio between first and second best guess
	void SetMinimumRatio(double r)
	{
		m_MinRatio = r;
	}

	//! set a maximum threshold for the pixel entropy
	void SetMaxPixelEntropy(double e)
	{
		m_MaxPixelEntropy = e;
	}

	//! Set label value for undecided pixels.
	void SetLabelForUndecidedPixels(const OutputPixelType l)
	{
		m_LabelForUndecidedPixels = l;
		m_HasLabelForUndecidedPixels = true;
		this->Modified();
	}

	/** Get label value used for undecided pixels.
	 * After updating the filter, this function returns the actual label value
	 * used for undecided pixels in the current output. Note that this value
	 * is overwritten when SetLabelForUndecidedPixels is called and the new
	 * value only becomes effective upon the next filter update.
	 */
	OutputPixelType GetLabelForUndecidedPixels() const
	{
		return m_LabelForUndecidedPixels;
	}

	//! determine the type of weight given each input in the voting process
	//! see WeightType for more details
	void SetWeightType(WeightType weightType)
	{
		m_weightType = weightType;
	}

	//! set probabilities for each label for one input
	//! in order for probability-related functionality to work, this method needs to be
	//! called once with each input idx
	void SetProbabilityImages(int inputIdx, std::vector<DoubleImg::Pointer> const & probImgs)
	{
		m_probImgs.insert(std::make_pair(inputIdx, probImgs));
	}

	//! sets the data determining which input is considered for voting on which label
	//! At each pixel the label value is checked, and an input is only considered if
	//! this input is considered a suitable voter for this label (i.e., if the pair
	//! <label, input index> is contained in the set given here)
	void SetInputLabelVotersSet(std::set<std::pair<int, int> > inputLabelVotersSet)
	{
		m_inputLabelVotersSet = inputLabelVotersSet;
	}

	//! set a weight for a pair of <label, input index>
	//! only used if WeightType LabelBased is used (see WeightType, SetWeightType)
	void SetInputLabelWeightMap(std::map<std::pair<int, int>, double> inputLabelWeightMap)
	{
		m_inputLabelWeightMap = inputLabelWeightMap;
	}

	//! Unset label value for undecided pixels and turn on automatic selection.
	void UnsetLabelForUndecidedPixels()
	{
		if (m_HasLabelForUndecidedPixels)
		{
			m_HasLabelForUndecidedPixels = false;
			this->Modified();
		}
	}

	DoubleImg::Pointer GetNumbers(int mode)
	{
		switch (mode)
		{
		case AbsolutePercentage: return m_imgAbsMinPerc;
		case DiffPercentage:     return m_imgMinDiffPerc;
		default:
		case Ratio:              return m_imgMinRatio;
		}
	}

	double GetUndecided() const
	{
		return m_undecidedPixels;
	}

#ifdef ITK_USE_CONCEPT_CHECKING
	// Begin concept checking
	itkConceptMacro(InputConvertibleToOutputCheck,
		(itk::Concept::Convertible< InputPixelType, OutputPixelType >));
	itkConceptMacro(IntConvertibleToInputCheck,
		(itk::Concept::Convertible< int, InputPixelType >));
	itkConceptMacro(SameDimensionCheck,
		(itk::Concept::SameDimension< InputImageDimension, ImageDimension >));
	itkConceptMacro(InputIntCheck,
		(itk::Concept::IsInteger< InputPixelType >));
	itkConceptMacro(IntConvertibleToOutputPixelType,
		(itk::Concept::Convertible< int, OutputPixelType >));
	itkConceptMacro(InputPlusIntCheck,
		(itk::Concept::AdditiveOperators< InputPixelType, int >));
	itkConceptMacro(InputIncrementDecrementOperatorsCheck,
		(itk::Concept::IncrementDecrementOperators< InputPixelType >));
	itkConceptMacro(OutputOStreamWritableCheck,
		(itk::Concept::OStreamWritable< OutputPixelType >));
	// End concept checking
#endif

protected:
	iAParametrizableLabelVotingImageFilter();
	virtual ~iAParametrizableLabelVotingImageFilter() {}

	/** Determine maximum label value in all input images and initialize
	 * global data. */
	void BeforeThreadedGenerateData() override;

	void ThreadedGenerateData
		(const OutputImageRegionType & outputRegionForThread, itk::ThreadIdType threadId) override;

	void PrintSelf(std::ostream &, itk::Indent) const override;

	/** Determine maximum value among all input images' pixels */
	InputPixelType ComputeMaximumInputValue();

private:
	iAParametrizableLabelVotingImageFilter(const Self &) =delete;
	void operator=(const Self &) =delete;

	OutputPixelType m_LabelForUndecidedPixels;
	bool            m_HasLabelForUndecidedPixels;
	size_t          m_TotalLabelCount;

	//! @{
	//! various parameters for influencing label voting result
	double          m_AbsMinPercentage;
	double          m_MinDiffPercentage;
	double          m_MinRatio;
	double			m_MaxPixelEntropy;
	typename DoubleImg::Pointer m_imgAbsMinPerc;
	typename DoubleImg::Pointer m_imgMinDiffPerc;
	typename DoubleImg::Pointer m_imgMinRatio;
	typename DoubleImg::Pointer m_imgPixelEntropy;
	std::map<int, std::vector<DoubleImg::Pointer> > m_probImgs;
	std::set<std::pair<int, int> > m_inputLabelVotersSet;
	std::map<std::pair<int, int>, double> m_inputLabelWeightMap;
	WeightType       m_weightType;
	double           m_undecidedPixels;
};

#ifndef ITK_MANUAL_INSTANTIATION
#include "iAParametrizableLabelVotingImageFilter.hxx"
#endif
