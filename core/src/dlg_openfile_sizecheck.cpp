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

#include "dlg_commoninput.h"
#include "iAToolsVTK.h"    // for mapVTKTypeStringToSize
#include "iO/iAIO.h"
#include "vtkImageReader.h"

#include <QComboBox>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

dlg_openfile_sizecheck::dlg_openfile_sizecheck(bool isVolumeStack, QString const & fileName, QWidget *parent, QString const & title,
	QStringList const & additionallabels, QList<QVariant> const & additionalvalues, iARawFileParameters & rawFileParams)
{
	QFileInfo info1(fileName);
	m_fileSize = info1.size();


	if (!isVolumeStack)   // TODO: not ideal - either load from outside, or only create these boxes here!
	{
		m_extentXIdx = 0; m_extentYIdx = 1; m_extentZIdx = 2; m_voxelSizeIdx = 10;
	}
	else
	{
		m_extentXIdx = 5; m_extentYIdx = 6; m_extentZIdx = 7; m_voxelSizeIdx = 14;
	}

	QStringList datatype(vtkDataTypeList());
	datatype[mapVTKTypeToIdx(m_rawFileParams.m_scalarType)] = "!" + datatype[mapVTKTypeToIdx(m_rawFileParams.m_scalarType)];
	QStringList byteOrderStr = (QStringList() << tr("Little Endian") << tr("Big Endian"));
	byteOrderStr[mapVTKByteOrderToIdx(m_rawFileParams.m_byteOrder)] = "!" + byteOrderStr[mapVTKByteOrderToIdx(m_rawFileParams.m_byteOrder)];
	QStringList labels = (QStringList()
		<< tr("#Size X") << tr("#Size Y") << tr("#Size Z")
		<< tr("#Spacing X") << tr("#Spacing Y") << tr("#Spacing Z")
		<< tr("#Origin X") << tr("#Origin Y") << tr("#Origin Z")
		<< tr("#Headersize")
		<< tr("+Data Type")
		<< tr("+Byte Order") << additionallabels);

	QList<QVariant> values = (QList<QVariant>()
		<< tr("%1").arg(m_rawFileParams.m_spacing[0]) << tr("%1").arg(m_rawFileParams.m_spacing[1]) << tr("%1").arg(m_rawFileParams.m_spacing[2])
		<< tr("%1").arg(m_rawFileParams.m_spacing[0]) << tr("%1").arg(m_rawFileParams.m_spacing[1]) << tr("%1").arg(m_rawFileParams.m_spacing[2])
		<< tr("%1").arg(m_rawFileParams.m_origin[0]) << tr("%1").arg(m_rawFileParams.m_origin[1]) << tr("%1").arg(m_rawFileParams.m_origin[2])
		<< tr("%1").arg(m_rawFileParams.m_headersize)
		<< datatype
		<< byteOrderStr << additionalvalues);

	m_inputDlg = new dlg_commoninput(parent, "RAW file specs", labels, values);

	m_actualSizeLabel = new QLabel("Actual file size: " + QString::number(m_fileSize) + " bytes", this);
	m_actualSizeLabel->setAlignment(Qt::AlignRight);
	m_inputDlg->gridLayout->addWidget(m_actualSizeLabel, labels.size(), 0, 1, 1);

	m_proposedSizeLabel = new QLabel("Predicted file size: ", this);
	m_proposedSizeLabel->setAlignment(Qt::AlignRight);
	m_inputDlg->gridLayout->addWidget(m_proposedSizeLabel, labels.size() + 1, 0, 1, 1);

	m_inputDlg->gridLayout->addWidget(m_inputDlg->buttonBox, labels.size() + 2, 0, 1, 1);

	connect(qobject_cast<QLineEdit*>(m_inputDlg->widgetList()[m_extentXIdx]), SIGNAL(textChanged(const QString)), this, SLOT(checkFileSize()));
	connect(qobject_cast<QLineEdit*>(m_inputDlg->widgetList()[m_extentYIdx]), SIGNAL(textChanged(const QString)), this, SLOT(checkFileSize()));
	connect(qobject_cast<QLineEdit*>(m_inputDlg->widgetList()[m_extentZIdx]), SIGNAL(textChanged(const QString)), this, SLOT(checkFileSize()));
	connect(qobject_cast<QComboBox*>(m_inputDlg->widgetList()[m_voxelSizeIdx]), SIGNAL(currentIndexChanged(int)), this, SLOT(checkFileSize()));

	checkFileSize();

	if (m_inputDlg->exec() != QDialog::Accepted)
		return false;

	m_rawFileParams.m_spacing[0] = m_inputDlg->getDblValue(0); m_rawFileParams.m_spacing[1] = m_inputDlg->getDblValue(1); m_rawFileParams.m_spacing[2] = m_inputDlg->getDblValue(2);
	m_rawFileParams.m_extent[0] = 0; m_rawFileParams.m_extent[2] = 0; m_rawFileParams.m_extent[4] = 0;
	m_rawFileParams.m_extent[1] = m_rawFileParams.m_spacing[0]; m_rawFileParams.m_extent[3] = m_rawFileParams.m_spacing[1]; m_rawFileParams.m_extent[5] = m_rawFileParams.m_spacing[2];
	m_rawFileParams.m_extent[1]--; m_rawFileParams.m_extent[3]--; m_rawFileParams.m_extent[5]--;

	m_rawFileParams.m_spacing[0] = m_inputDlg->getDblValue(3); m_rawFileParams.m_spacing[1] = m_inputDlg->getDblValue(4); m_rawFileParams.m_spacing[2] = m_inputDlg->getDblValue(5);
	m_rawFileParams.m_spacing[0] = m_rawFileParams.m_spacing[0]; m_rawFileParams.m_spacing[1] = m_rawFileParams.m_spacing[1]; m_rawFileParams.m_spacing[2] = m_rawFileParams.m_spacing[2];

	m_rawFileParams.m_origin[0] = m_inputDlg->getDblValue(6); m_rawFileParams.m_origin[1] = m_inputDlg->getDblValue(7); m_rawFileParams.m_origin[2] = m_inputDlg->getDblValue(8);
	m_rawFileParams.m_origin[0] = m_rawFileParams.m_origin[0]; m_rawFileParams.m_origin[1] = m_rawFileParams.m_origin[1]; m_rawFileParams.m_origin[2] = m_rawFileParams.m_origin[2];

	m_rawFileParams.m_headersize = m_inputDlg->getDblValue(9);
	m_rawFileParams.m_headersize = m_rawFileParams.m_headersize;
	m_rawFileParams.m_fileName = f;
	m_rawFileParams.m_scalarType = mapVTKTypeStringToInt(m_inputDlg->getComboBoxValue(10));
	m_rawFileParams.m_scalarType = m_rawFileParams.m_scalarType;
	if (m_inputDlg->getComboBoxValue(11) == "Little Endian")
		m_rawFileParams.m_byteOrder = VTK_FILE_BYTE_ORDER_LITTLE_ENDIAN;
	else if (m_inputDlg->getComboBoxValue(11) == "Big Endian")
		m_rawFileParams.m_byteOrder = VTK_FILE_BYTE_ORDER_BIG_ENDIAN;

	m_rawFileParams.m_byteOrder = m_rawFileParams.m_byteOrder;
	return true;
}


dlg_openfile_sizecheck::~dlg_openfile_sizecheck()
{
	delete m_inputDlg;
}

void dlg_openfile_sizecheck::checkFileSize()
{
	qint64 extentX = m_inputDlg->getDblValue(m_extentXIdx), extentY= m_inputDlg->getDblValue(m_extentYIdx), extentZ = m_inputDlg->getDblValue(m_extentZIdx);
	qint64 voxelSize = mapVTKTypeStringToSize(m_inputDlg->getComboBoxValue(m_voxelSizeIdx));
	qint64 proposedSize = extentX*extentY*extentZ*voxelSize;
	m_proposedSizeLabel->setText("Predicted file size: " + QString::number(proposedSize) + " bytes");
	m_proposedSizeLabel->setStyleSheet(QString("QLabel { background-color : %1; }").arg(proposedSize == m_fileSize ? "#BFB" : "#FBB" ));
	m_inputDlg->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(proposedSize == m_fileSize);
}
