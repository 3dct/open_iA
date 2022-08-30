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
#include "iADCMFileIO.h"

#include "defines.h"       // for DIM
#include "iAConnector.h"
#include "iAFileUtils.h"   // for getLocalEncodingFileName
#include "iAProgress.h"
//#include "iAToolsVTK.h"

#include <itkImage.h>
#include <itkImageSeriesReader.h>
#include <itkGDCMImageIO.h>
#include <itkGDCMSeriesFileNames.h>

#include <vtkImageData.h>

#include <QFileInfo>


const QString iADCMFileIO::Name("DICOM files");

iADCMFileIO::iADCMFileIO() : iAFileIO(iADataSetType::Volume, iADataSetType::None)
{
}

std::vector<std::shared_ptr<iADataSet>> iADCMFileIO::load(QString const& fileName, QVariantMap const& parameters, iAProgress* progress)
{
	Q_UNUSED(parameters);
	using PixelType = signed short; // check why signed short
	using ImageType =  itk::Image<PixelType, DIM>;
	auto reader = itk::ImageSeriesReader<ImageType>::New();
	auto dicomIO = itk::GDCMImageIO::New();
	reader->SetImageIO(dicomIO);
	progress->observe(reader);
	auto nameGenerator = itk::GDCMSeriesFileNames::New();
	nameGenerator->SetUseSeriesDetails(true);
	nameGenerator->SetDirectory(getLocalEncodingFileName(QFileInfo(fileName).canonicalPath()));
	auto const & seriesUID = nameGenerator->GetSeriesUIDs();
	std::string seriesIdentifier = seriesUID.begin()->c_str();
	auto fileNames = nameGenerator->GetFileNames(seriesIdentifier);
	reader->SetFileNames(fileNames);
	reader->Update();
	reader->Modified();
	reader->Update();

	iAConnector con;
	con.setImage(reader->GetOutput());
	auto img = vtkSmartPointer<vtkImageData>::New();
	img->DeepCopy(con.vtkImage());
	return { std::make_shared<iAImageData>(fileName, img) };
}

QString iADCMFileIO::name() const
{
	return Name;
}

QStringList iADCMFileIO::extensions() const
{
	return QStringList{ "dcm" };
}