/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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

#include "iAFileUtils.h"    // getLocalEncodingFileName
#include "iALog.h"
#include "iAITKIO.h"

#include "iAbase_export.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-overflow"
#include <itkCastImageFilter.h>
#include <itkChangeInformationImageFilter.h>
#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>
#include <itkImageRegionConstIterator.h>
#include <itkImageRegionIterator.h>
#include <itkRescaleIntensityImageFilter.h>
#pragma GCC diagnostic pop

#include <QString>

// TODO: unify with iAITKIO !

iAbase_API iAITKIO::ScalarType itkScalarType(iAITKIO::ImagePointer image);
iAbase_API iAITKIO::PixelType itkPixelType( iAITKIO::ImagePointer image );
iAbase_API iAITKIO::ImagePointer allocateImage(iAITKIO::ImagePointer img);
iAbase_API iAITKIO::ImagePointer allocateImage(int const size[iAITKIO::Dim], double const spacing[iAITKIO::Dim], iAITKIO::ScalarType scalarType);
iAbase_API void storeImage(iAITKIO::ImagePtr image, QString const & filename, bool useCompression);

//! Translate from world coordinates to voxel coordinates for the given image
//! @param img a VTK image
//! @param worldCoord world (=scene) coordinates (3 components: x, y, z)
//! @param voxelCoord place for storing 3 components of voxel coordinates in img for the given world coordinates (clamped)
iAbase_API itk::Index<3> mapWorldCoordsToIndex(iAITKIO::ImagePointer img, double const* worldCoord);
iAbase_API void mapWorldToVoxelCoords(iAITKIO::ImagePointer img, double const* worldCoord, double * voxelCoord);

//! @{
//! Generic access to pixels of any ITK image as double.
//! Slow! If you need to access more than a few pixels,
//! convert the whole image first (maybe using templates) and then access directly!
iAbase_API double itkPixel(iAITKIO::ImagePointer img, iAITKIO::ImageBaseType::IndexType idx);
iAbase_API void setITKPixel(iAITKIO::ImagePointer img, iAITKIO::ImageBaseType::IndexType idx, double value);
//! @}

//! extract part of an image as a new file
iAbase_API iAITKIO::ImagePointer extractImage(iAITKIO::ImagePointer inImg, size_t const indexArr[iAITKIO::Dim], size_t const sizeArr[iAITKIO::Dim]);

//! set index offset of an image to (0,0,0)
//iAbase_API iAITKIO::ImagePointer setIndexOffsetToZero(iAITKIO::ImagePointer inImg);
template <typename T>
typename itk::Image<T, iAITKIO::Dim>::Pointer setIndexOffsetToZero(typename itk::Image<T, iAITKIO::Dim>::Pointer inImg)
{
	// change output image index offset to zero
	typedef itk::Image<T, iAITKIO::Dim> ImageType;
	typename ImageType::IndexType idx; idx.Fill(0);
	typename ImageType::PointType origin; origin.Fill(0);
	typename ImageType::RegionType outreg;
	auto size = inImg->GetLargestPossibleRegion().GetSize();
	outreg.SetIndex(idx);
	outreg.SetSize(size);
	auto refimage = ImageType::New();
	refimage->SetRegions(outreg);
	refimage->SetOrigin(origin);
	refimage->SetSpacing(inImg->GetSpacing());
	refimage->Allocate();
	typedef itk::ChangeInformationImageFilter<ImageType> CIIFType;
	auto changeFilter = CIIFType::New();
	changeFilter->SetInput(inImg);
	changeFilter->UseReferenceImageOn();
	changeFilter->SetReferenceImage(refimage);
	changeFilter->SetChangeRegion(true);
	changeFilter->Update();
	return changeFilter->GetOutput();
}

//! Source: http://itk.org/Wiki/ITK/Examples/Utilities/DeepCopy
template<typename TImage>
void deepCopy(typename TImage::Pointer input, typename TImage::Pointer output)
{
	output->SetRegions(input->GetLargestPossibleRegion());
	output->SetSpacing(input->GetSpacing());
	output->Allocate();

	itk::ImageRegionConstIterator<TImage> inputIterator(input, input->GetLargestPossibleRegion());
	itk::ImageRegionIterator<TImage> outputIterator(output, output->GetLargestPossibleRegion());

	while(!inputIterator.IsAtEnd())
	{
		outputIterator.Set(inputIterator.Get());
		++inputIterator;
		++outputIterator;
	}
}

template <typename TImage>
typename TImage::Pointer createImage(typename TImage::SizeType size, typename TImage::SpacingType spacing)
{
	typename TImage::Pointer image = TImage::New();
	typename TImage::IndexType start;
	start.Fill(0);
	typename TImage::RegionType region(start, size);
	image->SetRegions(region);
	image->Allocate();
	image->FillBuffer(0);
	image->SetSpacing(spacing);
	return image;
}

