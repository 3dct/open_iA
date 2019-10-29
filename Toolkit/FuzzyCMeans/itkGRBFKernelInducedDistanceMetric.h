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
#ifndef __itkGRBFKernelInducedDistanceMetric_h
#define __itkGRBFKernelInducedDistanceMetric_h

#include "itkNumericTraits.h"
#include "itkDistanceMetric.h"
#include "itkMeasurementVectorTraits.h"
#include "itkMacro.h"

#include "itkKernelInducedDistanceMetric.h"

namespace itk
{
namespace Statistics
{

/** \class GRBFKernelInducedDistanceMetric
 *
 * \brief Gaussian radial basis kernel distance function.
 *
 * This class is derived from KernelInducedDistanceMetric class and implements
 * the overloaded Evaluate() methods.
 *
 * \version 0.1
 *
 * \author Alberto Rey, Alfonso Castro and Bernardino Arcay. University of
 * A Coru&ntilde;a. Spain
 *
 * \sa KernelInducedDistanceMetric
 * \sa RBFKernelInducedDistanceMetric
 * \sa PolynomialKernelInducedDistanceMetric
 */
template< class TVector >
class GRBFKernelInducedDistanceMetric :
    public KernelInducedDistanceMetric< TVector >
{

public:

  /** Standard class typedefs. */
  typedef GRBFKernelInducedDistanceMetric         Self;
  typedef KernelInducedDistanceMetric< TVector >  Superclass;
  typedef SmartPointer< Self >                    Pointer;
  typedef SmartPointer< const Self >              ConstPointer;

  /** Typedef for the type of vector. */
  typedef typename Superclass::MeasurementVectorType MeasurementVectorType;

  /** Type of the component of a vector */
  typedef typename Superclass::ValueType ValueType;

  /** Typedef for the length of each measurement vector. */
  typedef typename Superclass::MeasurementVectorSizeType
      MeasurementVectorSizeType;

  /** Type of the origin. */
  typedef typename Superclass::OriginType OriginType;

  /** Vector information. */
  typedef typename itk::Statistics::MeasurementVectorTraits
      MeasurementVectorTraitsType;

  /** Run-time type information (and related methods). */
  itkTypeMacro(GRBFKernelInducedDistanceMetric, KernelInducedDistanceMetric);

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Get the a value. */
  itkGetConstMacro(A, double);

  /** Get the b value. */
  itkGetConstMacro(B, double);

  /** Get the sigma value. */
  itkGetConstMacro(Sigma, double);

  /** Set the a value. This value must be greater than 0. */
  itkSetClampMacro(A, double, 0.0, itk::NumericTraits< double >::max());

  /** Set the b value. This value is constrained between 1 and 2. */
  itkSetClampMacro(B, double, 1.0, 2.0);

  /** Set the sigma value. */
  itkSetMacro(Sigma, double);

  /** Gets the distance between the origin and x */
  double Evaluate(const MeasurementVectorType &x) const;

  /** Gets the distance between x1 and x2 */
  double Evaluate( const MeasurementVectorType &x1,
                   const MeasurementVectorType &x2 ) const;

  /** Gets the coordinate distance between a and b. NOTE: a and b should be
   * type of component. */
  double Evaluate(const ValueType &a, const ValueType &b) const;


protected:

  /* Constructor. */
  GRBFKernelInducedDistanceMetric(void);

  /* Destructor. */
  virtual ~GRBFKernelInducedDistanceMetric(void) {}

  /** Write the name-value pairs of the class data members to the supplied
   * output stream. */
  void PrintSelf(std::ostream &os, itk::Indent indent) const;


private:

  /** Radius of the GRBF function. */
  double m_Sigma;

  /** Exponent of the GRBF function. */
  double m_A;

  /** Exponent of the GRBF function. */
  double m_B;

};

} // end of namespace Statistics
} // end of namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkGRBFKernelInducedDistanceMetric.txx"
#endif

#endif // __itkGRBFKernelInducedDistanceMetric_h
