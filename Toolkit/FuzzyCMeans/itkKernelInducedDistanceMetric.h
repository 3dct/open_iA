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
#ifndef __itkKernelInducedDistanceMetric_h
#define __itkKernelInducedDistanceMetric_h

#include "itkNumericTraits.h"
#include "itkDistanceMetric.h"
#include "itkMeasurementVectorTraits.h"

namespace itk
{
namespace Statistics
{

/** \class KernelInducedDistanceMetric
 *
 * \brief Base class for kernel distance metrics.
 *
 * This class is derived from DistanceMetric class and declares common
 * interfaces for kernel distance metrics.
 *
 * As a function derived from DistanceMetric, users use Evaluate() method to
 * get result.
 *
 * \version 0.1
 *
 * \author Alberto Rey, Alfonso Castro and Bernardino Arcay. University of
 * A Coru&ntilde;a. Spain
 *
 * \sa DistanceMetric
 * \sa RBFKernelInducedDistanceMetric
 * \sa GRBFKernelInducedDistanceMetric
 * \sa PolynomialKernelInducedDistanceMetric
 */
template< class TVector >
class ITK_EXPORT KernelInducedDistanceMetric : public DistanceMetric< TVector >
{

public:

  /** Standard class typedefs. */
  typedef KernelInducedDistanceMetric Self;
  typedef DistanceMetric< TVector >   Superclass;
  typedef SmartPointer< Self >        Pointer;
  typedef SmartPointer< const Self >  ConstPointer;

#if ITK_VERSION_MAJOR < 4
#ifdef ITK_USE_REVIEW_STATISTICS
  /** Typedef to represent the measurement vector type. */
  typedef typename Superclass::MeasurementVectorType MeasurementVectorType;
  /** Typedef for the component of a vector. */
  typedef typename itk::Statistics::MeasurementVectorTraitsTypes<
                       MeasurementVectorType >::ValueType ValueType;
#else
  /** Typedef to represent the measurement vector type. */
  typedef TVector MeasurementVectorType;
  /** Type of the component of a vector. */
  typedef typename TVector::ValueType ValueType;
#endif
#else
  /** Typedef to represent the measurement vector type. */
  typedef typename Superclass::MeasurementVectorType MeasurementVectorType;
  /** Typedef for the component of a vector. */
  typedef typename itk::Statistics::MeasurementVectorTraitsTypes<
                       MeasurementVectorType >::ValueType ValueType;
#endif

  /** Typedef to represent the length of each measurement vector. */
  typedef typename Superclass::MeasurementVectorSizeType
      MeasurementVectorSizeType;

  /** Type of the origin. */
  typedef typename Superclass::OriginType OriginType;

  /** Run-time type information (and related methods). */
  itkTypeMacro(KernelInducedDistanceMetric, DistanceMetric);

  /** Gets the distance between the origin and x */
  virtual double Evaluate(const MeasurementVectorType &x) const = 0;

  /** Gets the distance between x1 and x2 */
  virtual double Evaluate( const MeasurementVectorType &x1,
                           const MeasurementVectorType &x2 ) const = 0;

  /** Gets the coordinate distance between a and b. NOTE: a and b should be
   * type of component. */
  virtual double Evaluate(const ValueType &a, const ValueType &b) const = 0;


protected:

  /** Constructor. */
  KernelInducedDistanceMetric();

  /** Destructor. */
  virtual ~KernelInducedDistanceMetric() {}

  /** Write the name-value pairs of the class data members to the supplied
   * output stream. */
  void PrintSelf(std::ostream &os, itk::Indent indent) const;

}; // end of class

} // end of namespace Statistics
} // end of namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkKernelInducedDistanceMetric.txx"
#endif

#endif // __itkKernelInducedDistanceMetric_h
