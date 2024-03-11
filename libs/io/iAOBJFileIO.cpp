// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAOBJFileIO.h"

#include "iAExceptionThrowingErrorObserver.h"
#include "iAFileUtils.h"
#include "iAPolyData.h"
#include "iAProgress.h"

#include <vtkOBJReader.h>
#include <vtkOBJWriter.h>

iAOBJFileIO::iAOBJFileIO() : iAFileIO(iADataSetType::Mesh, iADataSetType::Mesh)
{
}

std::shared_ptr<iADataSet> iAOBJFileIO::loadData(QString const& fileName, QVariantMap const& params, iAProgress const& progress)
{
	Q_UNUSED(params);
	vtkNew<vtkOBJReader> reader;
	progress.observe(reader);
	reader->AddObserver(vtkCommand::ErrorEvent, iAExceptionThrowingErrorObserver::New());
	reader->SetFileName(getLocalEncodingFileName(fileName).c_str());
	vtkNew<vtkPolyData> polyData;
	reader->SetOutput(polyData);
	reader->Update();
	return { std::make_shared<iAPolyData>(polyData) };
}

void iAOBJFileIO::saveData(QString const& fileName, std::shared_ptr<iADataSet> dataSet, QVariantMap const& paramValues, iAProgress const& progress)
{
	assert(dataSet->type() == iADataSetType::Mesh);
	Q_UNUSED(paramValues);
	vtkNew<vtkOBJWriter> writer;
	progress.observe(writer);
	writer->AddObserver(vtkCommand::ErrorEvent, iAExceptionThrowingErrorObserver::New());
	writer->SetFileName(getLocalEncodingFileName(fileName).c_str());
	writer->SetInputData(dynamic_cast<iAPolyData*>(dataSet.get())->poly());
	writer->Write();
}

QString iAOBJFileIO::name() const
{
	return "WaveFront OBJ file";
}

QStringList iAOBJFileIO::extensions() const
{
	return QStringList{ "obj" };
}
