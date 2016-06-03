/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 
#ifndef FILTERLABELIMAGE_H
#define FILTERLABELIMAGE_H
// itk
#include <itkImageToImageFilter.h>
// std
#include <vector>

namespace itk
{
	template< class TImage>
	class FilterLabelImage :public ImageToImageFilter < TImage, TImage >
	{
	public:
		/** Standard class typedefs. */
		typedef FilterLabelImage						Self;
		typedef ImageToImageFilter< TImage, TImage >	Superclass;
		typedef SmartPointer< Self >					Pointer;

		/** Method for creation through the object factory. */
		itkNewMacro(Self);

		/** Run-time type information (and related methods). */
		itkTypeMacro(FilterLabelImage, ImageToImageFilter);

		void SetList(std::vector<unsigned short> list);

	protected:
		FilterLabelImage(){}
		~FilterLabelImage(){}

		/** Does the real work. */
		virtual void GenerateData();

	private:
		FilterLabelImage(const Self &); //purposely not implemented
		void operator=(const Self &);  //purposely not implemented
		std::vector<unsigned short> m_list;

	};

	template< class TImage>
	void itk::FilterLabelImage<TImage>::SetList(std::vector<unsigned short> list)
	{
		m_list = list;
	}

} //namespace ITK


#ifndef ITK_MANUAL_INSTANTIATION
#include "FilterLabelImage.hxx"
#endif


#endif // FILTERLABELIMAGE_H