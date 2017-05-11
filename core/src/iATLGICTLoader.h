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
#pragma once

#include "iAModality.h"
#include "iAModalityList.h"
#include "iAObserverProgress.h"

#include <vtkImageReader2.h>
#include <vtkTIFFReader.h>
#include <vtkBMPReader.h>
#include <vtkPNGReader.h>
#include <vtkJPEGReader.h>
#include <vtkStringArray.h>

#include <QDir>
#include <QFileInfoList>
#include <QThread>


QString GreatestCommonPrefix(QString const & str1, QString const & str2)
{
	int pos = 0;
	while (pos < str1.size() && pos < str2.size() && str1.at(pos) == str2.at(pos))
	{
		++pos;
	}
	return str1.left(pos);
}


class iATLGICTLoader : public QThread
{
public:
	iATLGICTLoader(QSharedPointer<iAModalityList> modList, double const * const spacing, double const * const origin, QFileInfoList subDirs, iAObserverProgress* observer) :
		m_modList(modList),
		m_subDirs(subDirs),
		m_observer(observer)
	{
		std::copy(spacing, spacing + 3, m_spacing);
		std::copy(origin, origin + 3, m_origin);
	}
private:
	QSharedPointer<iAModalityList> m_modList;
	double m_spacing[3];
	double m_origin[3];
	QFileInfoList m_subDirs;
	iAObserverProgress* m_observer;
	void run()
	{
		QStringList imgFilter;
		imgFilter << "*.tif" << "*.bmp" << "*.jpg" << "*.png";
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
			reader->AddObserver(vtkCommand::ProgressEvent, m_observer);
			reader->Update();
			vtkSmartPointer<vtkImageData> img = reader->GetOutput();

			// add modality
			QString modName = subDirFileInfo.baseName();
			modName = modName.left(modName.length() - 4); // 4 => length of "_rec"
			m_modList->Add(QSharedPointer<iAModality>(new iAModality(modName, subDirFileInfo.absoluteFilePath(), -1, img, 0)));
		}
		if (m_modList->size() == 0)
		{
			DEBUG_LOG("No modalities loaded!");
			return;
		}

	}
};
