// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAITKFileIO.h"

#include <iAImageData.h>
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
		"gipl", "gipl.gz",                                          // GIPL files
		"hdr", "hdr.gz", "img", "img.gz", "nia", "nii", "nii.gz",   // NIFTI files
		"lsm",                                                      // LSM files
		"mnc", "mnc.gz", "mnc2",                                    // MINC images
		"mrc", // "rec",                                            // MRC images ("rec" also listed in ITK, but we open that as a raw file, so remove it here)
		"nrrd", "nhdr",                                             // NRRD files
		"pic",                                                      // BioRad images
		"spr",                                                      // Stimulate images
		 //".j2k", ".jp2", ".jpt"     // JPEG 2000 -> image stack reader? but there we use VTK at the moment
	};
}
