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
#include "iAIOProvider.h"

#include "iARawFileParameters.h"
#include "iAValueTypeVectorHelpers.h"
#include "iAToolsVTK.h"

#include <QObject>

const QString iAIOProvider::ProjectFileExtension(".mod");
const QString iAIOProvider::ProjectFileTypeFilter("open_iA modality file (*"+ProjectFileExtension+");;");
const QString iAIOProvider::NewProjectFileExtension(".iaproj");
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
		"All supported types (*.mhd *.mha *.stl *.vgi *.raw *.rec *.vol *.pro *.pre *.pars *.dcm *.nia *.nii *.nii.gz *.hdr *.hdr.gz *.img *.img.gz *.oif *.am *.vtk *.nkc "
		"*.vti "+ImageFormatExtensions+" *"+ProjectFileExtension+" *"+NewProjectFileExtension+");;"

		+ MetaImages + VTKFiles +
		"STL files (*.stl);;"
		"VG Studio Scenes (*.vgi);;"
		"RAW files (*.raw *.rec *.vol *.pro *.pre);;"
		"PARS files (*.pars);;"
		"Dicom Series (*.dcm);;"
//		"NRRD files (*.nrrd *.nhdr);;"	// currently not supported as it reads as a itk::VectorImage, which we cannot convert to vtkImageData at the moment
		"NIFTI Images (*.nia *.nii *.nii.gz *.hdr *.hdr.gz *.img *.img.gz);;"
		"Olympus FluoView (*.oif);;"
		"AmiraMesh (*.am);;"
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

QVariantMap rawParamsToMap(iARawFileParameters const& p)
{
	QVariantMap result;
	result["Size"] = variantVector<int>({ static_cast<int>(p.m_size[0]), static_cast<int>(p.m_size[1]), static_cast<int>(p.m_size[2]) });
	result["Spacing"] = variantVector<double>({ p.m_spacing[0], p.m_spacing[1], p.m_spacing[2] });
	result["Origin"] = variantVector<double>({ p.m_origin[0], p.m_origin[1], p.m_origin[2] });
	result["Headersize"] = p.m_headersize;
	result["DataType"] = mapVTKTypeToReadableDataType(p.m_scalarType);
	result["ByteOrder"] = ByteOrder::mapVTKTypeToString(p.m_byteOrder);
	return result;
}

iARawFileParameters rawParamsFromMap(QVariantMap const& map)
{
	iARawFileParameters result;
	setFromVectorVariant<int>(result.m_size, map["Size"]);
	setFromVectorVariant<double>(result.m_spacing, map["Spacing"]);
	setFromVectorVariant<double>(result.m_origin, map["Origin"]);
	result.m_headersize = map["Headersize"].toULongLong();
	result.m_scalarType = mapReadableDataTypeToVTKType(map["Data Type"].toString());
	result.m_byteOrder = ByteOrder::mapStringToVTKType(map["Byte Order"].toString());
	return result;
}
