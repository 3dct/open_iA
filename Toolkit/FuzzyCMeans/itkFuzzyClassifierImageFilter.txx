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
#ifndef __itkFuzzyClassifierImageFilter_txx
#define __itkFuzzyClassifierImageFilter_txx

#include "itkFuzzyClassifierImageFilter.h"

namespace itk
{

template< class TInputVectorImage, class TLabel >
void
FuzzyClassifierImageFilter< TInputVectorImage, TLabel >
::PrintSelf(std::ostream &os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);
}


template< class TInputVectorImage, class TLabel >
void
FuzzyClassifierImageFilter< TInputVectorImage, TLabel >
::GenerateData()
{
  itkDebugMacro("Starting GenerateData()");

  // static_cast<> is used here because the maximum class label must be
  // strictly less than the maximum number representable by the label type
  // (because background pixels are represented by label equal to the number
  // of classes).
  const LabelType numberOfClasses = static_cast< LabelType >(
                                        this->GetInput()->GetVectorLength() );

  if (numberOfClasses == 0)
    {
    itkExceptionMacro( <<
        "The number of components in the input Membership image is Zero" );
    }

  this->AllocateOutputs();

  this->Classify();
}


template< class TInputVectorImage, class TLabel >
void
FuzzyClassifierImageFilter< TInputVectorImage, TLabel >
::Classify()
{
  itkDebugMacro("Starting classification");

  OutputImagePointer labelsImage = this->GetOutput();

  OutputImageRegionType imageRegion = labelsImage->GetBufferedRegion();

  OutputImageIterator itrLabelsImage(labelsImage, imageRegion);
  InputImageConstIterator itrMembershipImage(this->GetInput(), imageRegion);

  // static_cast<> is used here because the maximum class label must be
  // strictly less than the maximum number representable by the label type
  // (because background pixels are represented by label equal to the number
  // of classes).
  const LabelType numberOfClasses = static_cast< LabelType >(
                                        this->GetInput()->GetVectorLength() );

  DecisionRulePointer decisionRule = DecisionRuleType::New();
  DecisionRuleType::MembershipVectorType membershipVector;

  InputImagePixelType currentMembershipPixel;

  for ( itrLabelsImage.GoToBegin(), itrMembershipImage.GoToBegin();
        !itrLabelsImage.IsAtEnd();
        ++itrLabelsImage, ++itrMembershipImage )
    {
    // Get fuzzy membership value.
    currentMembershipPixel = itrMembershipImage.Get();

    // If the background is ignored then background memberships (i.e. an array
    // filled with -1) have been used for background values in the membership
    // image. The label assigned to those pixels is the number of classes.
    if (currentMembershipPixel[0] == -1)
      {
      itrLabelsImage.Set(numberOfClasses);
      continue;
      }

    membershipVector.resize(currentMembershipPixel.Size());

    for(int i=0; i<numberOfClasses; ++i){
      membershipVector[i]=currentMembershipPixel[i];
    }

    itrLabelsImage.Set( static_cast< LabelType >(
                            decisionRule->Evaluate(membershipVector) ) );
    }
}

} // end of namespace itk

#endif // __itkFuzzyClassifierImageFilter_txx
