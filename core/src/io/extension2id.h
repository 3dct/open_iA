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

#include "defines.h"

#include <QMap>

typedef QMap<QString, iAIOType> mapQString2int;

static mapQString2int fill_extensionToId()
{
	mapQString2int m;

	m[""] = UNKNOWN_READER;
	m["MHD"] = MHD_READER;
	m["MHA"] = MHD_READER;
	m["STL"] = STL_READER;
	m["RAW"] = RAW_READER;
	m["VOL"] = RAW_READER;
	m["REC"] = RAW_READER;
	m["PRO"] = RAW_READER;
	m["PARS"] = PARS_READER;
	m["VGI"] = VGI_READER;
	m["DCM"] = DCM_READER;
//	m["NRRD"] = NRRD_READER; see iAIOProvider.cpp why this is commented out
//	m["NHDR"] = NRRD_READER;
	m["TIF"] = MHD_READER;
	m["TIFF"] = MHD_READER;
	m["JPG"] = MHD_READER;
	m["JPEG"] = MHD_READER;
	m["PNG"] = MHD_READER;
	m["BMP"] = MHD_READER;
	m["PNG"] = MHD_READER;
	m["NIA"] = MHD_READER;
	m["NII"] = MHD_READER;
	m["GZ"]  = MHD_READER;   // actually, only nii.gz and img.gz...
	m["HDR"] = MHD_READER;
	m["IMG"] = MHD_READER;
	m["OIF"] = OIF_READER;
	m["AM"] = AM_READER;
	m["VTI"] = VTI_READER;
	m["VTK"] = VTK_READER;
	m["MOD"] = PROJECT_READER;
#ifdef USE_HDF5
	m["HDF5"] = HDF5_READER;
	m["H5"] = HDF5_READER;
	m["HE5"] = HDF5_READER;
	m["NC"] = HDF5_READER;
	m["CDF"] = HDF5_READER;
	m["MAT"] = HDF5_READER;
#endif

	return m;
}
const mapQString2int extensionToId = fill_extensionToId();

static mapQString2int fill_extensionToIdStack()
{
	mapQString2int m;

	m[""] = UNKNOWN_READER;
	m["RAW"] = VOLUME_STACK_READER;
	m["MHD"] = VOLUME_STACK_MHD_READER;
	m["VOLSTACK"] = VOLUME_STACK_VOLSTACK_READER;
	m["TIF"] = TIF_STACK_READER;
	m["TIFF"] = TIF_STACK_READER;
	m["JPG"] = JPG_STACK_READER;
	m["JPEG"] = JPG_STACK_READER;
	m["PNG"] = PNG_STACK_READER;
	m["BMP"] = BMP_STACK_READER;

	return m;
}
const mapQString2int extensionToIdStack = fill_extensionToIdStack();

static mapQString2int fill_extensionToSaveId()
{
	mapQString2int m;
	m["TIF"] = TIF_STACK_WRITER;
	m["TIFF"] = TIF_STACK_WRITER;
	m["JPG"] = JPG_STACK_WRITER;
	m["JPEG"] = JPG_STACK_WRITER;
	m["PNG"] = PNG_STACK_WRITER;
	m["BMP"] = BMP_STACK_WRITER;
	m["DCM"] = DCM_WRITER;
	m["AM"] = AM_WRITER;
	m["CSV"] = CSV_WRITER;
	m["HDF5"] = MHD_WRITER;
	m["HE5"] = MHD_WRITER;
	m["H5"] = MHD_WRITER;
	return m;
}
const mapQString2int extensionToSaveId = fill_extensionToSaveId();
