// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iATLGICTLoader.h"

#include <iAJobListView.h>
#include <iAMultiStepProgressObserver.h>
#include <iAParameterDlg.h>
#include <iAMdiChild.h>

#include <iAFileUtils.h>
#include <iALog.h>
#include <iAImageData.h>
#include <iAStringHelper.h>

#include <vtkImageData.h>
#include <vtkImageReader2.h>
#include <vtkTIFFReader.h>
#include <vtkBMPReader.h>
#include <vtkPNGReader.h>
#include <vtkJPEGReader.h>
#include <vtkStringArray.h>

#include <QDir>
#include <QSettings>


iATLGICTLoader::iATLGICTLoader():
	m_multiStepObserver(nullptr)
{}


bool iATLGICTLoader::setup(QString const& baseDirectory, QWidget* parent)
{
	m_baseDirectory = baseDirectory;

	if (m_baseDirectory.isEmpty())
	{
		return false;
	}
	QDir dir(m_baseDirectory);
	QStringList nameFilter;
	nameFilter << "*_Rec";
	m_subDirs = dir.entryInfoList(nameFilter, QDir::Dirs | QDir::NoDotAndDotDot);
	if (m_subDirs.size() == 0)
	{
		LOG(lvlError, "No data found (expected to find subfolders with _Rec suffix).");
		return false;
	}

	QStringList logFileFilter;
	logFileFilter << "*.log";
	QFileInfoList logFiles = dir.entryInfoList(logFileFilter, QDir::Files);
	if (logFiles.size() == 0)
	{
		LOG(lvlError, "No log file found (expected to find a file with .log suffix).");
		return false;
	}
	QSettings iniLog(logFiles[0].absoluteFilePath(), QSettings::IniFormat);
	double pixelSize = iniLog.value("Reconstruction/Pixel Size (um)", 1000).toDouble() / 1000;
	iAAttributes params;
	addAttr(params, "Spacing X", iAValueType::Continuous, pixelSize);
	addAttr(params, "Spacing Y", iAValueType::Continuous, pixelSize);
	addAttr(params, "Spacing Z", iAValueType::Continuous, pixelSize);
	addAttr(params, "Origin X", iAValueType::Continuous, 0);
	addAttr(params, "Origin Y", iAValueType::Continuous, 0);
	addAttr(params, "Origin Z", iAValueType::Continuous, 0);
	iAParameterDlg dlg(parent, "Set file parameters", params);
	if (dlg.exec() != QDialog::Accepted)
	{
		return false;
	}
	auto values = dlg.parameterValues();
	m_spacing[0] = values["Spacing X"].toDouble();
	m_spacing[1] = values["Spacing Y"].toDouble();
	m_spacing[2] = values["Spacing Z"].toDouble();
	m_origin[0] = values["Origin X"].toDouble();
	m_origin[1] = values["Origin Y"].toDouble();
	m_origin[2] = values["Origin Z"].toDouble();
	return true;
}

void iATLGICTLoader::start(iAMdiChild* child)
{
	m_multiStepObserver = new iAMultiStepProgressObserver(m_subDirs.size());
	m_child = child;
	m_child->show();
	LOG(lvlInfo, tr("Loading TLGI-CT data."));
	iAJobListView::get()->addJob("Loading TLGI-CT data", m_multiStepObserver->progressObject(), this);
	connect(this, &iATLGICTLoader::finished, this, &iATLGICTLoader::finishUp);		// this needs to be last, as it deletes this object!
	QThread::start();
}

iATLGICTLoader::~iATLGICTLoader()
{
	delete m_multiStepObserver;
}

