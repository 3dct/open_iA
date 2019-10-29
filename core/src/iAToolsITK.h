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
#pragma once

#include "iAConsole.h"
#include "io/iAITKIO.h"
#include "open_iA_Core_export.h"

#include <itkCastImageFilter.h>
#include <itkChangeInformationImageFilter.h>
#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>
#include <itkImageRegionConstIterator.h>
#include <itkImageRegionIterator.h>
#include <itkRescaleIntensityImageFilter.h>

#include <QString>

// TODO: unify with iAITKIO !

open_iA_Core_API itk::ImageIOBase::IOComponentType itkScalarPixelType(iAITKIO::ImagePointer image);
open_iA_Core_API itk::ImageIOBase::IOPixelType itkPixelType( iAITKIO::ImagePointer image );
open_iA_Core_API iAITKIO::ImagePointer allocateImage(iAITKIO::ImagePointer img);
open_iA_Core_API iAITKIO::ImagePointer allocateImage(int const size[iAITKIO::m_DIM], double const spacing[iAITKIO::m_DIM], itk::ImageIOBase::IOComponentType type);
open_iA_Core_API void storeImage(iAITKIO::ImagePointer image, QString const & filename, bool useCompression);

//! @{
//! Generic access to pixels of any ITK image as double.
//! Slow! If you need to access more than a few pixels,
//! convert the whole image first (maybe using templates) and then access directly!
open_iA_Core_API double itkPixel(iAITKIO::ImagePointer img, iAITKIO::ImageBaseType::IndexType idx);
open_iA_Core_API void setITKPixel(iAITKIO::ImagePointer img, iAITKIO::ImageBaseType::IndexType idx, double value);
//! @}

//! extract part of an image as a new file
open_iA_Core_API iAITKIO::ImagePointer extractImage(iAITKIO::ImagePointer inImg, size_t const indexArr[iAITKIO::m_DIM], size_t const sizeArr[iAITKIO::m_DIM]);

