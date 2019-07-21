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
#include "dlg_openfile_sizecheck.h"

#include "iAToolsVTK.h"    // for mapVTKTypeStringToSize

#include <QComboBox>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

dlg_openfile_sizecheck::dlg_openfile_sizecheck(bool isVolumeStack, QString const & fileName, QWidget *parent, QString const & title,
	QStringList const & labels, QList<QVariant> const & values, QTextDocument *fDescr) :
	dlg_commoninput(parent, title, labels, values, fDescr)
{
	QFileInfo info1(fileName);
	m_fileSize = info1.size();

	m_actualSizeLabel = new QLabel("Actual file size: " + QString::number(m_fileSize) + " bytes", this);
	m_actualSizeLabel->setAlignment(Qt::AlignRight);
	gridLayout->addWidget(m_actualSizeLabel, labels.size(), 0, 1, 1);

	m_proposedSizeLabel = new QLabel("Predicted file size: ", this);
	m_proposedSizeLabel->setAlignment(Qt::AlignRight);
	gridLayout->addWidget(m_proposedSizeLabel, labels.size()+1, 0, 1, 1);

	gridLayout->addWidget(buttonBox, labels.size()+2, 0, 1, 1);

	if (!isVolumeStack)   // TODO: not ideal - either load from outside, or only create these boxes here!
	{
		m_extentXIdx = 0; m_extentYIdx = 1; m_extentZIdx = 2; m_voxelSizeIdx = 10;
	}
	else
	{
		m_extentXIdx = 5; m_extentYIdx = 6; m_extentZIdx = 7; m_voxelSizeIdx = 14;
	}
	connect(qobject_cast<QLineEdit*>(m_widgetList[m_extentXIdx]), SIGNAL(textChanged(const QString)), this, SLOT(checkFileSize()));
	connect(qobject_cast<QLineEdit*>(m_widgetList[m_extentYIdx]), SIGNAL(textChanged(const QString)), this, SLOT(checkFileSize()));
	connect(qobject_cast<QLineEdit*>(m_widgetList[m_extentZIdx]), SIGNAL(textChanged(const QString)), this, SLOT(checkFileSize()));
	connect(qobject_cast<QComboBox*>(m_widgetList[m_voxelSizeIdx]), SIGNAL(currentIndexChanged(int)), this, SLOT(checkFileSize()));

	checkFileSize();
}

void dlg_openfile_sizecheck::checkFileSize()
{
	qint64 extentX = getDblValue(m_extentXIdx), extentY= getDblValue(m_extentYIdx), extentZ = getDblValue(m_extentZIdx);
	qint64 voxelSize = mapVTKTypeStringToSize(getComboBoxValue(m_voxelSizeIdx));
	qint64 proposedSize = extentX*extentY*extentZ*voxelSize;
	m_proposedSizeLabel->setText("Predicted file size: " + QString::number(proposedSize) + " bytes");
	m_proposedSizeLabel->setStyleSheet(QString("QLabel { background-color : %1; }").arg(proposedSize == m_fileSize ? "#BFB" : "#FBB" ));
	buttonBox->button(QDialogButtonBox::Ok)->setEnabled(proposedSize == m_fileSize);
}