void iATLGICTLoader::run()
{
	m_result = std::make_shared<iADataCollection>(m_subDirs.size(), std::shared_ptr<QSettings>());
	QStringList imgFilter;
	imgFilter << "*.tif" << "*.bmp" << "*.jpg" << "*.png";
	int completedDirs = 0;
	for (QFileInfo subDirFileInfo : m_subDirs)
	{
		QDir subDir(subDirFileInfo.absoluteFilePath());
		subDir.setFilter(QDir::Files);
		subDir.setNameFilters(imgFilter);
		QFileInfoList imgFiles = subDir.entryInfoList();
		QString fileNameBase;
		// determine most common file name base
		// TODO: merge with image stack file name guessing!
		for (QFileInfo imgFileInfo : imgFiles)
		{
			if (fileNameBase.isEmpty())
			{
				fileNameBase = imgFileInfo.absoluteFilePath();
			}
			else
			{
				fileNameBase = greatestCommonPrefix(fileNameBase, imgFileInfo.absoluteFilePath());
			}
		}
		int baseLength = fileNameBase.length();
		// determine index range:
		int min = std::numeric_limits<int>::max();
		int max = std::numeric_limits<int>::min();
		QString ext;
		int digits = -1;
		for (QFileInfo imgFileInfo : imgFiles)
		{
			QString imgFileName = imgFileInfo.absoluteFilePath();
			QString completeSuffix = imgFileInfo.completeSuffix();
			QString lastDigit = imgFileName.mid(imgFileName.length() - (completeSuffix.length() + 2), 1);
			bool ok;
			/*int myNum =*/ lastDigit.toInt(&ok);
			if (!ok)
			{
				//LOG(lvlInfo, QString("Skipping image with no number at end '%1'.").arg(imgFileName));
				continue;
			}
			if (ext.isEmpty())
			{
				ext = completeSuffix;
			}
			else
			{
				if (ext != completeSuffix)
				{
					LOG(lvlInfo, QString("Inconsistent file suffix: %1 has %2, previous files had %3.").arg(imgFileName).arg(completeSuffix).arg(ext));
					return;
				}
			}

			QString numStr = imgFileName.mid(baseLength, imgFileName.length() - baseLength - completeSuffix.length() - 1);
			if (digits == -1)
			{
				digits = numStr.length();
			}

			int num = numStr.toInt(&ok);
			if (!ok)
			{
				LOG(lvlInfo, QString("Invalid, non-numeric part (%1) in image file name '%2'.").arg(numStr).arg(imgFileName));
				return;
			}
			if (num < min)
			{
				min = num;
			}
			if (num > max)
			{
				max = num;
			}
		}

		if (max - min + 1 > imgFiles.size())
		{
			LOG(lvlInfo, QString("Stack loading: not all indices in the interval [%1, %2] are used for base name %3.").arg(min).arg(max).arg(fileNameBase));
			return;
		}

		vtkSmartPointer<vtkStringArray> fileNames = vtkSmartPointer<vtkStringArray>::New();
		for (int i = min; i <= max; i++)
		{
			QString temp = fileNameBase + QString("%1").arg(i, digits, 10, QChar('0')) + "." + ext;
			temp = temp.replace("/", "\\");
			fileNames->InsertNextValue(getLocalEncodingFileName(temp));
		}

		// load image stack // TODO: maybe reuse iAImageStackFileIO here?
		ext = ext.toLower();
		vtkSmartPointer<vtkImageReader2> reader;
		if (ext == "jpg" || ext == "jpeg")
		{
			reader = vtkSmartPointer<vtkJPEGReader>::New();
		}
		else if (ext == "png")
		{
			reader = vtkSmartPointer<vtkPNGReader>::New();
		}
		else if (ext == "bmp")
		{
			reader = vtkSmartPointer<vtkBMPReader>::New();
		}
		else if (ext == "tif" || ext == "tiff")
		{
			reader = vtkSmartPointer<vtkTIFFReader>::New();
		}
		else
		{
			LOG(lvlError, QString("Unknown or undefined image extension (%1)!").arg(ext));
			return;
		}
		reader->SetFileNames(fileNames);
		reader->SetDataOrigin(m_origin);
		reader->SetDataSpacing(m_spacing);
		m_multiStepObserver->observe(reader);		// intercept progress and divide by number of images!
		reader->Update();
		vtkSmartPointer<vtkImageData> img = reader->GetOutput();

		QString dataSetName = subDirFileInfo.baseName();
		dataSetName = dataSetName.left(dataSetName.length() - 4); // 4 => length of "_rec"

		auto imgData = std::make_shared<iAImageData>(img);
		imgData->setMetaData(iADataSet::NameKey, dataSetName);
		m_result->addDataSet(imgData);
		m_multiStepObserver->setCompletedSteps(++completedDirs);
	}
	if (m_result->dataSets().empty())
	{
		LOG(lvlError, "No datasets loaded!");
		return;
	}
}

void iATLGICTLoader::finishUp()
{
	m_child->setWindowTitleAndFile(m_baseDirectory);
	m_child->addDataSet(m_result);
	LOG(lvlInfo, tr("Loading sequence completed; directory: %1.").arg(m_baseDirectory));
	delete this;
}
