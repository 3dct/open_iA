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
#include "iAVTIFileIO.h"

#include "iAFileUtils.h"
#include "iAProgress.h"

#include <vtkImageData.h>
#include <vtkXMLImageDataReader.h>
#include <vtkXMLImageDataWriter.h>

iAVTIFileIO::iAVTIFileIO() : iAFileIO(iADataSetType::Volume, iADataSetType::Volume)
{}

std::shared_ptr<iADataSet> iAVTIFileIO::loadData(QString const& fileName, QVariantMap const& paramValues, iAProgress const& progress)
{
	Q_UNUSED(paramValues);
	vtkNew<vtkXMLImageDataReader> reader;
	progress.observe(reader);
	reader->SetFileName(getLocalEncodingFileName(fileName).c_str());
	reader->Update();
	auto img = reader->GetOutput();
	return { std::make_shared<iAImageData>(img) };
}

void  iAVTIFileIO::saveData(QString const& fileName, std::shared_ptr<iADataSet> dataSet, QVariantMap const& paramValues, iAProgress const& progress)
{
	Q_UNUSED(paramValues);
	vtkNew<vtkXMLImageDataWriter> writer;
	progress.observe(writer);
	writer->SetFileName(getLocalEncodingFileName(fileName).c_str());
	if (!dynamic_cast<iAImageData*>(dataSet.get()))
	{
		throw std::runtime_error("VTI volume export: Given dataset is not an image!");
	}
	auto img = dynamic_cast<iAImageData*>(dataSet.get())->vtkImage();
	writer->SetInputData(img);
	writer->Write();
}

QString iAVTIFileIO::name() const
{
	return "Serial XML VTK image data";
}

QStringList iAVTIFileIO::extensions() const
{
	return QStringList{ "vti" };
}
