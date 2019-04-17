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
#include "iAToolsITK.h"

#include "iAMathUtility.h"
#include "iATypedCallHelper.h"

#include <itkExtractImageFilter.h>
#include <itkStatisticsImageFilter.h>

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
	else if (dynamic_cast<itk::Image<itk::RGBPixel< unsigned char >, iAITKIO::m_DIM> *>(imagePtr))
		result = itk::ImageIOBase::UCHAR;
	else if (dynamic_cast<itk::Image< itk::RGBPixel< char >, iAITKIO::m_DIM> *>(imagePtr))
		result = itk::ImageIOBase::CHAR;
	else if (dynamic_cast<itk::Image< itk::RGBPixel< short >, iAITKIO::m_DIM> *>(imagePtr))
		result = itk::ImageIOBase::SHORT;
	else if (dynamic_cast<itk::Image< itk::RGBPixel< unsigned short >, iAITKIO::m_DIM> *>(imagePtr))
		result = itk::ImageIOBase::USHORT;
	else if (dynamic_cast<itk::Image< itk::RGBPixel< int >, iAITKIO::m_DIM> *>(imagePtr))
		result = itk::ImageIOBase::INT;
	else if (dynamic_cast<itk::Image< itk::RGBPixel< unsigned int >, iAITKIO::m_DIM> *>(imagePtr))
		result = itk::ImageIOBase::UINT;
	else if (dynamic_cast<itk::Image< itk::RGBPixel< long >, iAITKIO::m_DIM> *>(imagePtr))
		result = itk::ImageIOBase::LONG;
	else if (dynamic_cast<itk::Image< itk::RGBPixel< unsigned long >, iAITKIO::m_DIM> *>(imagePtr))
		result = itk::ImageIOBase::ULONG;
	else if (dynamic_cast<itk::Image< itk::RGBPixel< float >, iAITKIO::m_DIM> *>(imagePtr))
		result = itk::ImageIOBase::FLOAT;
	else if (dynamic_cast<itk::Image<itk::RGBPixel< double >, iAITKIO::m_DIM> *>(imagePtr))
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
	else if (dynamic_cast<itk::Image<itk::RGBPixel< unsigned char >, iAITKIO::m_DIM> *>(imagePtr))
		result = itk::ImageIOBase::RGB;
	else if (dynamic_cast<itk::Image< itk::RGBPixel< char >, iAITKIO::m_DIM> *>(imagePtr))
		result = itk::ImageIOBase::RGB;
	else if (dynamic_cast<itk::Image< itk::RGBPixel< short >, iAITKIO::m_DIM> *>(imagePtr))
		result = itk::ImageIOBase::RGB;
	else if (dynamic_cast<itk::Image< itk::RGBPixel< unsigned short >, iAITKIO::m_DIM> *>(imagePtr))
		result = itk::ImageIOBase::RGB;
	else if (dynamic_cast<itk::Image< itk::RGBPixel< int >, iAITKIO::m_DIM> *>(imagePtr))
		result = itk::ImageIOBase::RGB;
	else if (dynamic_cast<itk::Image< itk::RGBPixel< unsigned int >, iAITKIO::m_DIM> *>(imagePtr))
		result = itk::ImageIOBase::RGB;
	else if (dynamic_cast<itk::Image< itk::RGBPixel< long >, iAITKIO::m_DIM> *>(imagePtr))
		result = itk::ImageIOBase::RGB;
	else if (dynamic_cast<itk::Image< itk::RGBPixel< unsigned long >, iAITKIO::m_DIM> *>(imagePtr))
		result = itk::ImageIOBase::RGB;
	else if (dynamic_cast<itk::Image< itk::RGBPixel< float >, iAITKIO::m_DIM> *>(imagePtr))
		result = itk::ImageIOBase::RGB;
	else if (dynamic_cast<itk::Image<itk::RGBPixel< double >, iAITKIO::m_DIM> *>(imagePtr))
		result = itk::ImageIOBase::RGB;

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
void alloc_image_tmpl2(int const size[iAITKIO::m_DIM], double const spacing[iAITKIO::m_DIM], iAITKIO::ImagePointer & result)
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


iAITKIO::ImagePointer AllocateImage(int const size[iAITKIO::m_DIM], double const spacing[iAITKIO::m_DIM], itk::ImageIOBase::IOComponentType type)
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


template <typename T>
void InternalExtractImage(iAITKIO::ImagePointer inImg, size_t const indexArr[iAITKIO::m_DIM], size_t const sizeArr[iAITKIO::m_DIM], iAITKIO::ImagePointer & outImg)
{
	typedef itk::Image< T, iAITKIO::m_DIM > ImageType;
	auto typedImg = dynamic_cast<ImageType *>(inImg.GetPointer());
	typedef itk::ExtractImageFilter< ImageType, ImageType > ExtractType;
	auto extractor = ExtractType::New();
	auto size = typedImg->GetLargestPossibleRegion().GetSize();
	typename ExtractType::InputImageRegionType::IndexType index;
	for (int i = 0; i < iAITKIO::m_DIM; ++i)
	{
		index[i] = clamp(static_cast<size_t>(0), size[i], indexArr[i]);
		size[i] = clamp(static_cast<size_t>(0), size[i] - index[i], sizeArr[i]);
	}
	typename ExtractType::InputImageRegionType region;
	region.SetIndex(index);
	region.SetSize(size);
	extractor->SetInput(typedImg);
	extractor->SetExtractionRegion(region);
	extractor->Update();
	outImg = SetIndexOffsetToZero<T>(extractor->GetOutput());
}

iAITKIO::ImagePointer ExtractImage(iAITKIO::ImagePointer inImg, size_t const indexArr[iAITKIO::m_DIM], size_t const sizeArr[iAITKIO::m_DIM])
{
	iAITKIO::ImagePointer outImg;
	ITK_TYPED_CALL(InternalExtractImage, GetITKScalarPixelType(inImg), inImg, indexArr, sizeArr, outImg);
	return outImg;
}

template <typename T>
void internalGetStatistics(iAITKIO::ImagePointer img, double* min, double* max, double* mean, double* stddev, double* vari, double* sum)
{
	typedef itk::Image< T, iAITKIO::m_DIM > ImageType;
	auto statisticsImageFilter = itk::StatisticsImageFilter<ImageType>::New();
	statisticsImageFilter->SetInput(dynamic_cast<ImageType*>(img.GetPointer()));
	statisticsImageFilter->Update();
	if (min)    *min = statisticsImageFilter->GetMinimum();
	if (max)    *max = statisticsImageFilter->GetMaximum();
	if (mean)   *mean = statisticsImageFilter->GetMean();
	if (stddev) *stddev = statisticsImageFilter->GetSigma();
	if (vari)   *vari = statisticsImageFilter->GetVariance();
	if (sum)    *sum = statisticsImageFilter->GetVariance();
}

void getStatistics(iAITKIO::ImagePointer img, double* min, double* max, double* mean, double* stddev, double* variance, double * sum)
{
	ITK_TYPED_CALL(internalGetStatistics, GetITKScalarPixelType(img), img, min, max, mean, stddev, variance, sum);
}
