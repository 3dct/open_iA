/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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

#include <QColor>
#include <QString>

#include "open_iA_Core_export.h"

#define DIM 3

const QString organisationName = "FHW";
const QString applicationName = "open_iA";

enum IOType
{
	UNKNOWN_READER,
	MHD_READER,
	STL_READER,
	RAW_READER,
	PRO_READER,
	PARS_READER,
	VGI_READER,
	TIF_STACK_READER,
	JPG_STACK_READER,
	PNG_STACK_READER,
	BMP_STACK_READER,
	MHD_WRITER,
	STL_WRITER,
	TIF_STACK_WRITER,
	JPG_STACK_WRITER,
	PNG_STACK_WRITER,
	BMP_STACK_WRITER,
	MPEG_WRITER,
	OGV_WRITER,
	AVI_WRITER,
	CSV_READER,
	XML_READER,
	XML_WRITER,
	VOLUME_STACK_READER,
	VOLUME_STACK_MHD_READER,
	VOLUME_STACK_VOLSTACK_READER,
	VOLUME_STACK_VOLSTACK_WRITER,
	DCM_READER,
	DCM_WRITER, 
	NRRD_READER,
	OIF_READER,
	AM_READER,
	AM_WRITER,
	VTI_READER,
	CSV_WRITER
};

const int DefaultMagicLensSize = 120;
const int MinimumMagicLensSize = 20;
const int MaximumMagicLensSize = 8192;
const int DefaultHistogramBins = 2048;

// define preset colors
open_iA_Core_API QColor * PredefinedColors();
