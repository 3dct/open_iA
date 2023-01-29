// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include <iAExceptionThrowingErrorObserver.h>
#include <iAFileUtils.h>
#include <iAFilterDefault.h>
#include <iALog.h>
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

// TODO NEWIO: merge with iAImageStackFileIO ?
IAFILTER_DEFAULT_CLASS(iAStackReaderFilter);

void iAStackReaderFilter::performWork(QVariantMap const & parameters)
{
	QString fileName = parameters["File name"].toString();
	QFileInfo fi(fileName);
	QString prefix, suffix;
	int indexRange[2];
	int digits;
	determineStackParameters(fileName, prefix, suffix, indexRange, digits);

	vtkSmartPointer<vtkImageReader2> imgReader;
	auto ext = fi.suffix().toUpper();
	if (ext == "TIF" || ext == "TIFF")
	{
		imgReader = vtkSmartPointer<vtkTIFFReader>::New();
	}
	else if (ext == "JPG" || ext == "JPEG")
	{
		imgReader = vtkSmartPointer<vtkJPEGReader>::New();
	}
	else if (ext == "PNG")
	{
		imgReader = vtkSmartPointer<vtkPNGReader>::New();
	}
	else if (ext == "BMP")
	{
		imgReader = vtkSmartPointer<vtkBMPReader>::New();
	}
	else
	{
		throw std::runtime_error(QString("Unknown filetype of extension %1").arg(ext).toStdString());
	}
	progress()->observe(imgReader);
	imgReader->AddObserver(vtkCommand::ErrorEvent, iAExceptionThrowingErrorObserver::New());
	auto fileNameArray = vtkSmartPointer<vtkStringArray>::New();
	for (int i = indexRange[0]; i <= indexRange[1]; i++)
	{
		QString temp = prefix + QString("%1").arg(i, digits, 10, QChar('0')) + suffix;
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

iAStackReaderFilter::iAStackReaderFilter() :
	iAFilter("Image Stack Reader", "Input",
		"Read an image stack.<br/>"
		"Minimum and maximum index are automatically determined "
		"from the given filename (any image file which is part of the stack should do), spacing and datatype can be adapted.", 0, 1)
{
	addParameter("File name", iAValueType::FileNameOpen, "");
	addParameter("Spacing X", iAValueType::Continuous, 1.0);
	addParameter("Spacing Y", iAValueType::Continuous, 1.0);
	addParameter("Spacing Z", iAValueType::Continuous, 1.0);
}
