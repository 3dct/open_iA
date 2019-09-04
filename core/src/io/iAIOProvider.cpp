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
#include "iAIOProvider.h"

#include <QObject>

const QString iAIOProvider::ProjectFileExtension(".mod");
const QString iAIOProvider::ProjectFileTypeFilter("open_iA modality file (*"+ProjectFileExtension+");;");
const QString iAIOProvider::NewProjectFileExtension(".opf");
const QString iAIOProvider::NewProjectFileTypeFilter("open_iA project file (*"+NewProjectFileExtension+");;");
const QString iAIOProvider::MetaImages("Meta Images (*.mhd *.mha);;");
const QString iAIOProvider::VTKFiles("VTK Files (*.vtk);;");

namespace
{
	const QString ImageFormatExtensions("*.bmp *.jpg *.jpeg *.png *.tif *.tiff");
}

QString iAIOProvider::GetSupportedLoadFormats()
{
	return QString(
		"All supported types (*.mhd *.mha *.stl *.vgi *.raw *.rec *.vol *.pro *.pars *.dcm *.oif *.am *.vtk "
#ifdef USE_HDF5
		"*.hdf5 *.h5 *.he5 *.mat "
#endif
		"*.vti "+ImageFormatExtensions+" *"+ProjectFileExtension+");;"
		+ MetaImages + VTKFiles +
		"STL files (*.stl);;"
		"VG Studio Scenes (*.vgi);;"
		"RAW files (*.raw *.rec *.vol *.pro);;"
		"PARS files (*.pars);;"
		"Dicom Series (*.dcm);;"
//		"NRRD files (*.nrrd *.nhdr);;"	// currently not supported as it reads as a itk::VectorImage, which we cannot convert to vtkImageData at the moment
		"Olympus FluoView (*.oif);;"
		"AmiraMesh (*.am);;"
#ifdef USE_HDF5
		"Hierarchical Data Format v5 (*.hdf5 *.h5 *.he5);;"
		"Matlab data files v7.3 (*.mat);;"
		"Network Common Data Format v4 (*.nc *.cdf);;"
#endif
		"Serial VTK image data (*.vti);;") +
		GetSupportedImageFormats() +
		ProjectFileTypeFilter +
		NewProjectFileTypeFilter;
}


QString iAIOProvider::GetSupportedSaveFormats()
{
	return
		MetaImages +
		"STL files (*.stl);;"
		"AmiraMesh (*.am);;"
		"ITK HDF5 (*.hdf5);;"
		"Comma-Separated Values (*.csv)";
}

QString iAIOProvider::GetSupportedImageStackFormats()
{
	return QString("All supported types (*.mhd *.mha " + ImageFormatExtensions + ");;"
		) + MetaImages +
		GetSupportedImageFormats();
}

QString iAIOProvider::GetSupportedVolumeStackFormats()
{
	return
		"All supported types (*.mhd *.raw *.volstack);;"
		+ MetaImages +
		"RAW files (*.raw);;"
		"Volume Stack (*.volstack);;";
}

QString iAIOProvider::GetSupportedImageFormats()
{
	return QObject::tr(
		"BMP (*.bmp);;"
		"JPEG (*.jpg *.jpeg);;"
		"PNG (*.png);;"
		"TIFF (*.tif *.tiff);;"
	);
}
