/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
#include "dlg_openfile_sizecheck.h"

#include "iAToolsVTK.h"

#include <QComboBox>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QObject>


dlg_openfile_sizecheck::dlg_openfile_sizecheck(bool isVolumeStack, QWidget *parent, QString winTitle, QStringList inList, QList<QVariant> inPara, QTextDocument *fDescr, QString fileName, int extentIndex1, int extentIndex2, int extentIndex3, int datatypeIndex) :
	dlg_commoninput(parent, winTitle, inList, inPara, fDescr)
{
	this->isVolumeStack = isVolumeStack;
	QFileInfo info1(fileName);
	fileSize = info1.size();

	actualSizeLabel = new QLabel("Actual file size: " + QString::number(fileSize) + " bytes", this);
	actualSizeLabel->setAlignment(Qt::AlignRight);
	gridLayout->addWidget(actualSizeLabel, inList.size(), 0, 1, 1);
	
	proposedSizeLabel = new QLabel("Predicted file size: ", this);
	proposedSizeLabel->setAlignment(Qt::AlignRight);
	gridLayout->addWidget(proposedSizeLabel, inList.size()+1, 0, 1, 1);

	gridLayout->addWidget(buttonBox, inList.size()+2, 0, 1, 1);
		

	if (!isVolumeStack)
	{
		connect(qobject_cast<QLineEdit*>(widgetList[0]), SIGNAL(textChanged (const QString)), this, SLOT(CheckFileSize()));
		connect(qobject_cast<QLineEdit*>(widgetList[1]), SIGNAL(textChanged (const QString)), this, SLOT(CheckFileSize()));
		connect(qobject_cast<QLineEdit*>(widgetList[2]), SIGNAL(textChanged (const QString)), this, SLOT(CheckFileSize()));
		connect(qobject_cast<QComboBox*>(widgetList[10]), SIGNAL(currentIndexChanged (int)), this, SLOT(CheckFileSize()));
	}
	else
	{
		connect(qobject_cast<QLineEdit*>(widgetList[5]), SIGNAL(textChanged (const QString)), this, SLOT(CheckFileSize()));
		connect(qobject_cast<QLineEdit*>(widgetList[6]), SIGNAL(textChanged (const QString)), this, SLOT(CheckFileSize()));
		connect(qobject_cast<QLineEdit*>(widgetList[7]), SIGNAL(textChanged (const QString)), this, SLOT(CheckFileSize()));
		connect(qobject_cast<QComboBox*>(widgetList[14]), SIGNAL(currentIndexChanged (int)), this, SLOT(CheckFileSize()));
	}
	
	CheckFileSize();
}


void dlg_openfile_sizecheck::CheckFileSize()
{
	size_t extent[3];
	size_t voxelSize = 0; 
	size_t proposedSize;

	if (!isVolumeStack) {
		extent[0] = getDblValue(0); extent[1]= getDblValue(1); extent[2] = getDblValue(2);
		voxelSize = MapVTKTypeStringToSize(getComboBoxValue(10));
		proposedSize = extent[0]*extent[1]*extent[2]*voxelSize;
	}
	else 
	{
		extent[0] = getDblValue(5); extent[1]= getDblValue(6); extent[2] = getDblValue(7);
		voxelSize = MapVTKTypeStringToSize(getComboBoxValue(14));
		proposedSize = extent[0]*extent[1]*extent[2]*voxelSize;
	}
	proposedSizeLabel->setText("Predicted file size: " + QString::number(proposedSize) + " bytes");
}
