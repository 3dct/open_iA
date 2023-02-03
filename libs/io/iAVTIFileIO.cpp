// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
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
