/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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
#include "iATLGICTLoader.h"

#include "dlg_commoninput.h"
#include "iAConsole.h"
#include "iAModality.h"
#include "iAModalityList.h"
#include "iAMultiStepProgressObserver.h"
#include "iAObserverProgress.h"
#include "mdichild.h"

#include <vtkImageData.h>
#include <vtkImageReader2.h>
#include <vtkTIFFReader.h>
#include <vtkBMPReader.h>
#include <vtkPNGReader.h>
#include <vtkJPEGReader.h>
#include <vtkStringArray.h>

#include <QDateTime>
#include <QDir>
#include <QSettings>

namespace
{
	QString GreatestCommonPrefix(QString const & str1, QString const & str2)
	{
		int pos = 0;
		while (pos < str1.size() && pos < str2.size() && str1.at(pos) == str2.at(pos))
		{
			++pos;
		}
		return str1.left(pos);
	}
}


iATLGICTLoader::iATLGICTLoader():
	m_multiStepObserver(0)
{}


bool iATLGICTLoader::setup(QString const & baseDirectory, QWidget* parent)
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
		DEBUG_LOG("No data found (expected to find subfolders with _Rec suffix).");
		return false;
	}

	QStringList logFileFilter;
	logFileFilter << "*.log";
	QFileInfoList logFiles = dir.entryInfoList(logFileFilter, QDir::Files);
	if (logFiles.size() == 0)
	{
		DEBUG_LOG("No log file found (expected to find a file with .log suffix).");
		return false;
	}
	QSettings iniLog(logFiles[0].absoluteFilePath(), QSettings::IniFormat);
	double pixelSize = iniLog.value("Reconstruction/Pixel Size (um)", 1000).toDouble() / 1000;

	double spacing[3] = { pixelSize, pixelSize, pixelSize };
	double origin[3] = { 0, 0, 0 };
	QStringList inList;
	inList << tr("#Spacing X") << tr("#Spacing Y") << tr("#Spacing Z")
		<< tr("#Origin X") << tr("#Origin Y") << tr("#Origin Z");
	QList<QVariant> inPara;
	inPara << tr("%1").arg(spacing[0]) << tr("%1").arg(spacing[1]) << tr("%1").arg(spacing[2])
		<< tr("%1").arg(origin[0]) << tr("%1").arg(origin[1]) << tr("%1").arg(origin[2]);

	dlg_commoninput dlg(parent, "Set file parameters", inList, inPara, NULL);
	if (!dlg.exec() == QDialog::Accepted)
	{
		return false;
	}
	m_spacing[0] = dlg.getValues()[0]; m_spacing[1] = dlg.getValues()[1]; m_spacing[2] = dlg.getValues()[2];
	m_origin[0] = dlg.getValues()[3]; m_origin[1] = dlg.getValues()[4]; m_origin[2] = dlg.getValues()[5];
	return true;
}


void iATLGICTLoader::start(MdiChild* child)
{
	m_multiStepObserver = new iAMultiStepProgressObserver(m_subDirs.size());
	m_child = child;
	m_child->show();
	m_child->addMsg(tr("%1  Loading sequence started... \n"
		"  The duration of the loading sequence depends on the size of your data set and may last several minutes. \n"
		"  Please wait...").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat)));

	connect(m_multiStepObserver, SIGNAL(oprogress(int)), m_child, SLOT(updateProgressBar(int)));
	connect(this, SIGNAL(started()), m_child, SLOT(initProgressBar()));
	connect(this, SIGNAL(finished()), m_child, SLOT(hideProgressBar()));
	connect(this, SIGNAL(finished()), this, SLOT(finishUp()));		// this needs to be last, as it deletes this object!
	QThread::start();
}


iATLGICTLoader::~iATLGICTLoader()
{
	delete m_multiStepObserver;
}


void iATLGICTLoader::run()
{
	m_modList = QSharedPointer<iAModalityList>(new iAModalityList);
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
		for (QFileInfo imgFileInfo : imgFiles)
		{
			if (fileNameBase.isEmpty())
			{
				fileNameBase = imgFileInfo.absoluteFilePath();
			}
			else
			{
				fileNameBase = GreatestCommonPrefix(fileNameBase, imgFileInfo.absoluteFilePath());
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
			int myNum = lastDigit.toInt(&ok);
			if (!ok)
			{
				//DEBUG_LOG(QString("Skipping image with no number at end '%1'.").arg(imgFileName));
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
					DEBUG_LOG(QString("Inconsistent file suffix: %1 has %2, previous files had %3.").arg(imgFileName).arg(completeSuffix).arg(ext));
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
				DEBUG_LOG(QString("Invalid, non-numeric part (%1) in image file name '%2'.").arg(numStr).arg(imgFileName));
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
			DEBUG_LOG(QString("Stack loading: not all indices in the interval [%1, %2] are used for base name %3.").arg(min).arg(max).arg(fileNameBase));
			return;
		}

		vtkSmartPointer<vtkStringArray> fileNames = vtkSmartPointer<vtkStringArray>::New();
		for (int i = min; i <= max; i++)
		{
			QString temp = fileNameBase + QString("%1").arg(i, digits, 10, QChar('0')) + "." + ext;
			temp = temp.replace("/", "\\");
			fileNames->InsertNextValue(temp.toLatin1());
		}

		// load image stack // TODO: put to common location and use from iAIO!
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
			DEBUG_LOG(QString("Unknown or undefined image extension (%1)!").arg(ext));
			return;
		}
		reader->SetFileNames(fileNames);
		reader->SetDataOrigin(m_origin);
		reader->SetDataSpacing(m_spacing);
		reader->AddObserver(vtkCommand::ProgressEvent, m_multiStepObserver);		// intercept progress and divide by number of images!
		reader->Update();
		vtkSmartPointer<vtkImageData> img = reader->GetOutput();

		// add modality
		QString modName = subDirFileInfo.baseName();
		modName = modName.left(modName.length() - 4); // 4 => length of "_rec"
		m_modList->Add(QSharedPointer<iAModality>(new iAModality(modName, subDirFileInfo.absoluteFilePath(), -1, img, 0)));
		m_multiStepObserver->SetCompletedSteps(++completedDirs);
	}
	if (m_modList->size() == 0)
	{
		DEBUG_LOG("No modalities loaded!");
		return;
	}
}



void iATLGICTLoader::finishUp()
{
	m_child->setCurrentFile(m_baseDirectory);
	m_child->SetModalities(m_modList);
	m_child->addMsg(tr("%1  Loading sequence completed.").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat)));
	m_child->addMsg("  Directory: " + m_baseDirectory);
	delete this;
}
