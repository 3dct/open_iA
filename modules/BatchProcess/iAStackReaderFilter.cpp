/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
#include "iAStackReaderFilter.h"

#include <iAConsole.h>
#include <iAExceptionThrowingErrorObserver.h>
#include <io/iAFileUtils.h>
#include <iAProgress.h>
#include <iAStringHelper.h>
#include <iAToolsVTK.h>

#include <vtkCommand.h>
#include <vtkImageReader2.h>
#include <vtkBMPReader.h>
#include <vtkImageData.h>
#include <vtkJPEGReader.h>
#include <vtkPNGReader.h>
#include <vtkStringArray.h>
#include <vtkTIFFReader.h>

#include <QFileInfo>
#include <QDir>

void iAStackReaderFilter::performWork(QMap<QString, QVariant> const & parameters)
{
	QString fullFileName = parameters["Folder name"].toString() + "/" + parameters["File name base"].toString();
	QFileInfo fi(fullFileName);
	QDir dir(fi.absolutePath());
	QStringList nameFilters;
	nameFilters << "*." + fi.suffix();
	QFileInfoList imgFiles = dir.entryInfoList(nameFilters);
	// determine most common file name base
	QString fileNamesBase;
	for (QFileInfo imgFileInfo : imgFiles)
	{
		QString imgFileName = imgFileInfo.absoluteFilePath();
		QString suffix = imgFileInfo.suffix();
		QString lastDigit = imgFileName.mid(imgFileName.length() - (suffix.length() + 2), 1);
		bool ok;
		int myNum = lastDigit.toInt(&ok);
		if (!ok)
		{
			DEBUG_LOG(QString("Skipping image with no number at end '%1'.").arg(imgFileName));
			continue;
		}
		if (fileNamesBase.isEmpty())
		{
			fileNamesBase = imgFileInfo.absoluteFilePath();
		}
		else
		{
			fileNamesBase = GreatestCommonPrefix(fileNamesBase, imgFileInfo.absoluteFilePath());
		}
	}
	int baseLength = fileNamesBase.length();
	// determine index range:
	int indexRange[2] = { std::numeric_limits<int>::max(), std::numeric_limits<int>::min() };
	int digits = -1;
	for (QFileInfo imgFileInfo : imgFiles)
	{
		QString imgFileName = imgFileInfo.absoluteFilePath();
		QString suffix = imgFileInfo.suffix();
		QString lastDigit = imgFileName.mid(imgFileName.length() - (suffix.length() + 2), 1);
		bool ok;
		int myNum = lastDigit.toInt(&ok);
		if (!ok)
		{
			//DEBUG_LOG(QString("Skipping image with no number at end '%1'.").arg(imgFileName));
			continue;
		}
		QString numStr = imgFileName.mid(baseLength, imgFileName.length() - baseLength - suffix.length() - 1);
		if (digits == -1)
		{
			digits = numStr.length();
		}
		int num = numStr.toInt(&ok);
		if (!ok)
		{
			DEBUG_LOG(QString("Invalid, non-numeric part (%1) in image file name '%2'.").arg(numStr).arg(imgFileName));
			continue;
		}
		if (num < indexRange[0])
		{
			indexRange[0] = num;
		}
		if (num > indexRange[1])
		{
			indexRange[1] = num;
		}
	}
	QString extension = "." + fi.suffix();

	vtkSmartPointer<vtkImageReader2> imgReader;
	if (extension.toUpper() == ".TIF" || extension.toUpper() == ".TIFF")
		imgReader = vtkSmartPointer<vtkTIFFReader>::New();
	else if (extension.toUpper() == ".JPG" || extension.toUpper() == ".JPEG")
		imgReader = vtkSmartPointer<vtkJPEGReader>::New();
	else if (extension.toUpper() == ".PNG")
		imgReader = vtkSmartPointer<vtkPNGReader>::New();
	else if (extension.toUpper() == ".BMP")
		imgReader = vtkSmartPointer<vtkBMPReader>::New();
	else
		throw std::runtime_error(QString("Unknown filetype of extension %1").arg(extension).toStdString());
	progress()->Observe(imgReader);
	imgReader->AddObserver(vtkCommand::ErrorEvent, iAExceptionThrowingErrorObserver::New());
	auto fileNameArray = vtkSmartPointer<vtkStringArray>::New();
	for (int i = indexRange[0]; i <= indexRange[1]; i++)
	{
		QString temp = fileNamesBase + QString("%1").arg(i, digits, 10, QChar('0')) + extension;
		fileNameArray->InsertNextValue(getLocalEncodingFileName(temp));
	}
	imgReader->SetFileNames(fileNameArray);
	double origin[3];
	origin[0] = origin[1] = origin[2] = 0;
	imgReader->SetDataOrigin(origin);
	double spacing[3];
	spacing[0] = parameters["Spacing X"].toDouble();
	spacing[1] = parameters["Spacing Y"].toDouble();
	spacing[2] = parameters["Spacing Z"].toDouble();
	imgReader->SetDataSpacing(spacing);
	imgReader->Update();
	addOutput(imgReader->GetOutput());
}

IAFILTER_CREATE(iAStackReaderFilter)

iAStackReaderFilter::iAStackReaderFilter() :
	iAFilter("Image Stack Reader", "Readers",
		"Read an image stack.<br/>"
		"Minimum and maximum index are automatically determined "
		"from the given folder name, spacing and datatype can be adapted.", 0, 1)
{
	addParameter("Folder name", String, "");
	addParameter("File name base", String, "");
	addParameter("Spacing X", Continuous, 1.0);
	addParameter("Spacing Y", Continuous, 1.0);
	addParameter("Spacing Z", Continuous, 1.0);
}