// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAVTKFileIO.h"

#include "iAExceptionThrowingErrorObserver.h"
#include "iAFileUtils.h"   // for getLocalEncodingFileName
#include "iAProgress.h"
#include "iAToolsVTK.h"

#include <vtkGenericDataObjectReader.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkRectilinearGrid.h>

#include <cstring>   // for memcpy -> ToDo: get rid of memcpy, use VTK dataset conversions instead!

const QString iAVTKFileIO::Name("VTK files");

iAVTKFileIO::iAVTKFileIO() : iAFileIO(iADataSetType::All, iADataSetType::None)
{}

std::shared_ptr<iADataSet> iAVTKFileIO::loadData(QString const& fileName, QVariantMap const& paramValues, iAProgress const& progress)
{
	Q_UNUSED(paramValues);
	auto reader = vtkSmartPointer<vtkGenericDataObjectReader>::New();
	reader->SetFileName(getLocalEncodingFileName(fileName).c_str());
	progress.observe(reader);
	reader->AddObserver(vtkCommand::ErrorEvent, iAExceptionThrowingErrorObserver::New());
	reader->Update();

	if (reader->IsFilePolyData())
	{
		LOG(lvlInfo, "File contains polydata");
		vtkSmartPointer<vtkPolyData> polyData = reader->GetPolyDataOutput();
		return { std::make_shared<iAPolyData>(polyData) };
	}
	else if (reader->IsFileRectilinearGrid())
	{
		LOG(lvlInfo, "File contains a reclinear grid");
		// TODO: improve - probably using sampling, e.g. vtkProbeFilter
		auto rectilinearGrid = reader->GetRectilinearGridOutput();
		int* extent = rectilinearGrid->GetExtent();
		vtkDataArray* coords[3] = {
			rectilinearGrid->GetXCoordinates(),
			rectilinearGrid->GetYCoordinates(),
			rectilinearGrid->GetZCoordinates()
		};

		const int NumDimensions = 3;
		double spacing[NumDimensions];
		// determine spacing and make sure it is the same over all coordinates:
		for (int i = 0; i < NumDimensions; ++i)
		{
			int numComp = coords[i]->GetNumberOfComponents();
			int numValues = coords[i]->GetNumberOfValues();
			int extentSize = extent[i * 2 + 1] - extent[i * 2] + 1;
			if (numComp != 1 || numValues != extentSize)
			{
				LOG(lvlWarn,
					QString("Don't know how to handle situation where number of components is %1 "
						"and number of values=%2 not equal to extentSize=%3")
					.arg(numComp)
					.arg(numValues)
					.arg(extentSize));
				return {};
			}
			if (numValues < 2)
			{
				LOG(lvlWarn, QString("Dimension %1 has dimensions of less than 2, cannot compute proper spacing, using 1 instead!"));
				spacing[i] = 1;
			}
			else
			{
				spacing[i] = coords[i]->GetComponent(1, 0) - coords[i]->GetComponent(0, 0);
				for (int j = 2; j < numValues; ++j)
				{
					double actSpacing = coords[i]->GetComponent(j, 0) - coords[i]->GetComponent(j - 1, 0);
					if (actSpacing != spacing[i])
					{
						LOG(lvlWarn, QString("Spacing for cordinate %1 not the same as between 0..1 (%2) at index %3 (%4).")
							.arg(i)
							.arg(spacing[i])
							.arg(j)
							.arg(actSpacing));
						return {};
					}
				}
			}
		}

		auto numOfArrays = rectilinearGrid->GetPointData()->GetNumberOfArrays();

		//		for (
		int i = 0; // i < numOfArrays; ++i)
		if (numOfArrays > 0) //< remove this if enabling for loop
		{
			auto arrayData = rectilinearGrid->GetPointData()->GetAbstractArray(rectilinearGrid->GetPointData()->GetArrayName(i));
			int dataType = arrayData->GetDataType();

			int size[3] = {
				extent[1] - extent[0],
				extent[3] - extent[2],
				extent[5] - extent[4],
			};
			auto img = allocateImage(dataType, size, spacing);
			//arrayData->
			// memcpy scalar pointer into img->GetScalarPointer ?
			size_t byteSize = mapVTKTypeToSize(dataType) * size[0] * size[1] * size[2];

			auto arrayPtr = arrayData->GetVoidPointer(0);
			std::memcpy(img->GetScalarPointer(), arrayPtr, byteSize);

			return std::make_shared<iAImageData>(img);
		}
		return {};
	}
	else
	{
		LOG(lvlWarn, "This type of dataset within a vtk format is currently not supported");
		return {};
	}
}

QString iAVTKFileIO::name() const
{
	return Name;
}

QStringList iAVTKFileIO::extensions() const
{
	return QStringList{ "vtk" };
}
