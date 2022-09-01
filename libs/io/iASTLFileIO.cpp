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
#include "iASTLFileIO.h"

#include "iAExceptionThrowingErrorObserver.h"
#include "iAFileUtils.h"
#include "iAProgress.h"

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

std::vector<std::shared_ptr<iADataSet>> iASTLFileIO::loadData(QString const& fileName, QVariantMap const& params, iAProgress const& progress)
{
	Q_UNUSED(params);
	vtkNew<vtkSTLReader> reader;
	progress.observe(reader);
	reader->AddObserver(vtkCommand::ErrorEvent, iAExceptionThrowingErrorObserver::New());
	reader->SetFileName(getLocalEncodingFileName(fileName).c_str());
	vtkNew<vtkPolyData> polyData;
	reader->SetOutput(polyData);
	reader->Update();
	return { std::make_shared<iAPolyData>(fileName, polyData) };
}

void iASTLFileIO::save(QString const& fileName, std::vector<std::shared_ptr<iADataSet>> const& dataSets, QVariantMap const& paramValues, iAProgress const& progress)
{
	assert(dataSets.size() == 1 && dataSets[0]->type() == iADataSetType::Mesh);
	Q_UNUSED(paramValues);
	vtkNew<vtkSTLWriter> writer;
	progress.observe(writer);
	writer->AddObserver(vtkCommand::ErrorEvent, iAExceptionThrowingErrorObserver::New());
	writer->SetFileType(paramValues[FormatParam].toString() == FmtBinary ? VTK_BINARY : VTK_ASCII);
	writer->SetFileName(getLocalEncodingFileName(fileName).c_str());
	writer->SetInputData(dynamic_cast<iAPolyData*>(dataSets[0].get())->poly());
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