//! set index offset of an image to (0,0,0)
//open_iA_Core_API iAITKIO::ImagePointer setIndexOffsetToZero(iAITKIO::ImagePointer inImg);
template <typename T>
typename itk::Image<T, iAITKIO::m_DIM>::Pointer setIndexOffsetToZero(typename itk::Image<T, iAITKIO::m_DIM>::Pointer inImg)
{
	// change output image index offset to zero
	typedef itk::Image<T, iAITKIO::m_DIM> ImageType;
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
void storeImage(TImage * image, QString const & filename, bool useCompression = true)
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
		DEBUG_LOG(QString("Error while writing image file '%1': %2")
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

template<typename ResultPixelType>
iAITKIO::ImagePointer castImageTo(iAITKIO::ImagePointer img)
{
	// can I retrieve number of dimensions somehow? otherwise assume 3 fixed?
	switch (itkScalarPixelType(img))
	{
		case itk::ImageIOBase::UCHAR:
			return internalCastImageTo<itk::Image<unsigned char, 3>, itk::Image<ResultPixelType, 3> >(img);
		case itk::ImageIOBase::CHAR:
			return internalCastImageTo<itk::Image<char, 3>, itk::Image<ResultPixelType, 3> >(img);
		case itk::ImageIOBase::SHORT:
			return internalCastImageTo<itk::Image<short, 3>, itk::Image<ResultPixelType, 3> >(img);
		case itk::ImageIOBase::USHORT:
			return internalCastImageTo<itk::Image<unsigned short, 3>, itk::Image<ResultPixelType, 3> >(img);
		case itk::ImageIOBase::INT:
			return internalCastImageTo<itk::Image<int, 3>, itk::Image<ResultPixelType, 3> >(img);
		case itk::ImageIOBase::UINT:
			return internalCastImageTo<itk::Image<unsigned int, 3>, itk::Image<ResultPixelType, 3> >(img);
		case itk::ImageIOBase::LONG:
			return internalCastImageTo<itk::Image<long, 3>, itk::Image<ResultPixelType, 3> >(img);
		case itk::ImageIOBase::ULONG:
			return internalCastImageTo<itk::Image<unsigned long, 3>, itk::Image<ResultPixelType, 3> >(img);
#if ITK_VERSION_MAJOR > 4 || (ITK_VERSION_MAJOR == 4 && ITK_VERSION_MINOR > 12)
		case itk::ImageIOBase::LONGLONG:
			return internalCastImageTo<itk::Image<long long, 3>, itk::Image<ResultPixelType, 3> >(img);
		case itk::ImageIOBase::ULONGLONG:
			return internalCastImageTo<itk::Image<unsigned long long, 3>, itk::Image<ResultPixelType, 3> >(img);
#endif
		case itk::ImageIOBase::FLOAT:
			return internalCastImageTo<itk::Image<float, 3>, itk::Image<ResultPixelType, 3> >(img);
		default:
			DEBUG_LOG("ERROR: Invalid/Unknown itk pixel datatype in rescale!");
		case itk::ImageIOBase::DOUBLE:
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
	switch (itkScalarPixelType(img))
	{
	case itk::ImageIOBase::UCHAR:
		return internalRescaleImageTo<itk::Image<unsigned char, 3>, itk::Image<ResultPixelType, 3> >(img, min, max);
	case itk::ImageIOBase::CHAR:
		return internalRescaleImageTo<itk::Image<char, 3>, itk::Image<ResultPixelType, 3> >(img, min, max);
	case itk::ImageIOBase::SHORT:
		return internalRescaleImageTo<itk::Image<short, 3>, itk::Image<ResultPixelType, 3> >(img, min, max);
	case itk::ImageIOBase::USHORT:
		return internalRescaleImageTo<itk::Image<unsigned short, 3>, itk::Image<ResultPixelType, 3> >(img, min, max);
	case itk::ImageIOBase::INT:
		return internalRescaleImageTo<itk::Image<int, 3>, itk::Image<ResultPixelType, 3> >(img, min, max);
	case itk::ImageIOBase::UINT:
		return internalRescaleImageTo<itk::Image<unsigned int, 3>, itk::Image<ResultPixelType, 3> >(img, min, max);
	case itk::ImageIOBase::LONG:
		return internalRescaleImageTo<itk::Image<long, 3>, itk::Image<ResultPixelType, 3> >(img, min, max);
	case itk::ImageIOBase::ULONG:
		return internalRescaleImageTo<itk::Image<unsigned long, 3>, itk::Image<ResultPixelType, 3> >(img, min, max);
#if ITK_VERSION_MAJOR > 4 || (ITK_VERSION_MAJOR == 4 && ITK_VERSION_MINOR > 12)
	case itk::ImageIOBase::LONGLONG:
		return internalRescaleImageTo<itk::Image<long long, 3>, itk::Image<ResultPixelType, 3> >(img, min, max);
	case itk::ImageIOBase::ULONGLONG:
		return internalRescaleImageTo<itk::Image<unsigned long long, 3>, itk::Image<ResultPixelType, 3> >(img, min, max);
#endif
	case itk::ImageIOBase::FLOAT:
		return internalRescaleImageTo<itk::Image<float, 3>, itk::Image<ResultPixelType, 3> >(img, min, max);
	default:
		DEBUG_LOG("ERROR: Invalid/Unknown itk pixel datatype in rescale!");
	case itk::ImageIOBase::DOUBLE:
		return internalRescaleImageTo<itk::Image<double, 3>, itk::Image<ResultPixelType, 3> >(img, min, max);
	}
}

open_iA_Core_API void getStatistics(iAITKIO::ImagePointer img, double* min, double* max = nullptr, double* mean = nullptr, double* stddev = nullptr, double* variance = nullptr, double * sum = nullptr);
