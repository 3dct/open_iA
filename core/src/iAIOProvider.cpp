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
 
#include "iAIOProvider.h"

#include <QObject>

const QString iAIOProvider::ProjectFileExtension(".mod");
const QString iAIOProvider::ProjectFileTypeFilter("open_iA project file (*"+ProjectFileExtension+");;All files (*.*)");
const QString iAIOProvider::MetaImages("Meta Images (*.mhd *.mha);;");
namespace
{
	const QString ImageFormatExtensions("*.bmp *.jpg *.jpeg *.png *.tif *.tiff");
}

QString iAIOProvider::GetSupportedLoadFormats()
{
	return QString(
		"All supported types (*.mhd *.mha *.stl *.vgi *.raw *.rec *.vol *.pro *.pars *.dcm *.nrrd *.oif *.am *.hdf5 *.h5 *.he5 *.mat *.vti "+ImageFormatExtensions+");;"
		+ MetaImages +
		"STL files (*.stl);;"
		"VG Studio Scenes (*.vgi);;"
		"RAW files (*.raw *.rec *.vol *.pro);;"
		"PARS files (*.pars);;"
		"Dicom Series (*.dcm);;"
		"NRRD files (*.nrrd);;"
		"Olympus FluoView (*.oif);;"
		"AmiraMesh (*.am);;"
		"Hierarchical Data Format v5 (*.hdf5 *.h5 *.he5);;"
		"Network Common Data Format v4 (*.nc *.cdf);;"
		"Matlab data files v7.3 (*.mat);;"
		"Serial VTK image data (*.vti);;") +
		GetSupportedImageFormats();
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