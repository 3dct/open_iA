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

#include "iAToolsITK.h"


itk::ImageIOBase::IOComponentType GetITKScalarPixelType(iAITKIO::ImagePointer image)
{
	itk::ImageIOBase::IOComponentType result = itk::ImageIOBase::UNKNOWNCOMPONENTTYPE;
	iAITKIO::ImageBaseType * imagePtr = image.GetPointer();

	if (dynamic_cast<itk::Image< unsigned char, iAITKIO::m_DIM> *>(imagePtr))
		result = itk::ImageIOBase::UCHAR;
	else if (dynamic_cast<itk::Image< char, iAITKIO::m_DIM> *>(imagePtr))
		result = itk::ImageIOBase::CHAR;
	else if (dynamic_cast<itk::Image< short, iAITKIO::m_DIM> *>(imagePtr))
		result = itk::ImageIOBase::SHORT;
	else if (dynamic_cast<itk::Image< unsigned short, iAITKIO::m_DIM> *>(imagePtr))
		result = itk::ImageIOBase::USHORT;
	else if (dynamic_cast<itk::Image< int, iAITKIO::m_DIM> *>(imagePtr))
		result = itk::ImageIOBase::INT;
	else if (dynamic_cast<itk::Image< unsigned int, iAITKIO::m_DIM> *>(imagePtr))
		result = itk::ImageIOBase::UINT;
	else if (dynamic_cast<itk::Image< long, iAITKIO::m_DIM> *>(imagePtr))
		result = itk::ImageIOBase::LONG;
	else if (dynamic_cast<itk::Image< unsigned long, iAITKIO::m_DIM> *>(imagePtr))
		result = itk::ImageIOBase::ULONG;
	else if (dynamic_cast<itk::Image< float, iAITKIO::m_DIM> *>(imagePtr))
		result = itk::ImageIOBase::FLOAT;
	else if (dynamic_cast<itk::Image< double, iAITKIO::m_DIM> *>(imagePtr))
		result = itk::ImageIOBase::DOUBLE;
	else if ( dynamic_cast<itk::Image<itk::RGBAPixel< unsigned char >, iAITKIO::m_DIM> *>( imagePtr ) )
		result = itk::ImageIOBase::UCHAR;
	else if ( dynamic_cast<itk::Image< itk::RGBAPixel< char >, iAITKIO::m_DIM> *>( imagePtr ) )
		result = itk::ImageIOBase::CHAR;
	else if ( dynamic_cast<itk::Image< itk::RGBAPixel< short >, iAITKIO::m_DIM> *>( imagePtr ) )
		result = itk::ImageIOBase::SHORT;
	else if ( dynamic_cast<itk::Image< itk::RGBAPixel< unsigned short >, iAITKIO::m_DIM> *>( imagePtr ) )
		result = itk::ImageIOBase::USHORT;
	else if ( dynamic_cast<itk::Image< itk::RGBAPixel< int >, iAITKIO::m_DIM> *>( imagePtr ) )
		result = itk::ImageIOBase::INT;
	else if ( dynamic_cast<itk::Image< itk::RGBAPixel< unsigned int >, iAITKIO::m_DIM> *>( imagePtr ) )
		result = itk::ImageIOBase::UINT;
	else if ( dynamic_cast<itk::Image< itk::RGBAPixel< long >, iAITKIO::m_DIM> *>( imagePtr ) )
		result = itk::ImageIOBase::LONG;
	else if ( dynamic_cast<itk::Image< itk::RGBAPixel< unsigned long >, iAITKIO::m_DIM> *>( imagePtr ) )
		result = itk::ImageIOBase::ULONG;
	else if ( dynamic_cast<itk::Image< itk::RGBAPixel< float >, iAITKIO::m_DIM> *>( imagePtr ) )
		result = itk::ImageIOBase::FLOAT;
	else if ( dynamic_cast<itk::Image<itk::RGBAPixel< double >, iAITKIO::m_DIM> *>( imagePtr ) )
		result = itk::ImageIOBase::DOUBLE;

	return result;
}


