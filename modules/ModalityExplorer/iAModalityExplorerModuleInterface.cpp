/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 
#include "pch.h"
#include "iAModalityExplorerModuleInterface.h"

#include "dlg_commoninput.h"
#include "iAConsole.h"
#include "iAIO.h"
#include "mainwindow.h"
#include "mdichild.h"

#include "dlg_modalities.h"
#include "iAModalityExplorerAttachment.h"
#include "iAModality.h"

#include <vtkImageReader2.h>
#include <vtkTIFFReader.h>
#include <vtkBMPReader.h>
#include <vtkPNGReader.h>
#include <vtkJPEGReader.h>
#include <vtkStringArray.h>

#include <QFileDialog>
#include <QMessageBox>

#include <cassert>


iAModalityExplorerModuleInterface::iAModalityExplorerModuleInterface()
{}

iAModalityExplorerAttachment* iAModalityExplorerModuleInterface::GetAttachment(MdiChild* child)
{
	m_mdiChild = child;
	if (!iAModuleInterface::GetAttachment<iAModalityExplorerAttachment>())
	{
		OpenDialog();
	}
	return iAModuleInterface::GetAttachment<iAModalityExplorerAttachment>();
}

void iAModalityExplorerModuleInterface::Initialize()
{
	QMenu * toolsMenu = m_mainWnd->getToolsMenu();
	QMenu * menuMultiModalChannel = getMenuWithTitle( toolsMenu, QString( "Multi-Modal/-Channel Images" ), false );
	QAction * actionModalityExplorer = new QAction(QApplication::translate("MainWindow", "Modality Explorer", 0), m_mainWnd );
	QAction * actionLoadModalities = new QAction(QApplication::translate("MainWindow", "Load Modalities", 0), m_mainWnd);
	QAction * actionTLGICTData = new QAction(QApplication::translate("MainWindow", "Load TLGI-CT Data", 0), m_mainWnd);
	AddActionToMenuAlphabeticallySorted(menuMultiModalChannel, actionModalityExplorer, true);
	AddActionToMenuAlphabeticallySorted(menuMultiModalChannel, actionLoadModalities, false);
	AddActionToMenuAlphabeticallySorted(menuMultiModalChannel, actionTLGICTData, false);
	connect(actionModalityExplorer, SIGNAL(triggered()), this, SLOT(ModalityExplorer()));
	connect(actionLoadModalities, SIGNAL(triggered()), this, SLOT(LoadModalities()));
	connect(actionTLGICTData, SIGNAL(triggered()), this, SLOT(OpenTLGICTData()));
}

bool iAModalityExplorerModuleInterface::OpenDialog()
{
	PrepareActiveChild();
	if (!m_mdiChild)
	{
		return false;
	}
	bool result = AttachToMdiChild( m_mdiChild );
	iAModalityExplorerAttachment* modExplorerAttach = iAModuleInterface::GetAttachment<iAModalityExplorerAttachment>();
	assert(modExplorerAttach);
	QSharedPointer<iAModalityList> modList(new iAModalityList);
	if (!m_mdiChild->currentFile().isEmpty())
	{
		modList->Add(QSharedPointer<iAModality>(new iAModality("Main", m_mdiChild->currentFile(), m_mdiChild->getImagePointer(), iAModality::MainRenderer)));
	}
	modExplorerAttach->SetModalities(modList);
	
	return result;
}

iAModuleAttachmentToChild* iAModalityExplorerModuleInterface::CreateAttachment(MainWindow* mainWnd, iAChildData childData)
{
	iAModalityExplorerAttachment* result = iAModalityExplorerAttachment::create( mainWnd, childData);
	return result;
}


void iAModalityExplorerModuleInterface::LoadModalities()
{
	PrepareActiveChild();
	bool loadFirstDataset = false;
	if (!m_mdiChild)
	{
		loadFirstDataset = true;
		MdiChild* child = m_mainWnd->createMdiChild();
		child->newFile();
		child->show();
		PrepareActiveChild();
	}
	iAModalityExplorerAttachment* modExplorerAttach = GetAttachment(m_mdiChild);
	if (modExplorerAttach->LoadModalities() && loadFirstDataset)
	{
		m_mdiChild->setImageData(
			modExplorerAttach->GetModalitiesDlg()->GetModalities()->Get(0)->GetFileName(),
			modExplorerAttach->GetModalitiesDlg()->GetModalities()->Get(0)->GetImage()
		);
	}
}

void iAModalityExplorerModuleInterface::ModalityExplorer()
{
	PrepareActiveChild();
	if (!m_mdiChild)
	{
		return;
	}
	if (iAModuleInterface::GetAttachment<iAModalityExplorerAttachment>())
	{
		return;
	}
	OpenDialog();
}



