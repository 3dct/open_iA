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
#include "iADefaultFileIOs.h"

#include "iAFileTypeRegistry.h"

#include "iAAmiraVolumeFileIO.h"
#include "iACSVImageFileIO.h"
#include "iADCMFileIO.h"
#include "iAGraphFileIO.h"
#include "iAHDF5IO.h"
#include "iAImageStackFileIO.h"
#include "iAMetaFileIO.h"
#include "iANKCFileIO.h"
#include "iAProjectFileIO.h"
#include "iAOIFFileIO.h"
#include "iARawFileIO.h"
#include "iASTLFileIO.h"
#include "iAVGIFileIO.h"
#include "iAVTIFileIO.h"
#include "iAVTKFileIO.h"

void setupDefaultFileIOs()
{
	// volume file formats:
	iAFileTypeRegistry::addFileType<iAAmiraVolumeFileIO>();
	iAFileTypeRegistry::addFileType<iACSVImageFileIO>();
	iAFileTypeRegistry::addFileType<iADCMFileIO>();
	iAFileTypeRegistry::addFileType<iAImageStackFileIO>();
	iAFileTypeRegistry::addFileType<iAMetaFileIO>();
	iAFileTypeRegistry::addFileType<iANKCFileIO>();
	iAFileTypeRegistry::addFileType<iAOIFFileIO>();
	iAFileTypeRegistry::addFileType<iAVGIFileIO>();
	iAFileTypeRegistry::addFileType<iAVTIFileIO>();
	iAFileTypeRegistry::addFileType<iARawFileIO>();
#ifdef USE_HDF5
	iAFileTypeRegistry::addFileType<iAHDF5IO>();
#endif

	// mesh file formats:
	iAFileTypeRegistry::addFileType<iASTLFileIO>();

	// graph file formats:
	iAFileTypeRegistry::addFileType<iAGraphFileIO>();

	// file formats which can contain different types of data:
	iAFileTypeRegistry::addFileType<iAVTKFileIO>();
	
	// collection file formats:
	iAFileTypeRegistry::addFileType<iAProjectFileIO>();
}