itk::ImageIOBase::IOPixelType GetITKPixelType( iAITKIO::ImagePointer image )
{
	itk::ImageIOBase::IOPixelType result = itk::ImageIOBase::UNKNOWNPIXELTYPE;
	iAITKIO::ImageBaseType * imagePtr = image.GetPointer();

	if ( dynamic_cast<itk::Image< unsigned char, iAITKIO::m_DIM> *>( imagePtr ) )
		result = itk::ImageIOBase::SCALAR;
	else if ( dynamic_cast<itk::Image< char, iAITKIO::m_DIM> *>( imagePtr ) )
		result = itk::ImageIOBase::SCALAR;
	else if ( dynamic_cast<itk::Image< short, iAITKIO::m_DIM> *>( imagePtr ) )
		result = itk::ImageIOBase::SCALAR;
	else if ( dynamic_cast<itk::Image< unsigned short, iAITKIO::m_DIM> *>( imagePtr ) )
		result = itk::ImageIOBase::SCALAR;
	else if ( dynamic_cast<itk::Image< int, iAITKIO::m_DIM> *>( imagePtr ) )
		result = itk::ImageIOBase::SCALAR;
	else if ( dynamic_cast<itk::Image< unsigned int, iAITKIO::m_DIM> *>( imagePtr ) )
		result = itk::ImageIOBase::SCALAR;
	else if ( dynamic_cast<itk::Image< long, iAITKIO::m_DIM> *>( imagePtr ) )
		result = itk::ImageIOBase::SCALAR;
	else if ( dynamic_cast<itk::Image< unsigned long, iAITKIO::m_DIM> *>( imagePtr ) )
		result = itk::ImageIOBase::SCALAR;
	else if ( dynamic_cast<itk::Image< float, iAITKIO::m_DIM> *>( imagePtr ) )
		result = itk::ImageIOBase::SCALAR;
	else if ( dynamic_cast<itk::Image< double, iAITKIO::m_DIM> *>( imagePtr ) )
		result = itk::ImageIOBase::SCALAR;
	else if ( dynamic_cast<itk::Image<itk::RGBAPixel< unsigned char >, iAITKIO::m_DIM> *>( imagePtr ) )
		result = itk::ImageIOBase::RGBA;
	else if ( dynamic_cast<itk::Image< itk::RGBAPixel< char >, iAITKIO::m_DIM> *>( imagePtr ) )
		result = itk::ImageIOBase::RGBA;
	else if ( dynamic_cast<itk::Image< itk::RGBAPixel< short >, iAITKIO::m_DIM> *>( imagePtr ) )
		result = itk::ImageIOBase::RGBA;
	else if ( dynamic_cast<itk::Image< itk::RGBAPixel< unsigned short >, iAITKIO::m_DIM> *>( imagePtr ) )
		result = itk::ImageIOBase::RGBA;
	else if ( dynamic_cast<itk::Image< itk::RGBAPixel< int >, iAITKIO::m_DIM> *>( imagePtr ) )
		result = itk::ImageIOBase::RGBA;
	else if ( dynamic_cast<itk::Image< itk::RGBAPixel< unsigned int >, iAITKIO::m_DIM> *>( imagePtr ) )
		result = itk::ImageIOBase::RGBA;
	else if ( dynamic_cast<itk::Image< itk::RGBAPixel< long >, iAITKIO::m_DIM> *>( imagePtr ) )
		result = itk::ImageIOBase::RGBA;
	else if ( dynamic_cast<itk::Image< itk::RGBAPixel< unsigned long >, iAITKIO::m_DIM> *>( imagePtr ) )
		result = itk::ImageIOBase::RGBA;
	else if ( dynamic_cast<itk::Image< itk::RGBAPixel< float >, iAITKIO::m_DIM> *>( imagePtr ) )
		result = itk::ImageIOBase::RGBA;
	else if ( dynamic_cast<itk::Image<itk::RGBAPixel< double >, iAITKIO::m_DIM> *>( imagePtr ) )
		result = itk::ImageIOBase::RGBA;

	return result;
}


