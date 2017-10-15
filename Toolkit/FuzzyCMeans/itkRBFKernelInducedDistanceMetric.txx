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
#ifndef __itkRBFKernelInducedDistanceMetric_txx
#define __itkRBFKernelInducedDistanceMetric_txx

#include "itkRBFKernelInducedDistanceMetric.h"

namespace itk
{
namespace Statistics
{

template< class TVector >
RBFKernelInducedDistanceMetric< TVector >
::RBFKernelInducedDistanceMetric(void)
{
  m_Sigma = 150.0;
  m_A = 2.0;
  m_B = 1.0;
}


template< class TVector >
inline double
RBFKernelInducedDistanceMetric< TVector >
::Evaluate(const MeasurementVectorType &x) const
{
  MeasurementVectorSizeType 
      measurementVectorSize = this->GetMeasurementVectorSize();
  if(measurementVectorSize == 0) 
    {
    itkExceptionMacro( << "Please set the MeasurementVectorSize first" );
    }
  MeasurementVectorTraitsType::Assert( this->GetOrigin(), measurementVectorSize,
    "RBFKernelInducedDistanceMetric::Evaluate Origin and input vector have different lengths" );

  double subExpression = NumericTraits< double >::Zero;

  for(unsigned int i = 0; i < measurementVectorSize; i++)
    {
    const double temp = this->GetOrigin()[i] - x[i];
    subExpression += vcl_pow(vcl_fabs(temp), this->m_A);
    }

  subExpression = - vcl_pow(subExpression, this->m_B);
  subExpression = subExpression / vcl_pow(this->m_Sigma, 2);

  const double distance = vcl_exp( subExpression );

  return distance;
}

template< class TVector >
inline double
RBFKernelInducedDistanceMetric< TVector >
::Evaluate( const MeasurementVectorType &x1,
            const MeasurementVectorType &x2 ) const
{
  MeasurementVectorSizeType 
      measurementVectorSize = this->GetMeasurementVectorSize();
  if(measurementVectorSize == 0) 
    {
    itkExceptionMacro( << "Please set the MeasurementVectorSize first" );
    }

  MeasurementVectorTraitsType::Assert( x1, measurementVectorSize,
    "RBFKernelInducedDistanceMetric::Evaluate First input vector and measurement vector set in the distance metric have unequal size." );

  MeasurementVectorTraitsType::Assert( x2, measurementVectorSize,
    "RBFKernelInducedDistanceMetric::Evaluate Second input vector and measurement vector set in the distance metric have unequal size." );

  double subExpression = NumericTraits< double >::Zero;

  for(unsigned int i = 0; i < measurementVectorSize; i++)
    {
    const double temp = x1[i] - x2[i];
    subExpression += vcl_pow(vcl_fabs(temp), this->m_A);
    }

  subExpression = - vcl_pow(subExpression, this->m_B);
  subExpression = subExpression / vcl_pow(this->m_Sigma, 2);

  const double distance = vcl_exp( subExpression );

  return distance;
}

template< class TVector >
inline double
RBFKernelInducedDistanceMetric< TVector >
::Evaluate(const ValueType &a, const ValueType &b) const
{
  double temp = vcl_fabs(static_cast< double >(a - b));

  temp = vcl_pow(temp, m_A);
  temp = - vcl_pow(temp, m_B);
  temp = temp / vcl_pow(m_Sigma, 2);

  return vcl_exp(temp);
}

template< class TVector >
void
RBFKernelInducedDistanceMetric< TVector >
::PrintSelf(std::ostream &os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);

  os << indent << "Sigma: " << m_Sigma << std::endl;
  os << indent << "A : " << m_A << std::endl;
  os << indent << "B : " << m_B << std::endl;
}

} // end of namespace Statistics
} // end of namespace itk

#endif // __itkRBFKernelInducedDistanceMetric_txx
