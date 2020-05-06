/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "io/iARawFileParameters.h"
#include "iAToolsVTK.h"    // for mapVTKTypeToReadableDataType, readableDataTypes, ...

#include <vtkImageReader.h>  // for VTK_FILE_BYTE_ORDER_... constants

#include <QComboBox>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

#include <cassert>

namespace
{
	unsigned int mapVTKByteOrderToIdx(unsigned int vtkByteOrder)
	{
		switch (vtkByteOrder)
		{
		default:
		case VTK_FILE_BYTE_ORDER_LITTLE_ENDIAN: return 0;
		case VTK_FILE_BYTE_ORDER_BIG_ENDIAN: return 1;
		}
	}
}

dlg_openfile_sizecheck::dlg_openfile_sizecheck(QString const & fileName, QWidget *parent, QString const & title,
	QStringList const & additionalLabels, QList<QVariant> const & additionalValues, iARawFileParameters & rawFileParams):
	m_accepted(false)
{
	QFileInfo info1(fileName);
	m_fileSize = info1.size();
	m_sizeXIdx = 0; m_sizeYIdx = 1; m_sizeZIdx = 2; m_headerSizeIdx = 9; m_voxelSizeIdx = 10;

	QStringList datatype(readableDataTypeList(false));
	QString selectedType = mapVTKTypeToReadableDataType(rawFileParams.m_scalarType);
	int selectedIdx = datatype.indexOf(selectedType);
	if (selectedIdx != -1)
		datatype[selectedIdx] = "!" + datatype[selectedIdx];
	QStringList byteOrderStr = (QStringList() << tr("Little Endian") << tr("Big Endian"));
	byteOrderStr[mapVTKByteOrderToIdx(rawFileParams.m_byteOrder)] = "!" + byteOrderStr[mapVTKByteOrderToIdx(rawFileParams.m_byteOrder)];
	QStringList labels = (QStringList()
		<< tr("#Size X") << tr("#Size Y") << tr("#Size Z")
		<< tr("#Spacing X") << tr("#Spacing Y") << tr("#Spacing Z")
		<< tr("#Origin X") << tr("#Origin Y") << tr("#Origin Z")
		<< tr("#Headersize")
		<< tr("+Data Type")
		<< tr("+Byte Order"));
	m_fixedParams = labels.size();
	labels.append(additionalLabels);

	QList<QVariant> values = (QList<QVariant>()
		<< rawFileParams.m_size[0]    << rawFileParams.m_size[1]    << rawFileParams.m_size[2]
		<< rawFileParams.m_spacing[0] << rawFileParams.m_spacing[1] << rawFileParams.m_spacing[2]
		<< rawFileParams.m_origin[0]  << rawFileParams.m_origin[1]  << rawFileParams.m_origin[2]
		<< rawFileParams.m_headersize
		<< datatype
		<< byteOrderStr
		<< additionalValues);

	m_inputDlg = new dlg_commoninput(parent, title, labels, values);

	m_actualSizeLabel = new QLabel("Actual file size: " + QString::number(m_fileSize) + " bytes");
	m_actualSizeLabel->setAlignment(Qt::AlignRight);
	m_inputDlg->gridLayout->addWidget(m_actualSizeLabel, labels.size(), 0, 1, 1);

	m_proposedSizeLabel = new QLabel("Predicted file size: ");
	m_proposedSizeLabel->setAlignment(Qt::AlignRight);
	m_inputDlg->gridLayout->addWidget(m_proposedSizeLabel, labels.size() + 1, 0, 1, 1);

	m_inputDlg->gridLayout->addWidget(m_inputDlg->buttonBox, labels.size() + 2, 0, 1, 1);

	connect(qobject_cast<QLineEdit*>(m_inputDlg->widgetList()[m_sizeXIdx])     , &QLineEdit::textChanged, this, &dlg_openfile_sizecheck::checkFileSize);
	connect(qobject_cast<QLineEdit*>(m_inputDlg->widgetList()[m_sizeYIdx])     , &QLineEdit::textChanged, this, &dlg_openfile_sizecheck::checkFileSize);
	connect(qobject_cast<QLineEdit*>(m_inputDlg->widgetList()[m_sizeZIdx])     , &QLineEdit::textChanged, this, &dlg_openfile_sizecheck::checkFileSize);
	connect(qobject_cast<QLineEdit*>(m_inputDlg->widgetList()[m_headerSizeIdx]), &QLineEdit::textChanged, this, &dlg_openfile_sizecheck::checkFileSize);
	connect(qobject_cast<QComboBox*>(m_inputDlg->widgetList()[m_voxelSizeIdx]), QOverload<int>::of(&QComboBox::currentIndexChanged), this, &dlg_openfile_sizecheck::checkFileSize);

	checkFileSize();

	if (m_inputDlg->exec() != QDialog::Accepted)
	{
		return;
	}

	for (int i = 0; i < 3; ++i)
	{
		rawFileParams.m_size[i] = m_inputDlg->getIntValue(i);
		rawFileParams.m_spacing[i] = m_inputDlg->getDblValue(3 + i);
		rawFileParams.m_origin[i] = m_inputDlg->getDblValue(6 + i);
	}
	rawFileParams.m_headersize = m_inputDlg->getDblValue(9);
	rawFileParams.m_scalarType = mapReadableDataTypeToVTKType(m_inputDlg->getComboBoxValue(10));
	if (m_inputDlg->getComboBoxValue(11) == "Little Endian")
		rawFileParams.m_byteOrder = VTK_FILE_BYTE_ORDER_LITTLE_ENDIAN;
	else if (m_inputDlg->getComboBoxValue(11) == "Big Endian")
		rawFileParams.m_byteOrder = VTK_FILE_BYTE_ORDER_BIG_ENDIAN;
	m_accepted = true;
}

void dlg_openfile_sizecheck::checkFileSize()
{
	qint64 sizeX = m_inputDlg->getDblValue(m_sizeXIdx),
		sizeY= m_inputDlg->getDblValue(m_sizeYIdx),
		sizeZ = m_inputDlg->getDblValue(m_sizeZIdx),
		voxelSize = mapVTKTypeToSize(mapReadableDataTypeToVTKType(m_inputDlg->getComboBoxValue(m_voxelSizeIdx))),
		headerSize = m_inputDlg->getDblValue(m_headerSizeIdx);
	qint64 proposedSize = sizeX * sizeY * sizeZ * voxelSize + headerSize;
	if (sizeX <= 0 || sizeY <= 0 || sizeZ <= 0 || voxelSize <= 0 || headerSize < 0)
	{
		m_proposedSizeLabel->setText("Invalid numbers in size, data type or header size (too large/negative)?");
	}
	else
	{
		m_proposedSizeLabel->setText("Predicted file size: " + QString::number(proposedSize) + " bytes");
	}
	m_proposedSizeLabel->setStyleSheet(QString("QLabel { background-color : %1; }").arg(proposedSize == m_fileSize ? "#BFB" : "#FBB" ));
	m_inputDlg->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(proposedSize == m_fileSize);
}

bool dlg_openfile_sizecheck::accepted() const
{
	return m_accepted;
}

int dlg_openfile_sizecheck::fixedParams() const
{
	return m_fixedParams;
}

dlg_commoninput const * dlg_openfile_sizecheck::inputDlg() const
{
	return m_inputDlg;
}

dlg_openfile_sizecheck::~dlg_openfile_sizecheck()
{
	delete m_inputDlg;
}
