// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAToolsITK.h"

#include "iAMathUtility.h"
#include "iATypedCallHelper.h"

#include <itkExtractImageFilter.h>
#include <itkStatisticsImageFilter.h>

iAITKIO::ScalarType itkScalarType(iAITKIO::ImagePointer image)
{
	auto result = iAITKIO::ScalarType::UNKNOWNCOMPONENTTYPE;
	iAITKIO::ImageBaseType * imagePtr = image.GetPointer();

	if (dynamic_cast<itk::Image< unsigned char, iAITKIO::Dim> *>(imagePtr))
		result = iAITKIO::ScalarType::UCHAR;
	else if (dynamic_cast<itk::Image< char, iAITKIO::Dim> *>(imagePtr))
		result = iAITKIO::ScalarType::CHAR;
	else if (dynamic_cast<itk::Image< short, iAITKIO::Dim> *>(imagePtr))
		result = iAITKIO::ScalarType::SHORT;
	else if (dynamic_cast<itk::Image< unsigned short, iAITKIO::Dim> *>(imagePtr))
		result = iAITKIO::ScalarType::USHORT;
	else if (dynamic_cast<itk::Image< int, iAITKIO::Dim> *>(imagePtr))
		result = iAITKIO::ScalarType::INT;
	else if (dynamic_cast<itk::Image< unsigned int, iAITKIO::Dim> *>(imagePtr))
		result = iAITKIO::ScalarType::UINT;
	else if (dynamic_cast<itk::Image< long, iAITKIO::Dim> *>(imagePtr))
		result = iAITKIO::ScalarType::LONG;
	else if (dynamic_cast<itk::Image< unsigned long, iAITKIO::Dim> *>(imagePtr))
		result = iAITKIO::ScalarType::ULONG;
	else if (dynamic_cast<itk::Image< long long, iAITKIO::Dim> *>(imagePtr))
		result = iAITKIO::ScalarType::LONGLONG;
	else if (dynamic_cast<itk::Image< unsigned long long, iAITKIO::Dim> *>(imagePtr))
		result = iAITKIO::ScalarType::ULONGLONG;
	else if (dynamic_cast<itk::Image< float, iAITKIO::Dim> *>(imagePtr))
		result = iAITKIO::ScalarType::FLOAT;
	else if (dynamic_cast<itk::Image< double, iAITKIO::Dim> *>(imagePtr))
		result = iAITKIO::ScalarType::DOUBLE;
	else if ( dynamic_cast<itk::Image<itk::RGBAPixel< unsigned char >, iAITKIO::Dim> *>( imagePtr ) )
		result = iAITKIO::ScalarType::UCHAR;
	else if ( dynamic_cast<itk::Image< itk::RGBAPixel< char >, iAITKIO::Dim> *>( imagePtr ) )
		result = iAITKIO::ScalarType::CHAR;
	else if ( dynamic_cast<itk::Image< itk::RGBAPixel< short >, iAITKIO::Dim> *>( imagePtr ) )
		result = iAITKIO::ScalarType::SHORT;
	else if ( dynamic_cast<itk::Image< itk::RGBAPixel< unsigned short >, iAITKIO::Dim> *>( imagePtr ) )
		result = iAITKIO::ScalarType::USHORT;
	else if ( dynamic_cast<itk::Image< itk::RGBAPixel< int >, iAITKIO::Dim> *>( imagePtr ) )
		result = iAITKIO::ScalarType::INT;
	else if ( dynamic_cast<itk::Image< itk::RGBAPixel< unsigned int >, iAITKIO::Dim> *>( imagePtr ) )
		result = iAITKIO::ScalarType::UINT;
	else if ( dynamic_cast<itk::Image< itk::RGBAPixel< long >, iAITKIO::Dim> *>( imagePtr ) )
		result = iAITKIO::ScalarType::LONG;
	else if ( dynamic_cast<itk::Image< itk::RGBAPixel< unsigned long >, iAITKIO::Dim> *>( imagePtr ) )
		result = iAITKIO::ScalarType::ULONG;
	else if (dynamic_cast<itk::Image< itk::RGBAPixel< long long >, iAITKIO::Dim> *>(imagePtr))
		result = iAITKIO::ScalarType::LONGLONG;
	else if (dynamic_cast<itk::Image< itk::RGBAPixel< unsigned long long >, iAITKIO::Dim> *>(imagePtr))
		result = iAITKIO::ScalarType::ULONGLONG;
	else if (dynamic_cast<itk::Image< itk::RGBAPixel< float >, iAITKIO::Dim> *>(imagePtr))
		result = iAITKIO::ScalarType::FLOAT;
	else if ( dynamic_cast<itk::Image<itk::RGBAPixel< double >, iAITKIO::Dim> *>( imagePtr ) )
		result = iAITKIO::ScalarType::DOUBLE;
	else if (dynamic_cast<itk::Image<itk::RGBPixel< unsigned char >, iAITKIO::Dim> *>(imagePtr))
		result = iAITKIO::ScalarType::UCHAR;
	else if (dynamic_cast<itk::Image< itk::RGBPixel< char >, iAITKIO::Dim> *>(imagePtr))
		result = iAITKIO::ScalarType::CHAR;
	else if (dynamic_cast<itk::Image< itk::RGBPixel< short >, iAITKIO::Dim> *>(imagePtr))
		result = iAITKIO::ScalarType::SHORT;
	else if (dynamic_cast<itk::Image< itk::RGBPixel< unsigned short >, iAITKIO::Dim> *>(imagePtr))
		result = iAITKIO::ScalarType::USHORT;
	else if (dynamic_cast<itk::Image< itk::RGBPixel< int >, iAITKIO::Dim> *>(imagePtr))
		result = iAITKIO::ScalarType::INT;
	else if (dynamic_cast<itk::Image< itk::RGBPixel< unsigned int >, iAITKIO::Dim> *>(imagePtr))
		result = iAITKIO::ScalarType::UINT;
	else if (dynamic_cast<itk::Image< itk::RGBPixel< long >, iAITKIO::Dim> *>(imagePtr))
		result = iAITKIO::ScalarType::LONG;
	else if (dynamic_cast<itk::Image< itk::RGBPixel< unsigned long >, iAITKIO::Dim> *>(imagePtr))
		result = iAITKIO::ScalarType::ULONG;
	else if (dynamic_cast<itk::Image< itk::RGBPixel< long long >, iAITKIO::Dim> *>(imagePtr))
		result = iAITKIO::ScalarType::LONGLONG;
	else if (dynamic_cast<itk::Image< itk::RGBPixel< unsigned long long >, iAITKIO::Dim> *>(imagePtr))
		result = iAITKIO::ScalarType::ULONGLONG;
	else if (dynamic_cast<itk::Image< itk::RGBPixel< float >, iAITKIO::Dim> *>(imagePtr))
		result = iAITKIO::ScalarType::FLOAT;
	else if (dynamic_cast<itk::Image<itk::RGBPixel< double >, iAITKIO::Dim> *>(imagePtr))
		result = iAITKIO::ScalarType::DOUBLE;

	return result;
}


