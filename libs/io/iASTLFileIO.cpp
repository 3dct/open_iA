// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iASTLFileIO.h"

#include <iAExceptionThrowingErrorObserver.h>
#include <iAPolyData.h>
#include <iAProgress.h>

#include <vtkSTLReader.h>
#include <vtkSTLWriter.h>

namespace
{
	QString const FormatParam = "Format";
	QString const FmtBinary = "Binary";
	QString const FmtASCII = "ASCII";
}

iASTLFileIO::iASTLFileIO() : iAFileIO(iADataSetType::Mesh, iADataSetType::Mesh)
{
	QStringList formatOptions = QStringList() << FmtBinary << FmtASCII;
	addAttr(m_params[Save], FormatParam, iAValueType::Categorical, formatOptions);
}

std::shared_ptr<iADataSet> iASTLFileIO::loadData(QString const& fileName, QVariantMap const& params, iAProgress const& progress)
{
	Q_UNUSED(params);
	vtkNew<vtkSTLReader> reader;
	progress.observe(reader);
	reader->AddObserver(vtkCommand::ErrorEvent, iAExceptionThrowingErrorObserver::New());
	reader->SetFileName(fileName.toStdString().c_str());
	vtkNew<vtkPolyData> polyData;
	reader->SetOutput(polyData);
	reader->Update();
	return { std::make_shared<iAPolyData>(polyData) };
}

void iASTLFileIO::saveData(QString const& fileName, std::shared_ptr<iADataSet> dataSet, QVariantMap const& paramValues, iAProgress const& progress)
{
	Q_UNUSED(paramValues);
	vtkNew<vtkSTLWriter> writer;
	progress.observe(writer);
	writer->AddObserver(vtkCommand::ErrorEvent, iAExceptionThrowingErrorObserver::New());
	writer->SetFileType(paramValues[FormatParam].toString() == FmtBinary ? VTK_BINARY : VTK_ASCII);
	writer->SetFileName(fileName.toStdString().c_str());
	writer->SetInputData(dynamic_cast<iAPolyData*>(dataSet.get())->poly());
	writer->Write();
}

QString iASTLFileIO::name() const
{
	return "STL file";
}

QStringList iASTLFileIO::extensions() const
{
	return QStringList{ "stl" };
}
