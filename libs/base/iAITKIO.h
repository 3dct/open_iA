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

#include "iAbase_export.h"

#include "iAItkVersion.h"

#include <itkImageBase.h>
#include <itkImageIOBase.h>

class iAProgress;

class QString;

namespace iAITKIO
{
	static const int Dim = 3;
	using ImageBaseType = itk::ImageBase<Dim>;
	using ImagePointer = ImageBaseType::Pointer;
	using ImagePtr = ImageBaseType*;
#if ITK_VERSION_NUMBER >= ITK_VERSION_CHECK(5, 1, 0)
	using PixelType = itk::CommonEnums::IOPixel;
	using ScalarType = itk::CommonEnums::IOComponent;
#else
	using PixelType = itk::ImageIOBase::IOPixelType;
	using ScalarType = itk::ImageIOBase::IOComponentType;
#endif

	// TODO:
	//     - check usage - replace with iAFileTypeRegistry::createIO where it makes sense to support broader range of file types
	//     - check iAToolsITK
	iAbase_API ImagePointer readFile(QString const& fileName, PixelType& pixelType, ScalarType& scalarType, bool releaseFlag, iAProgress const * progress = nullptr);
	iAbase_API void writeFile(QString const& fileName, ImagePtr image, ScalarType scalarType, bool useCompression = false, iAProgress const * progress = nullptr);
} // namespace iAITKIO
