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
#include "iAMetaFileIO.h"

#include "iADataSet.h"
#include "iAFileUtils.h"
#include "iAProgress.h"

#include "iAConnector.h"  // for writing

#include <vtkImageData.h>

#define VTK 0
#define ITK 1

#define META_LOAD_METHOD ITK
#if META_LOAD_METHOD == VTK
#include <vtkMetaImageReader.h>
#else
#include "iAToolsVTK.h"
#endif

iAMetaFileIO::iAMetaFileIO() :
	iAFileIO(iADataSetType::Volume, iADataSetType::Volume)
{
	addAttr(m_params[Save], CompressionStr, iAValueType::Boolean, false);
}

std::vector<std::shared_ptr<iADataSet>> iAMetaFileIO::loadData(QString const& fileName, QVariantMap const& paramValues, iAProgress const& progress)
{
	Q_UNUSED(paramValues);

#if META_LOAD_METHOD == VTK
	vtkNew<vtkMetaImageReader> reader;
	p->observe(reader);
	//reader->SetFileName(m_fileName.toStdString().c_str());
	imgReader->AddObserver(vtkCommand::ErrorEvent, iAExceptionThrowingErrorObserver::New());
	reader->SetFileName(getLocalEncodingFileName(m_fileName).c_str());
	reader->Update();
	if (reader->GetErrorCode() != 0)   // doesn't seem to catch errors such as file not existing...?
	{
		LOG(lvlWarn, QString("While reading file %1, an error (code %2) occurred!").arg(m_fileName).arg(reader->GetErrorCode()));
	}
	reader->ReleaseDataFlagOn();
	auto img = reader->GetOutput();
	// duration: 362,362,368,368,383 ms
#else
	Q_UNUSED(progress);
	auto img = vtkSmartPointer<vtkImageData>::New();
	readImage(fileName, true, img);    //< using iAToolsVTK
	// duration: ~400ms
#endif

	// new tests with large dataset (D:\TestDatasets\large\CFK-Probe_Stahlstift_10xavg_freebeam_448proj.raw):
	//
	// VTK: 50571(ignore, caching...), 4484, 4477, 4529 ms
	// ITK (using readImage from iAToolsVTK): 5876 (ignore), 5877, 5760, 5746 ms
	// -> VTK consistently faster; but doesn't produce an error if it doesn't find file for example (just returns a 1x1x1 image)!

	return { std::make_shared<iAImageData>(img) };
}

void iAMetaFileIO::saveData(QString const& fileName, std::vector<std::shared_ptr<iADataSet>>& dataSets, QVariantMap const& paramValues, iAProgress const& progress)
{
	iAConnector con;
	if (dataSets.size() != 1)
	{
		LOG(lvlError, QString("Meta File IO only supports writing exactly 1 dataset, %1 given!").arg(dataSets.size()));
		return;
	}
	auto imgData = dynamic_cast<iAImageData*>(dataSets[0].get());
	if (!imgData)
	{
		LOG(lvlError, "Meta File IO expects image(/volume) data, but given dataset was of a different type!");
		return;
	}
	storeImage(imgData->vtkImage(), fileName, paramValues[CompressionStr].toBool(), &progress);
}

QString iAMetaFileIO::name() const
{
	return "Meta Image";
}

QStringList iAMetaFileIO::extensions() const
{
	return QStringList{ "mhd", "mha" };
}
