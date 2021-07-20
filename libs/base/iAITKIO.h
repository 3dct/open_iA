/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#include <itkImageBase.h>
#include <itkImageIOBase.h>

class QString;

namespace iAITKIO
{
	// type definitions - unify with iAITKIO definitions, and with defines.h DIM!
	static const int m_DIM = 3;
	typedef itk::ImageBase< m_DIM > ImageBaseType;
	typedef ImageBaseType::Pointer ImagePointer;
	typedef ImageBaseType* ImagePtr;
	typedef itk::ImageIOBase::IOComponentType ScalarPixelType;


	// TODO: unify with mdichild::loadfile / iAIO!
	iAbase_API ImagePointer readFile(QString const& fileName, ScalarPixelType& pixelType, bool releaseFlag);
	iAbase_API void writeFile(QString const& fileName, ImagePtr image, ScalarPixelType pixelType, bool useCompression = false);
} // namespace iAITKIO
