/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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
#include "iARawFileParamDlg.h"

#include "io/iARawFileParameters.h"
#include "iAToolsVTK.h"    // for mapVTKTypeToReadableDataType, readableDataTypes, ...

#include <QComboBox>
#include <QFileInfo>
#include <QLabel>
#include <QLayout>
#include <QSpinBox>

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

iARawFileParamDlg::iARawFileParamDlg(QString const& fileName, QWidget* parent, QString const& title,
	iAParameterDlg::ParamListT const& additionalParams, iARawFileParameters& rawFileParams) :
	m_accepted(false)
{
	QFileInfo info1(fileName);
	m_fileSize = info1.size();

	QStringList datatype(readableDataTypeList(false));
	QString selectedType = mapVTKTypeToReadableDataType(rawFileParams.m_scalarType);
	selectOption(datatype, selectedType);
	QStringList byteOrderStr = (QStringList() << tr("Little Endian") << tr("Big Endian"));
	byteOrderStr[mapVTKByteOrderToIdx(rawFileParams.m_byteOrder)] = "!" + byteOrderStr[mapVTKByteOrderToIdx(rawFileParams.m_byteOrder)];
	iAParameterDlg::ParamListT params;
	addParameter(params, "Size X", iAValueType::Discrete, rawFileParams.m_size[0], 1);
	addParameter(params, "Size Y", iAValueType::Discrete, rawFileParams.m_size[1], 1);
	addParameter(params, "Size Z", iAValueType::Discrete, rawFileParams.m_size[2], 1);
	addParameter(params, "Spacing X", iAValueType::Continuous, rawFileParams.m_spacing[0]);
	addParameter(params, "Spacing Y", iAValueType::Continuous, rawFileParams.m_spacing[1]);
	addParameter(params, "Spacing Z", iAValueType::Continuous, rawFileParams.m_spacing[2]);
	addParameter(params, "Origin X", iAValueType::Continuous, rawFileParams.m_origin[0]);
	addParameter(params, "Origin Y", iAValueType::Continuous, rawFileParams.m_origin[1]);
	addParameter(params, "Origin Z", iAValueType::Continuous, rawFileParams.m_origin[2]);
	addParameter(params, "Headersize", iAValueType::Discrete, rawFileParams.m_headersize, 0);
	addParameter(params, "Data Type", iAValueType::Categorical, datatype);
	addParameter(params, "Byte Order", iAValueType::Categorical, byteOrderStr);
	params.append(additionalParams);
	m_inputDlg = new iAParameterDlg(parent, title, params);

	m_actualSizeLabel = new QLabel("Actual file size: " + QString::number(m_fileSize) + " bytes");
	m_actualSizeLabel->setAlignment(Qt::AlignRight);
	m_inputDlg->layout()->addWidget(m_actualSizeLabel);

	m_proposedSizeLabel = new QLabel("Predicted file size: ");
	m_proposedSizeLabel->setAlignment(Qt::AlignRight);
	m_inputDlg->layout()->addWidget(m_proposedSizeLabel);
	// indices here refer to the order that the items are inserted in above!
	connect(qobject_cast<QSpinBox*>(m_inputDlg->widgetList()[0]), QOverload<int>::of(&QSpinBox::valueChanged), this, &iARawFileParamDlg::checkFileSize);
	connect(qobject_cast<QSpinBox*>(m_inputDlg->widgetList()[1]), QOverload<int>::of(&QSpinBox::valueChanged), this, &iARawFileParamDlg::checkFileSize);
	connect(qobject_cast<QSpinBox*>(m_inputDlg->widgetList()[2]), QOverload<int>::of(&QSpinBox::valueChanged), this, &iARawFileParamDlg::checkFileSize);
	connect(qobject_cast<QSpinBox*>(m_inputDlg->widgetList()[9]), QOverload<int>::of(&QSpinBox::valueChanged), this, &iARawFileParamDlg::checkFileSize);
	connect(qobject_cast<QComboBox*>(m_inputDlg->widgetList()[10]), QOverload<int>::of(&QComboBox::currentIndexChanged), this, &iARawFileParamDlg::checkFileSize);

	checkFileSize();

	if (m_inputDlg->exec() != QDialog::Accepted)
	{
		return;
	}
	auto values = m_inputDlg->parameterValues();
	rawFileParams.m_size[0] = values["Size X"].toInt();
	rawFileParams.m_size[1] = values["Size Y"].toInt();
	rawFileParams.m_size[2] = values["Size Z"].toInt();
	rawFileParams.m_spacing[0] = values["Spacing X"].toDouble();
	rawFileParams.m_spacing[1] = values["Spacing Y"].toDouble();
	rawFileParams.m_spacing[2] = values["Spacing Z"].toDouble();
	rawFileParams.m_origin[0] = values["Origin X"].toDouble();
	rawFileParams.m_origin[1] = values["Origin Y"].toDouble();
	rawFileParams.m_origin[2] = values["Origin Z"].toDouble();
	rawFileParams.m_headersize = values["Headersize"].toULongLong();
	rawFileParams.m_scalarType = mapReadableDataTypeToVTKType(values["Data Type"].toString());
	if (values["Byte Order"].toString() == "Little Endian")
	{
		rawFileParams.m_byteOrder = VTK_FILE_BYTE_ORDER_LITTLE_ENDIAN;
	}
	else // if (values["Byte Order"].toString() == "Big Endian")
	{
		rawFileParams.m_byteOrder = VTK_FILE_BYTE_ORDER_BIG_ENDIAN;
	}
	m_accepted = true;
}

void iARawFileParamDlg::checkFileSize()
{
	auto values = m_inputDlg->parameterValues();
	qint64
		sizeX = values["Size X"].toLongLong(),
		sizeY = values["Size Y"].toLongLong(),
		sizeZ = values["Size Z"].toLongLong(),
		voxelSize = mapVTKTypeToSize(mapReadableDataTypeToVTKType(values["Data Type"].toString())),
		headerSize = values["Headersize"].toLongLong();
	qint64 proposedSize = sizeX * sizeY * sizeZ * voxelSize + headerSize;
	bool valid = !(sizeX <= 0 || sizeY <= 0 || sizeZ <= 0 || voxelSize <= 0 || headerSize < 0);
	m_proposedSizeLabel->setText(valid ?
		"Predicted file size: " + QString::number(proposedSize) + " bytes" :
		"Invalid numbers in size, data type or header size (too large/negative)?");
	m_proposedSizeLabel->setStyleSheet(
		QString("QLabel { background-color : %1; }").arg(proposedSize == m_fileSize ? "#BFB" : "#FBB"));
	m_inputDlg->setOKEnabled(proposedSize == m_fileSize);
}

bool iARawFileParamDlg::accepted() const
{
	return m_accepted;
}

QMap<QString, QVariant> iARawFileParamDlg::parameterValues() const
{
	return m_inputDlg->parameterValues();
}

iARawFileParamDlg::~iARawFileParamDlg()
{
	delete m_inputDlg;
}