template <typename TImage>
typename TImage::Pointer createImage(typename TImage::Pointer otherImg)
{
	typename TImage::Pointer image = TImage::New();
	typename TImage::RegionType reg(
		otherImg->GetLargestPossibleRegion().GetIndex(),
		otherImg->GetLargestPossibleRegion().GetSize()
	);
	image->SetRegions(reg);
	image->Allocate();
	image->FillBuffer(0);
	image->SetSpacing(otherImg->GetSpacing());
	return image;
}

template <typename TImage>
void storeImageOfType(TImage * image, QString const & filename, bool useCompression = true)
{
	typename itk::ImageFileWriter<TImage>::Pointer writer = itk::ImageFileWriter<TImage>::New();
	try
	{
		writer->SetFileName(getLocalEncodingFileName(filename).c_str());
		writer->SetUseCompression(useCompression);
		writer->SetInput(image);
		writer->Update();
	}
	catch (itk::ExceptionObject const & e)
	{
		LOG(lvlError, QString("Error while writing image file '%1': %2")
			.arg(filename)
			.arg(e.what()));
	}
}

template<typename SourceImageType, typename ResultImageType>
iAITKIO::ImagePointer internalCastImageTo(iAITKIO::ImagePointer img)
{
	typedef itk::CastImageFilter<SourceImageType, ResultImageType> CastType;
	typename CastType::Pointer cast = CastType::New();
	cast->SetInput(dynamic_cast<SourceImageType*>(img.GetPointer()));
	cast->Update();
	return cast->GetOutput();
}

// mapping from pixel type to itk component type ID:
template<typename T> struct iAITKTypeMapper { static const iAITKIO::ScalarType ID = iAITKIO::ScalarType::UNKNOWNCOMPONENTTYPE; };
//template <>                   class iAITKTypeMapper { int ID; };
template<> struct iAITKTypeMapper<unsigned char>       { static const iAITKIO::ScalarType ID = iAITKIO::ScalarType::UCHAR; };
template<> struct iAITKTypeMapper<char>                { static const iAITKIO::ScalarType ID = iAITKIO::ScalarType::CHAR; };
template<> struct iAITKTypeMapper<unsigned short>      { static const iAITKIO::ScalarType ID = iAITKIO::ScalarType::USHORT; };
template<> struct iAITKTypeMapper<short>               { static const iAITKIO::ScalarType ID = iAITKIO::ScalarType::SHORT; };
template<> struct iAITKTypeMapper<unsigned int>        { static const iAITKIO::ScalarType ID = iAITKIO::ScalarType::UINT; };
template<> struct iAITKTypeMapper<int>                 { static const iAITKIO::ScalarType ID = iAITKIO::ScalarType::INT; };
template<> struct iAITKTypeMapper<unsigned long>       { static const iAITKIO::ScalarType ID = iAITKIO::ScalarType::ULONG; };
template<> struct iAITKTypeMapper<long>                { static const iAITKIO::ScalarType ID = iAITKIO::ScalarType::LONG; };
template<> struct iAITKTypeMapper <unsigned long long> { static const iAITKIO::ScalarType ID = iAITKIO::ScalarType::ULONGLONG; };
template<> struct iAITKTypeMapper <long long>          { static const iAITKIO::ScalarType ID = iAITKIO::ScalarType::LONGLONG; };
template<> struct iAITKTypeMapper<float>               { static const iAITKIO::ScalarType ID = iAITKIO::ScalarType::FLOAT; };
template<> struct iAITKTypeMapper<double>              { static const iAITKIO::ScalarType ID = iAITKIO::ScalarType::DOUBLE; };

