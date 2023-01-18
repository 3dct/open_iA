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
#include "iAITKFileIO.h"

#include <iADataSet.h>
#include <iAFileUtils.h>   // for getLocalEncodingFileName
#include <iAToolsITK.h>
#include <iAProgress.h>
#include <iATypedCallHelper.h>



const QString iAITKFileIO::Name("Files readable by ITK");

iAITKFileIO::iAITKFileIO() : iAFileIO(iADataSetType::Volume, iADataSetType::Volume)
{}

std::shared_ptr<iADataSet> iAITKFileIO::loadData(QString const& fileName, QVariantMap const& paramValues, iAProgress const& progress)
{
	Q_UNUSED(paramValues);
	iAITKIO::PixelType pixelType;
	iAITKIO::ScalarType scalarType;
	return std::make_shared<iAImageData>(iAITKIO::readFile(fileName, pixelType, scalarType, false, &progress));
}

void iAITKFileIO::saveData(QString const& fileName, std::shared_ptr<iADataSet> dataSet,
	QVariantMap const& paramValues, iAProgress const& progress)
{
	auto imgData = dynamic_cast<iAImageData*>(dataSet.get());
	storeImage(imgData->itkImage(), fileName, paramValues[iAFileIO::CompressionStr].toBool(), &progress);
}

QString iAITKFileIO::name() const
{
	return Name;
}

QStringList iAITKFileIO::extensions() const
{
	// see https://itk.org/Wiki/ITK/FAQ#What_3D_file_formats_can_ITK_import_and_export?
	// and https://itk.org/Wiki/ITK/File_Formats
	// as well as a search for AddSupportedReadExtension in ITK code base
	return QStringList{
		"gipl", "gipl.gz"                                           // GIPL files
		"hdr", "hdr.gz", "img", "img.gz", "nia", "nii", "nii.gz",   // NIFTI files
		"lsm",                                                      // LSM files
		"mnc", "mnc.gz", "mnc2",                                    // MINC images
		"mrc", // "rec",                                            // MRC images ("rec" also listed in ITK, but we open that as a raw file, so remove it here)
		"nrrd", "nhdr",                                             // NRRD files
		"pic",                                                      // BioRad images
		"spr"                                                       // Stimulate images
		 //".j2k", ".jp2", ".jpt"     // JPEG 2000 -> image stack reader? but there we use VTK at the moment
	};
}