iAITKIO::PixelType itkPixelType( iAITKIO::ImagePointer image )
{
	auto result = iAITKIO::PixelType::UNKNOWNPIXELTYPE;
	iAITKIO::ImageBaseType * imagePtr = image.GetPointer();

	if ( dynamic_cast<itk::Image< unsigned char, iAITKIO::Dim> *>( imagePtr ) )
		result = iAITKIO::PixelType::SCALAR;
	else if ( dynamic_cast<itk::Image< char, iAITKIO::Dim> *>( imagePtr ) )
		result = iAITKIO::PixelType::SCALAR;
	else if ( dynamic_cast<itk::Image< short, iAITKIO::Dim> *>( imagePtr ) )
		result = iAITKIO::PixelType::SCALAR;
	else if ( dynamic_cast<itk::Image< unsigned short, iAITKIO::Dim> *>( imagePtr ) )
		result = iAITKIO::PixelType::SCALAR;
	else if ( dynamic_cast<itk::Image< int, iAITKIO::Dim> *>( imagePtr ) )
		result = iAITKIO::PixelType::SCALAR;
	else if ( dynamic_cast<itk::Image< unsigned int, iAITKIO::Dim> *>( imagePtr ) )
		result = iAITKIO::PixelType::SCALAR;
	else if ( dynamic_cast<itk::Image< long, iAITKIO::Dim> *>( imagePtr ) )
		result = iAITKIO::PixelType::SCALAR;
	else if ( dynamic_cast<itk::Image< unsigned long, iAITKIO::Dim> *>( imagePtr ) )
		result = iAITKIO::PixelType::SCALAR;
	else if ( dynamic_cast<itk::Image< float, iAITKIO::Dim> *>( imagePtr ) )
		result = iAITKIO::PixelType::SCALAR;
	else if ( dynamic_cast<itk::Image< double, iAITKIO::Dim> *>( imagePtr ) )
		result = iAITKIO::PixelType::SCALAR;
	else if ( dynamic_cast<itk::Image<itk::RGBAPixel< unsigned char >, iAITKIO::Dim> *>( imagePtr ) )
		result = iAITKIO::PixelType::RGBA;
	else if ( dynamic_cast<itk::Image< itk::RGBAPixel< char >, iAITKIO::Dim> *>( imagePtr ) )
		result = iAITKIO::PixelType::RGBA;
	else if ( dynamic_cast<itk::Image< itk::RGBAPixel< short >, iAITKIO::Dim> *>( imagePtr ) )
		result = iAITKIO::PixelType::RGBA;
	else if ( dynamic_cast<itk::Image< itk::RGBAPixel< unsigned short >, iAITKIO::Dim> *>( imagePtr ) )
		result = iAITKIO::PixelType::RGBA;
	else if ( dynamic_cast<itk::Image< itk::RGBAPixel< int >, iAITKIO::Dim> *>( imagePtr ) )
		result = iAITKIO::PixelType::RGBA;
	else if ( dynamic_cast<itk::Image< itk::RGBAPixel< unsigned int >, iAITKIO::Dim> *>( imagePtr ) )
		result = iAITKIO::PixelType::RGBA;
	else if ( dynamic_cast<itk::Image< itk::RGBAPixel< long >, iAITKIO::Dim> *>( imagePtr ) )
		result = iAITKIO::PixelType::RGBA;
	else if ( dynamic_cast<itk::Image< itk::RGBAPixel< unsigned long >, iAITKIO::Dim> *>( imagePtr ) )
		result = iAITKIO::PixelType::RGBA;
	else if ( dynamic_cast<itk::Image< itk::RGBAPixel< float >, iAITKIO::Dim> *>( imagePtr ) )
		result = iAITKIO::PixelType::RGBA;
	else if ( dynamic_cast<itk::Image<itk::RGBAPixel< double >, iAITKIO::Dim> *>( imagePtr ) )
		result = iAITKIO::PixelType::RGBA;
	else if (dynamic_cast<itk::Image<itk::RGBPixel< unsigned char >, iAITKIO::Dim> *>(imagePtr))
		result = iAITKIO::PixelType::RGB;
	else if (dynamic_cast<itk::Image< itk::RGBPixel< char >, iAITKIO::Dim> *>(imagePtr))
		result = iAITKIO::PixelType::RGB;
	else if (dynamic_cast<itk::Image< itk::RGBPixel< short >, iAITKIO::Dim> *>(imagePtr))
		result = iAITKIO::PixelType::RGB;
	else if (dynamic_cast<itk::Image< itk::RGBPixel< unsigned short >, iAITKIO::Dim> *>(imagePtr))
		result = iAITKIO::PixelType::RGB;
	else if (dynamic_cast<itk::Image< itk::RGBPixel< int >, iAITKIO::Dim> *>(imagePtr))
		result = iAITKIO::PixelType::RGB;
	else if (dynamic_cast<itk::Image< itk::RGBPixel< unsigned int >, iAITKIO::Dim> *>(imagePtr))
		result = iAITKIO::PixelType::RGB;
	else if (dynamic_cast<itk::Image< itk::RGBPixel< long >, iAITKIO::Dim> *>(imagePtr))
		result = iAITKIO::PixelType::RGB;
	else if (dynamic_cast<itk::Image< itk::RGBPixel< unsigned long >, iAITKIO::Dim> *>(imagePtr))
		result = iAITKIO::PixelType::RGB;
	else if (dynamic_cast<itk::Image< itk::RGBPixel< float >, iAITKIO::Dim> *>(imagePtr))
		result = iAITKIO::PixelType::RGB;
	else if (dynamic_cast<itk::Image<itk::RGBPixel< double >, iAITKIO::Dim> *>(imagePtr))
		result = iAITKIO::PixelType::RGB;

	return result;
}


