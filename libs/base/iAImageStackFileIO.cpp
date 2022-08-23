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
#include "iAImageStackFileIO.h"

#include "defines.h"         // for DIM
#include "iAExceptionThrowingErrorObserver.h"
#include "iAFileUtils.h"     // for getLocalEncodingFileName
#include "iAProgress.h"
#include "iAStringHelper.h"  // for greatestCommonPrefix
#include "iAToolsVTK.h"      // for mapVTKTypeToReadableDataType, readableDataTypes, ...
#include "iAValueTypeVectorHelpers.h"

#include <QDir>
#include <QFileInfo>


//#define VTK 0
//#define ITK 1
//#define STACK_LOAD_METHOD VTK

//#if STACK_LOAD_METHOD == ITK
//#else // VTK

	#include <vtkCommand.h>
	#include <vtkImageData.h>
	#include <vtkImageReader2.h>

	#include <vtkBMPReader.h>
	#include <vtkJPEGReader.h>
	#include <vtkPNGReader.h>
	#include <vtkTIFFReader.h>

	#include <vtkStringArray.h>

//#endif

const QString iAImageStackFileIO::Name("Image (Stack)");

namespace
{
	QString const LoadTypeStr("Loading Type");
	QString const SingleImageOption("Single Image");
	QString const ImageStackOption("Image Stack");
	QString const SpacingStr("Spacing");
	QString const FileNameBase("File name base");
	QString const Extension("Extension");
	QString const NumDigits("Number of digits in index");
	QString const MinimumIndex("Minimum index");
	QString const MaximumIndex("Maximum index");
	QString const StepStr("Step");
	QString const OriginStr("Origin");

	vtkNew<vtkStringArray> fileNameArray(QString fileNameBase, QString const & suffix, int* indexRange, int digitsInIndex, int stepSize)
	{
		vtkNew<vtkStringArray> result;
		for (int i = indexRange[0]; i <= indexRange[1]; i += stepSize)
		{
			QString temp = fileNameBase + QString("%1").arg(i, digitsInIndex, 10, QChar('0')) + suffix;
			result->InsertNextValue(getLocalEncodingFileName(temp).c_str());
		}
		return result;
	}
}

iAImageStackFileIO::iAImageStackFileIO() : iAFileIO(iADataSetType::Volume)
{
	QStringList loadTypes = QStringList() << SingleImageOption << ImageStackOption;
	addParameter(LoadTypeStr, iAValueType::Categorical, loadTypes);
	addParameter(StepStr, iAValueType::Discrete, 1);
	addParameter(SpacingStr, iAValueType::Vector3, variantVector<double>({1.0, 1.0, 1.0}));
	addParameter(OriginStr, iAValueType::Continuous, variantVector<double>({ 0.0, 0.0, 0.0 }));
}

void iAImageStackFileIO::setup(QString const& fileName)
{
	iAFileIO::setup(fileName);
	QString base, suffix;
	int range[2];
	int digits;
	determineStackParameters(fileName, base, suffix, range, digits);
	addParameter(FileNameBase, iAValueType::String, base);
	addParameter(Extension, iAValueType::String, suffix);
	addParameter(NumDigits, iAValueType::Discrete, digits);
	addParameter(MinimumIndex, iAValueType::Discrete, range[0]);
	addParameter(MaximumIndex, iAValueType::Discrete, range[1]);
}


std::vector<std::shared_ptr<iADataSet>> iAImageStackFileIO::load(iAProgress* progress, QVariantMap const& parameters)
{
	Q_UNUSED(parameters);

//#if RAW_LOAD_METHOD == ITK
//#else
	auto ext = QFileInfo(m_fileName).suffix().toLower();

	vtkSmartPointer<vtkImageReader2> imgReader;
	if (ext.endsWith("jpg") || ext.endsWith("jpeg"))
	{
		imgReader = vtkSmartPointer<vtkJPEGReader>::New();
	}
	else if (ext.endsWith("tif") || ext.endsWith("tiff"))
	{
		imgReader = vtkSmartPointer<vtkTIFFReader>::New();
	}
	else if (ext.endsWith("png"))
	{
		imgReader = vtkSmartPointer<vtkPNGReader>::New();
	}
	else if (ext.endsWith("bmp"))
	{
		imgReader = vtkSmartPointer<vtkBMPReader>::New();
	}
	else
	{
		throw std::runtime_error(QString("Unknown image extension '%1'!").arg(ext).toStdString());
	}
	imgReader->ReleaseDataFlagOn();
	progress->observe(imgReader);
	imgReader->AddObserver(vtkCommand::ErrorEvent, iAExceptionThrowingErrorObserver::New());

	if (parameters[LoadTypeStr] == SingleImageOption)
	{
		imgReader->SetFileName(getLocalEncodingFileName(m_fileName).c_str());
	}
	else
	{
		int indexRange[2];
		int digits;
		auto fileNameBase = parameters[FileNameBase].toString();
		auto suffix = parameters[Extension].toString();
		digits = parameters[NumDigits].toInt();
		indexRange[0] = parameters[MinimumIndex].toInt();
		indexRange[1] = parameters[MaximumIndex].toInt();
		int stepSize = parameters[StepStr].toInt();
		double spacing[3], origin[3];
		setFromVectorVariant<double>(spacing, parameters[SpacingStr]);
		setFromVectorVariant<double>(origin, parameters[OriginStr]);
		auto fileNames = fileNameArray(fileNameBase, suffix, indexRange, digits, stepSize);
		imgReader->SetFileNames(fileNames);
		imgReader->SetDataOrigin(origin);
		imgReader->SetDataSpacing(spacing);
	}
	auto img = vtkSmartPointer<vtkImageData>::New();
	imgReader->SetOutput(img);
	imgReader->Update();
	return { std::make_shared<iAImageData>(m_fileName, img) };
	// TODO: maybe compute range here as well?
	//auto rng = img->GetScalarRange();   // see also comments above about performance measurements
//#endif
}

QString iAImageStackFileIO::name() const
{
	return Name;
}

QStringList iAImageStackFileIO::extensions() const
{
	return QStringList{ "bmp", "jpg", "jpeg", "png", "tif", "tiff" };
}