template <class T>
void alloc_image_tmpl(iAITKIO::ImagePointer otherImg, iAITKIO::ImagePointer & result)
{
	typedef itk::Image<T, iAITKIO::m_DIM > ImageType;
	typedef typename ImageType::Pointer ImagePointer;

	ImagePointer image = ImageType::New();
	typename ImageType::RegionType reg(
		otherImg->GetLargestPossibleRegion().GetIndex(),
		otherImg->GetLargestPossibleRegion().GetSize()
	);
	image->SetRegions(reg);
	image->Allocate();
	image->FillBuffer(0);
	image->SetSpacing(otherImg->GetSpacing());
	result = image;
}


iAITKIO::ImagePointer AllocateImage(iAITKIO::ImagePointer img)
{
	iAITKIO::ImagePointer result;
	ITK_TYPED_CALL(alloc_image_tmpl, GetITKScalarPixelType(img), img, result);
	return result;
}

template <class T>
void alloc_image_tmpl2(int const size[3], double const spacing[3], iAITKIO::ImagePointer & result)
{
	typedef itk::Image<T, iAITKIO::m_DIM > ImageType;
	typedef typename ImageType::Pointer ImagePointer;

	ImagePointer image = ImageType::New();
	typename ImageType::IndexType idx;
	idx[0] = idx[1] = idx[2] = 0;
	typename ImageType::SizeType tsize;
	tsize[0] = size[0];
	tsize[1] = size[1];
	tsize[2] = size[2];
	typename ImageType::RegionType reg(
		idx,
		tsize
	);
	image->SetRegions(reg);
	image->Allocate();
	image->FillBuffer(0);
	image->SetSpacing(spacing);
	result = image;
}


// when moving that to core, we get unresolved external! why?
iAITKIO::ImagePointer AllocateImage(int const size[3], double const spacing[3], itk::ImageIOBase::IOComponentType type)
{
	iAITKIO::ImagePointer result;
	ITK_TYPED_CALL(alloc_image_tmpl2, type, size, spacing, result);
	return result;
}


void StoreImage(iAITKIO::ImagePointer image, QString const & filename, bool useCompression)
{
	iAITKIO::writeFile(filename, image, GetITKScalarPixelType(image), useCompression);
}

template <class TImage>
void GetITKPixel2(double & result, TImage* image, typename TImage::IndexType idx)
{
	result = image->GetPixel(idx);
}

template <class T>
void GetITKPixel(double & result, iAITKIO::ImagePointer img, iAITKIO::ImageBaseType::IndexType idx)
{
	typedef itk::Image<T, iAITKIO::m_DIM > ImageType;
	GetITKPixel2(result, dynamic_cast<ImageType*>(img.GetPointer()), idx);
}


double GetITKPixel(iAITKIO::ImagePointer img, iAITKIO::ImageBaseType::IndexType idx)
{
	double result;
	ITK_TYPED_CALL(GetITKPixel, GetITKScalarPixelType(img), result, img,  idx);
	return result;
}

template <class TImage>
void SetITKPixel2(double value, TImage* image, typename TImage::IndexType idx)
{
	image->SetPixel(idx, value);
}

template <class T>
void SetITKPixel(double value, iAITKIO::ImagePointer img, iAITKIO::ImageBaseType::IndexType idx)
{
	typedef itk::Image<T, iAITKIO::m_DIM > ImageType;
	SetITKPixel2(value, dynamic_cast<ImageType*>(img.GetPointer()), idx);
}


void SetITKPixel(iAITKIO::ImagePointer img, iAITKIO::ImageBaseType::IndexType idx, double value)
{
	ITK_TYPED_CALL(SetITKPixel, GetITKScalarPixelType(img), value, img, idx);
}