template <class T>
void alloc_image_tmpl(iAITKIO::ImagePointer otherImg, iAITKIO::ImagePointer & result)
{
	typedef itk::Image<T, iAITKIO::Dim > ImageType;
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


iAITKIO::ImagePointer allocateImage(iAITKIO::ImagePointer img)
{
	iAITKIO::ImagePointer result;
	ITK_TYPED_CALL(alloc_image_tmpl, itkScalarType(img), img, result);
	return result;
}

template <class T>
void alloc_image_tmpl2(int const size[iAITKIO::Dim], double const spacing[iAITKIO::Dim], iAITKIO::ImagePointer & result)
{
	typedef itk::Image<T, iAITKIO::Dim > ImageType;
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


iAITKIO::ImagePointer allocateImage(int const size[iAITKIO::Dim], double const spacing[iAITKIO::Dim], iAITKIO::ScalarType type)
{
	iAITKIO::ImagePointer result;
	ITK_TYPED_CALL(alloc_image_tmpl2, type, size, spacing, result);
	return result;
}

void storeImage(iAITKIO::ImagePtr image, QString const & filename, bool useCompression, iAProgress const * p)
{
	iAITKIO::writeFile(filename, image, itkScalarType(image), useCompression, p);
}

void mapWorldToVoxelCoords(iAITKIO::ImagePointer img, double const* worldCoord, double * voxelCoord)
{
	auto imgSpacing = img->GetSpacing();
	auto imgOrigin = img->GetOrigin();
	auto voxelStart = img->GetLargestPossibleRegion().GetIndex();
	auto voxelSize = img->GetLargestPossibleRegion().GetSize();
	// coords will contain voxel coordinates for the given channel
	for (int i = 0; i < 3; ++i)
	{
		auto minIdx = static_cast<double>(voxelStart[i]);
		auto maxIdx = static_cast<double>(voxelStart[i] + voxelSize[i]) - std::numeric_limits<double>::epsilon();
		auto value = (worldCoord[i] - imgOrigin[i]) / imgSpacing[i] + 0.5;	// + 0.5 to correct for BorderOn
		voxelCoord[i] = clamp(minIdx, maxIdx, value);
	}
	// TODO: check for negative origin images!
}

itk::Index<3> mapWorldCoordsToIndex(iAITKIO::ImagePointer img, double const* worldCoord)
{
	double voxelCoord[3];
	mapWorldToVoxelCoords(img, worldCoord, voxelCoord);
	return itk::Index<3>({
		static_cast<itk::IndexValueType>(voxelCoord[0]),
		static_cast<itk::IndexValueType>(voxelCoord[1]),
		static_cast<itk::IndexValueType>(voxelCoord[2]) });
}

template <class TImage>
void itkPixel2(double & result, TImage* image, typename TImage::IndexType idx)
{
	result = image->GetPixel(idx);
}

template <class T>
void itkPixel(double & result, iAITKIO::ImagePointer img, iAITKIO::ImageBaseType::IndexType idx)
{
	typedef itk::Image<T, iAITKIO::Dim > ImageType;
	itkPixel2(result, dynamic_cast<ImageType*>(img.GetPointer()), idx);
}


double itkPixel(iAITKIO::ImagePointer img, iAITKIO::ImageBaseType::IndexType idx)
{
	double result;
	ITK_TYPED_CALL(itkPixel, itkScalarType(img), result, img,  idx);
	return result;
}

template <class TImage>
void setITKPixel2(double value, TImage* image, typename TImage::IndexType idx)
{
	image->SetPixel(idx, value);
}

template <class T>
void setITKPixel(double value, iAITKIO::ImagePointer img, iAITKIO::ImageBaseType::IndexType idx)
{
	typedef itk::Image<T, iAITKIO::Dim > ImageType;
	setITKPixel2(value, dynamic_cast<ImageType*>(img.GetPointer()), idx);
}


void setITKPixel(iAITKIO::ImagePointer img, iAITKIO::ImageBaseType::IndexType idx, double value)
{
	ITK_TYPED_CALL(setITKPixel, itkScalarType(img), value, img, idx);
}


template <typename T>
void internalExtractImage(iAITKIO::ImagePointer inImg, size_t const indexArr[iAITKIO::Dim], size_t const sizeArr[iAITKIO::Dim], iAITKIO::ImagePointer & outImg)
{
	typedef itk::Image< T, iAITKIO::Dim > ImageType;
	auto typedImg = dynamic_cast<ImageType *>(inImg.GetPointer());
	typedef itk::ExtractImageFilter< ImageType, ImageType > ExtractType;
	auto extractor = ExtractType::New();
	auto size = typedImg->GetLargestPossibleRegion().GetSize();
	typename ExtractType::InputImageRegionType::IndexType index;
	for (int i = 0; i < iAITKIO::Dim; ++i)
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
	outImg = setIndexOffsetToZero<T>(extractor->GetOutput());
}

iAITKIO::ImagePointer extractImage(iAITKIO::ImagePointer inImg, size_t const indexArr[iAITKIO::Dim], size_t const sizeArr[iAITKIO::Dim])
{
	iAITKIO::ImagePointer outImg;
	ITK_TYPED_CALL(internalExtractImage, itkScalarType(inImg), inImg, indexArr, sizeArr, outImg);
	return outImg;
}

template <typename T>
void internalGetStatistics(iAITKIO::ImagePointer img, double* min, double* max, double* mean, double* stddev, double* vari, double* sum)
{
	typedef itk::Image< T, iAITKIO::Dim > ImageType;
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
	ITK_TYPED_CALL(internalGetStatistics, itkScalarType(img), img, min, max, mean, stddev, variance, sum);
}
