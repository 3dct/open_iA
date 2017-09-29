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
#ifndef __itkKernelInducedDistanceMetric_txx
#define __itkKernelInducedDistanceMetric_txx

#include "itkKernelInducedDistanceMetric.h"

namespace itk
{
namespace Statistics
{

template< class TVector >
KernelInducedDistanceMetric< TVector >
::KernelInducedDistanceMetric()
{
  this->SetMeasurementVectorSize(TVector::Dimension);

  OriginType origin(TVector::Dimension);
  origin.Fill(0.0);
  this->SetOrigin(origin);
}

template< class TVector >
void
KernelInducedDistanceMetric< TVector >
::PrintSelf(std::ostream& os, itk::Indent indent) const
{
  Superclass::PrintSelf(os, indent);
}

} // end of namespace Statistics
} // end of namespace itk

#endif // __itkKernelInducedDistanceMetric_txx