//! Cast pixel type of image to given ResultPixelType.
//! If input image already has that pixel type, the given input image is returned.
template<typename ResultPixelType>
iAITKIO::ImagePointer castImageTo(iAITKIO::ImagePointer img)
{
	// optimization: don't cast if already in desired type:
	if (itkScalarType(img) == iAITKTypeMapper<ResultPixelType>::ID)
	{
		return img;
	}
	// can I retrieve number of dimensions somehow? otherwise assume 3 fixed?
	switch (itkScalarType(img))
	{
		case iAITKIO::ScalarType::UCHAR:
			return internalCastImageTo<itk::Image<unsigned char, 3>, itk::Image<ResultPixelType, 3> >(img);
		case iAITKIO::ScalarType::CHAR:
			return internalCastImageTo<itk::Image<char, 3>, itk::Image<ResultPixelType, 3> >(img);
		case iAITKIO::ScalarType::SHORT:
			return internalCastImageTo<itk::Image<short, 3>, itk::Image<ResultPixelType, 3> >(img);
		case iAITKIO::ScalarType::USHORT:
			return internalCastImageTo<itk::Image<unsigned short, 3>, itk::Image<ResultPixelType, 3> >(img);
		case iAITKIO::ScalarType::INT:
			return internalCastImageTo<itk::Image<int, 3>, itk::Image<ResultPixelType, 3> >(img);
		case iAITKIO::ScalarType::UINT:
			return internalCastImageTo<itk::Image<unsigned int, 3>, itk::Image<ResultPixelType, 3> >(img);
		case iAITKIO::ScalarType::LONG:
			return internalCastImageTo<itk::Image<long, 3>, itk::Image<ResultPixelType, 3> >(img);
		case iAITKIO::ScalarType::ULONG:
			return internalCastImageTo<itk::Image<unsigned long, 3>, itk::Image<ResultPixelType, 3> >(img);
		case iAITKIO::ScalarType::LONGLONG:
			return internalCastImageTo<itk::Image<long long, 3>, itk::Image<ResultPixelType, 3> >(img);
		case iAITKIO::ScalarType::ULONGLONG:
			return internalCastImageTo<itk::Image<unsigned long long, 3>, itk::Image<ResultPixelType, 3> >(img);
		case iAITKIO::ScalarType::FLOAT:
			return internalCastImageTo<itk::Image<float, 3>, itk::Image<ResultPixelType, 3> >(img);
		default:
			LOG(lvlError, "Invalid/Unknown itk pixel datatype in rescale!");
#if __cplusplus >= 201703L
			[[fallthrough]];
#endif
			// fall through
		case iAITKIO::ScalarType::DOUBLE:
			return internalCastImageTo<itk::Image<double, 3>, itk::Image<ResultPixelType, 3> >(img);
	}
}

template<typename SourceImageType, typename ResultImageType>
iAITKIO::ImagePointer internalRescaleImageTo(iAITKIO::ImagePointer img, double min, double max)
{
	typedef itk::RescaleIntensityImageFilter<SourceImageType, ResultImageType> RescaleType;
	typename RescaleType::Pointer rescale = RescaleType::New();
	rescale->SetInput(dynamic_cast<SourceImageType*>(img.GetPointer()));
	rescale->SetOutputMinimum(min);
	rescale->SetOutputMaximum(max);
	rescale->Update();
	return rescale->GetOutput();
}

template<typename ResultPixelType>
iAITKIO::ImagePointer rescaleImageTo(iAITKIO::ImagePointer img, double min, double max)
{
	// can I retrieve number of dimensions somehow? otherwise assume 3 fixed?
	switch (itkScalarType(img))
	{
	case iAITKIO::ScalarType::UCHAR:
		return internalRescaleImageTo<itk::Image<unsigned char, 3>, itk::Image<ResultPixelType, 3> >(img, min, max);
	case iAITKIO::ScalarType::CHAR:
		return internalRescaleImageTo<itk::Image<char, 3>, itk::Image<ResultPixelType, 3> >(img, min, max);
	case iAITKIO::ScalarType::SHORT:
		return internalRescaleImageTo<itk::Image<short, 3>, itk::Image<ResultPixelType, 3> >(img, min, max);
	case iAITKIO::ScalarType::USHORT:
		return internalRescaleImageTo<itk::Image<unsigned short, 3>, itk::Image<ResultPixelType, 3> >(img, min, max);
	case iAITKIO::ScalarType::INT:
		return internalRescaleImageTo<itk::Image<int, 3>, itk::Image<ResultPixelType, 3> >(img, min, max);
	case iAITKIO::ScalarType::UINT:
		return internalRescaleImageTo<itk::Image<unsigned int, 3>, itk::Image<ResultPixelType, 3> >(img, min, max);
	case iAITKIO::ScalarType::LONG:
		return internalRescaleImageTo<itk::Image<long, 3>, itk::Image<ResultPixelType, 3> >(img, min, max);
	case iAITKIO::ScalarType::ULONG:
		return internalRescaleImageTo<itk::Image<unsigned long, 3>, itk::Image<ResultPixelType, 3> >(img, min, max);
	case iAITKIO::ScalarType::LONGLONG:
		return internalRescaleImageTo<itk::Image<long long, 3>, itk::Image<ResultPixelType, 3> >(img, min, max);
	case iAITKIO::ScalarType::ULONGLONG:
		return internalRescaleImageTo<itk::Image<unsigned long long, 3>, itk::Image<ResultPixelType, 3> >(img, min, max);
	case iAITKIO::ScalarType::FLOAT:
		return internalRescaleImageTo<itk::Image<float, 3>, itk::Image<ResultPixelType, 3> >(img, min, max);
	default:
		LOG(lvlError, "Invalid/Unknown itk pixel datatype in rescale!");
#if __cplusplus >= 201703L
		[[fallthrough]];
#endif
		// fall through
	case iAITKIO::ScalarType::DOUBLE:
		return internalRescaleImageTo<itk::Image<double, 3>, itk::Image<ResultPixelType, 3> >(img, min, max);
	}
}

iAbase_API void getStatistics(iAITKIO::ImagePointer img, double* min, double* max = nullptr, double* mean = nullptr, double* stddev = nullptr, double* variance = nullptr, double * sum = nullptr);