QString GreatestCommonPrefix(QString const & str1, QString const & str2)
{
	int pos = 0;
	while (pos < str1.size() && pos < str2.size() && str1.at(pos) == str2.at(pos))
	{
		++pos;
	}
	return str1.left(pos);
}

void iAModalityExplorerModuleInterface::OpenTLGICTData()
{
	QString baseDirectory = QFileDialog::getExistingDirectory(
		m_mainWnd,
		tr("Open Talbot-Lau Grating Interferometer CT Dataset"),
		m_mainWnd->getPath(),
		QFileDialog::ShowDirsOnly);
	
	if (baseDirectory.isEmpty())
	{
		return;
	}

	QDir dir(baseDirectory);
	QStringList nameFilter;
	dir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
	nameFilter << "*_Rec";
	dir.setNameFilters(nameFilter);
	QFileInfoList subDirs = dir.entryInfoList();
	if (subDirs.size() == 0)
	{
		DEBUG_LOG("No data found (expected to find subfolders with _Rec suffix)\n");
		return;
	}

	double spacing[3] = { 1, 1, 1 };
	double origin[3]  = { 0, 0, 0 };
	QStringList inList;
	inList << tr("#Spacing X") << tr("#Spacing Y") << tr("#Spacing Z")
		<< tr("#Origin X") << tr("#Origin Y") << tr("#Origin Z");
	QList<QVariant> inPara;
	inPara << tr("%1").arg(spacing[0]) << tr("%1").arg(spacing[1]) << tr("%1").arg(spacing[2])
		<< tr("%1").arg(origin[0]) << tr("%1").arg(origin[1]) << tr("%1").arg(origin[2]);

	dlg_commoninput *dlg = new dlg_commoninput(m_mainWnd, "Set file parameters", 6, inList, inPara, NULL);

	if (!dlg->exec() == QDialog::Accepted)
	{
		DEBUG_LOG("Data input aborted by user.\n");
		return;
	}

	spacing[0] = dlg->getValues()[0]; spacing[1] = dlg->getValues()[1]; spacing[2] = dlg->getValues()[2];
	origin[0] = dlg->getValues()[3];  origin[1] = dlg->getValues()[4];  origin[2] = dlg->getValues()[5];

	QSharedPointer<iAModalityList> modList(new iAModalityList);

	QStringList imgFilter;
	imgFilter << "*.tif" << "*.bmp" << "*.jpg" << "*.png";
	for (QFileInfo subDirFileInfo : subDirs)
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
			QString lastDigit = imgFileName.mid(imgFileName.length() - (completeSuffix.length() + 2), 1 );
			bool ok;
			int myNum = lastDigit.toInt(&ok);
			if (!ok)
			{
				//DEBUG_LOG(QString("Skipping image with no number at end '%1'\n").arg(imgFileName));
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
					DEBUG_LOG(QString("Inconsistent file suffix: %1 has %2, previous files had %3\n").arg(imgFileName).arg(completeSuffix).arg(ext));
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
				DEBUG_LOG(QString("Invalid, non-numeric part (%1) in image file name '%2'\n").arg(numStr).arg(imgFileName));
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
			DEBUG_LOG(QString("Stack loading: not all indices in the interval [%1, %2] are used for base name %3.\n").arg(min).arg(max).arg(fileNameBase));
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
			DEBUG_LOG(QString("Unknown or undefined image extension (%1)!\n").arg(ext));
			return;
		}
		reader->SetFileNames(fileNames);
		reader->SetDataOrigin(origin);
		reader->SetDataSpacing(spacing);
		reader->Update();
		vtkSmartPointer<vtkImageData> img = reader->GetOutput();

		// add modality
		QString modName = subDirFileInfo.baseName();
		modName = modName.left(modName.length() - 4); // 4 => length of "_rec"
		modList->Add(QSharedPointer<iAModality>(new iAModality(modName, subDirFileInfo.absoluteFilePath(), img, 0)));
	}

	MdiChild* child = m_mainWnd->createMdiChild();
	child->newFile();
	child->show();
	PrepareActiveChild();
	OpenDialog();

	iAModalityExplorerAttachment* modalityAttachment = iAModuleInterface::GetAttachment<iAModalityExplorerAttachment>();

	if (modList->size() == 0)
	{
		DEBUG_LOG("No modalities loaded!\n");
		return;
	}

	modalityAttachment->SetModalities(modList);

	m_mdiChild->setImageData(
		// TODO: avoid method chaining!
		modalityAttachment->GetModalitiesDlg()->GetModalities()->Get(0)->GetFileName(),
		modalityAttachment->GetModalitiesDlg()->GetModalities()->Get(0)->GetImage()
	);
}
