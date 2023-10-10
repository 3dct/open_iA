// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAMetaFileIO.h"

#include "iAFileUtils.h"
#include "iAImageData.h"
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

std::shared_ptr<iADataSet> iAMetaFileIO::loadData(QString const& fileName, QVariantMap const& paramValues, iAProgress const& progress)
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

	return std::make_shared<iAImageData>(img);
}

void iAMetaFileIO::saveData(QString const& fileName, std::shared_ptr<iADataSet> dataSet, QVariantMap const& paramValues, iAProgress const& progress)
{
	iAConnector con;
	auto imgData = dynamic_cast<iAImageData*>(dataSet.get());
	if (!imgData)
	{
		throw std::runtime_error("Meta File IO expects image(/volume) data, but given dataset was of a different type!");
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
