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
#include "iAVTKFileIO.h"

#include "defines.h"       // for DIM
#include "iAExceptionThrowingErrorObserver.h"
#include "iAFileUtils.h"   // for getLocalEncodingFileName
#include "iAProgress.h"
#include "iAToolsVTK.h"

#include <vtkGenericDataObjectReader.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkRectilinearGrid.h>

const QString iAVTKFileIO::Name("VTK files");

iAVTKFileIO::iAVTKFileIO() : iAFileIO(iADataSetType::All, iADataSetType::None)
{
}

std::vector<std::shared_ptr<iADataSet>> iAVTKFileIO::load(QString const& fileName, iAProgress* progress, QVariantMap const& parameters)
{
	Q_UNUSED(parameters);
	std::vector<std::shared_ptr<iADataSet>> result;
	auto reader = vtkSmartPointer<vtkGenericDataObjectReader>::New();
	reader->SetFileName(getLocalEncodingFileName(fileName).c_str());
	progress->observe(reader);
	reader->AddObserver(vtkCommand::ErrorEvent, iAExceptionThrowingErrorObserver::New());
	reader->Update();

	if (reader->IsFilePolyData())
	{
		LOG(lvlInfo, "File contains polydata");
		vtkSmartPointer<vtkPolyData> polyData = reader->GetPolyDataOutput();
		return { std::make_shared<iAPolyData>(fileName, polyData) };
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

			return { std::make_shared<iAImageData>(fileName, img) };
		}
		return {};
	}
	else
	{
		LOG(lvlWarn, "This type of vtk format is currently not supported");
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