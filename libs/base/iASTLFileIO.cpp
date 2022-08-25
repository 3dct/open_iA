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

iASTLFileIO::iASTLFileIO() : iAFileIO(iADataSetType::Mesh, iADataSetType::None)
{}

std::vector<std::shared_ptr<iADataSet>> iASTLFileIO::load(QString const& fileName, iAProgress* progress, QVariantMap const& params)
{
	Q_UNUSED(params);
	auto stlReader = vtkSmartPointer<vtkSTLReader>::New();
	progress->observe(stlReader);
	stlReader->AddObserver(vtkCommand::ErrorEvent, iAExceptionThrowingErrorObserver::New());
	stlReader->SetFileName(getLocalEncodingFileName(fileName).c_str());
	vtkNew<vtkPolyData> polyData;
	stlReader->SetOutput(polyData);
	stlReader->Update();
	return { std::make_shared<iAPolyData>(fileName, polyData) };
}

QString iASTLFileIO::name() const
{
	return "STL file";
}

QStringList iASTLFileIO::extensions() const
{
	return QStringList{ "stl" };
}
