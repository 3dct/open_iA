/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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
#pragma once

// Eigenvalue pixel accessor to access vector of eigen value pixels
// as individual images 
template< class TPixel >
class EigenValueAccessor
{
public:
  typedef TPixel                     InternalType;
  typedef float                      ExternalType;
  
  inline ExternalType Get( const InternalType & input ) const 
	{
	  return static_cast<ExternalType>( input[m_EigenIdx] );
	}

  void SetEigenIdx( unsigned int i )
	{
	this->m_EigenIdx = i;
	}
  
private:
  unsigned int m_EigenIdx;
};


// Functor to get trace of the hessian matrix (laplacian of the image )
namespace Functor {  
 
	template< typename TInput, typename TOutput >
	class HessianToLaplacianFunction
	{
	public:
	  typedef typename TInput::RealValueType  RealValueType;
	  HessianToLaplacianFunction() {}
	  ~HessianToLaplacianFunction() {}
  
	  inline TOutput operator()( const TInput & x ) const
		{
		return static_cast< TOutput >( x(0,0) + x(1,1) + x(2,2) );
		}
	};

} // namespace Functor
