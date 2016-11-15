/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenb�ck, *
*                     Artem & Alexander Amirkhanov, B. Fr�hler                        *
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
* Contact: FH O� Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstra�e 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
 
#include "pch.h"
#include "dlg_openfile_sizecheck.h"

#include <QComboBox>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QObject>


dlg_openfile_sizecheck::dlg_openfile_sizecheck(bool isVolumeStack, QWidget *parent, QString winTitle, int n, QStringList inList, QList<QVariant> inPara, QTextDocument *fDescr, QString fileName, int extentIndex1, int extentIndex2, int extentIndex3, int datatypeIndex, bool modal) : dlg_commoninput(parent, winTitle, n, inList, inPara, fDescr, modal)
{
	this->isVolumeStack = isVolumeStack;
	QFileInfo info1(fileName);
	fileSize = info1.size();

	actualSizeLabel = new QLabel("Actual file size: " + QString::number(fileSize) + " bytes", this);
	actualSizeLabel->setAlignment(Qt::AlignRight);
	gridLayout->addWidget(actualSizeLabel, n, 0, 1, 1);
	
	proposedSizeLabel = new QLabel("Predicted file size: ", this);
	proposedSizeLabel->setAlignment(Qt::AlignRight);
	gridLayout->addWidget(proposedSizeLabel, n+1, 0, 1, 1);

	gridLayout->addWidget(buttonBox, n+2, 0, 1, 1);
		

	if (!isVolumeStack) {
		connect(this->findChild<QLineEdit*>(widgetList[0]), SIGNAL(textChanged (const QString)), this, SLOT(CheckFileSize()));
		connect(this->findChild<QLineEdit*>(widgetList[1]), SIGNAL(textChanged (const QString)), this, SLOT(CheckFileSize()));
		connect(this->findChild<QLineEdit*>(widgetList[2]), SIGNAL(textChanged (const QString)), this, SLOT(CheckFileSize()));
		connect(this->findChild<QComboBox*>(widgetList[10]), SIGNAL(currentIndexChanged (int)), this, SLOT(CheckFileSize()));
	}

	else {
		connect(this->findChild<QLineEdit*>(widgetList[5]), SIGNAL(textChanged (const QString)), this, SLOT(CheckFileSize()));
		connect(this->findChild<QLineEdit*>(widgetList[6]), SIGNAL(textChanged (const QString)), this, SLOT(CheckFileSize()));
		connect(this->findChild<QLineEdit*>(widgetList[7]), SIGNAL(textChanged (const QString)), this, SLOT(CheckFileSize()));
		connect(this->findChild<QComboBox*>(widgetList[14]), SIGNAL(currentIndexChanged (int)), this, SLOT(CheckFileSize()));
	}
	
	CheckFileSize();
}


int dlg_openfile_sizecheck::CheckFileSize()
{
	size_t extent[3];
	size_t voxelSize = 0; 
	size_t proposedSize;

	if (!isVolumeStack) {
		extent[0] = getValues()[0]; extent[1]= getValues()[1]; extent[2] = getValues()[2];     

		if (getComboBoxValues()[10] == "VTK_UNSIGNED_CHAR") voxelSize = sizeof(unsigned char);
		else if (getComboBoxValues()[10] == "VTK_UNSIGNED_SHORT") voxelSize = sizeof(unsigned short);
		else if (getComboBoxValues()[10] == "VTK_FLOAT") voxelSize = sizeof(float);
		else if (getComboBoxValues()[10] == "VTK_UNSIGNED_CHAR") voxelSize = sizeof(unsigned char);
		else if (getComboBoxValues()[10] == "VTK_CHAR") voxelSize = sizeof(char);
		else if (getComboBoxValues()[10] == "VTK_SHORT") voxelSize = sizeof(short);
		else if (getComboBoxValues()[10] == "VTK_UNSIGNED_INT") voxelSize = sizeof(unsigned int);
		else if (getComboBoxValues()[10] == "VTK_INT") voxelSize = sizeof(int);
		else if (getComboBoxValues()[10] == "VTK_DOUBLE") voxelSize = sizeof(double);

		proposedSize = extent[0]*extent[1]*extent[2]*voxelSize;
	}
	else 
	{
		extent[0] = getValues()[5]; extent[1]= getValues()[6]; extent[2] = getValues()[7];
		if (getComboBoxValues()[14] == "VTK_UNSIGNED_CHAR") voxelSize = sizeof(unsigned char);
		else if (getComboBoxValues()[14] == "VTK_UNSIGNED_SHORT") voxelSize = sizeof(unsigned short);
		else if (getComboBoxValues()[14] == "VTK_FLOAT") voxelSize = sizeof(float);
		else if (getComboBoxValues()[14] == "VTK_UNSIGNED_CHAR") voxelSize = sizeof(unsigned char);
		else if (getComboBoxValues()[14] == "VTK_CHAR") voxelSize = sizeof(char);
		else if (getComboBoxValues()[14] == "VTK_SHORT") voxelSize = sizeof(short);
		else if (getComboBoxValues()[14] == "VTK_UNSIGNED_INT") voxelSize = sizeof(unsigned int);
		else if (getComboBoxValues()[14] == "VTK_INT") voxelSize = sizeof(int);
		else if (getComboBoxValues()[14] == "VTK_DOUBLE") voxelSize = sizeof(double);

		proposedSize = extent[0]*extent[1]*extent[2]*voxelSize;
	}

	proposedSizeLabel->setText("Predicted file size: " + QString::number(proposedSize) + " bytes");

	return 1;
}
