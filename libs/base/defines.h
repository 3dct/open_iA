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

#include <QString>

#include "iAbase_export.h"

#define DIM 3

iAbase_API extern const QString organisationName;
iAbase_API extern const QString applicationName;

enum iAIOType
{
	UNKNOWN_READER,
	MHD_READER,
	STL_READER,
	RAW_READER,
	PARS_READER,
	VGI_READER,
	TIF_STACK_READER,
	JPG_STACK_READER,
	PNG_STACK_READER,
	BMP_STACK_READER,
	CSV_READER,
	XML_READER,
	VOLUME_STACK_READER,
	VOLUME_STACK_MHD_READER,
	VOLUME_STACK_VOLSTACK_READER,
	VTK_READER,  //new for VTK Input
	DCM_READER,
//	NRRD_READER,     // see iAIOProvider.cpp why this is commented out
	OIF_READER,
	AM_READER,
	VTI_READER,
#ifdef USE_HDF5
	HDF5_READER,
#endif
	PROJECT_READER,
	NKC_READER,

	MHD_WRITER,
	STL_WRITER,
	TIF_STACK_WRITER,
	JPG_STACK_WRITER,
	PNG_STACK_WRITER,
	BMP_STACK_WRITER,
	MPEG_WRITER,
	OGV_WRITER,
	AVI_WRITER,
	XML_WRITER,
	VOLUME_STACK_VOLSTACK_WRITER,
	DCM_WRITER,
	AM_WRITER,
	CSV_WRITER,
	HDF5_WRITER
};

const int DefaultMagicLensSize = 120;
const int MinimumMagicLensSize = 40;
const int MaximumMagicLensSize = 8192;

const uint NotExistingChannel = std::numeric_limits<uint>::max();
