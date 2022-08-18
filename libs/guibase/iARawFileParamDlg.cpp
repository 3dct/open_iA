/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#include "iAAttributeDescriptor.h"    // for selectOption
#include "iARawFileParameters.h"
#include "iALog.h"
#include "iAToolsVTK.h"    // for mapVTKTypeToReadableDataType, readableDataTypes, ...

#include <QComboBox>
#include <QFileInfo>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QRegularExpression>
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
	iAParameterDlg::ParamListT const& additionalParams, iARawFileParameters& rawFileParams, bool brightTheme) :
	m_accepted(false),
	m_brightTheme(brightTheme)
{
	QFileInfo info1(fileName);
	m_fileSize = info1.size();

	QStringList datatype(readableDataTypeList(false));
	QString selectedType = mapVTKTypeToReadableDataType(rawFileParams.m_scalarType);
	selectOption(datatype, selectedType);
	QStringList byteOrderStr = (QStringList() << tr("Little Endian") << tr("Big Endian"));
	byteOrderStr[mapVTKByteOrderToIdx(rawFileParams.m_byteOrder)] = "!" + byteOrderStr[mapVTKByteOrderToIdx(rawFileParams.m_byteOrder)];
	iAParameterDlg::ParamListT params;
	QVector<int> sizeVec{ static_cast<int>(rawFileParams.m_size[0]), static_cast<int>(rawFileParams.m_size[1]), static_cast<int>(rawFileParams.m_size[2])};
	addParameter(params, "Size", iAValueType::Vector3i, QVariant::fromValue<QVector<int>>(sizeVec));
	QVector<double> spcVec{ rawFileParams.m_spacing[0], rawFileParams.m_spacing[1], rawFileParams.m_spacing[2] };
	addParameter(params, "Spacing", iAValueType::Vector3, QVariant::fromValue<QVector<double>>(spcVec));
	QVector<double> oriVec{ rawFileParams.m_origin[0], rawFileParams.m_origin[1], rawFileParams.m_origin[2] };
	addParameter(params, "Origin", iAValueType::Vector3, QVariant::fromValue<QVector<double>>(oriVec));
	addParameter(params, "Headersize", iAValueType::Discrete, rawFileParams.m_headersize, 0);
	addParameter(params, "Data Type", iAValueType::Categorical, datatype);
	addParameter(params, "Byte Order", iAValueType::Categorical, byteOrderStr);
	params.append(additionalParams);
	m_inputDlg = new iAParameterDlg(parent, title, params);

	auto fileNameLabel = new QLabel(QString("File Name: %1").arg(QFileInfo(fileName).fileName()));
	fileNameLabel->setToolTip(fileName);
	m_inputDlg->layout()->addWidget(fileNameLabel);
	auto guessFromFileNameButton = new QPushButton("Guess parameters from filename");
	m_inputDlg->layout()->addWidget(guessFromFileNameButton);
	connect(guessFromFileNameButton, &QPushButton::pressed, this, [this, fileName]{
		guessParameters(fileName);
	});

	auto actualSizeLabel = new QLabel("Actual file size: " + QString::number(m_fileSize) + " bytes");
	actualSizeLabel->setAlignment(Qt::AlignRight);
	m_inputDlg->layout()->addWidget(actualSizeLabel);

	m_proposedSizeLabel = new QLabel("Predicted file size: ");
	m_proposedSizeLabel->setAlignment(Qt::AlignRight);
	m_inputDlg->layout()->addWidget(m_proposedSizeLabel);

	connect(qobject_cast<QSpinBox*>(m_inputDlg->paramWidget("Size")), QOverload<int>::of(&QSpinBox::valueChanged), this, &iARawFileParamDlg::checkFileSize);
	connect(qobject_cast<QSpinBox*>(m_inputDlg->paramWidget("Headersize")), QOverload<int>::of(&QSpinBox::valueChanged), this, &iARawFileParamDlg::checkFileSize);
	connect(qobject_cast<QComboBox*>(m_inputDlg->paramWidget("Data Type")), QOverload<int>::of(&QComboBox::currentIndexChanged), this, &iARawFileParamDlg::checkFileSize);

	checkFileSize();

	if (m_inputDlg->exec() != QDialog::Accepted)
	{
		return;
	}
	auto values = m_inputDlg->parameterValues();
	auto newSizeVec = values["Size"].value<QVector<int>>();
	auto newSpcVec = values["Spacing"].value<QVector<double>>();
	auto newOriVec = values["Origin"].value<QVector<double>>();
	for (int i = 0; i < 3; ++i)
	{
		rawFileParams.m_size[i] = newSizeVec[i];
		rawFileParams.m_spacing[i] = newSpcVec[i];
		rawFileParams.m_origin[i] = newOriVec[i];
	}
	rawFileParams.m_headersize = values["Headersize"].toULongLong();
	rawFileParams.m_scalarType = mapReadableDataTypeToVTKType(values["Data Type"].toString());
	rawFileParams.m_byteOrder = mapReadableByteOrderToVTKType(values["Byte Order"].toString());
	m_accepted = true;
}

void iARawFileParamDlg::checkFileSize()
{
	auto values = m_inputDlg->parameterValues();
	auto sizeVec = values["Size"].value<QVector<int>>();
	qint64
		sizeX = sizeVec[0],
		sizeY = sizeVec[1],
		sizeZ = sizeVec[2],
		voxelSize = mapVTKTypeToSize(mapReadableDataTypeToVTKType(values["Data Type"].toString())),
		headerSize = values["Headersize"].toLongLong();
	qint64 proposedSize = sizeX * sizeY * sizeZ * voxelSize + headerSize;
	bool valid = !(sizeX <= 0 || sizeY <= 0 || sizeZ <= 0 || voxelSize <= 0 || headerSize < 0);
	m_proposedSizeLabel->setText(valid ?
		"Predicted file size: " + QString::number(proposedSize) + " bytes" :
		"Invalid numbers in size, data type or header size (too large/negative)?");
	m_proposedSizeLabel->setStyleSheet(
		QString("QLabel { background-color : %1; }").arg(proposedSize == m_fileSize ?
			(m_brightTheme ? "#BFB" : "#070"):
			(m_brightTheme ? "#FBB" : "#700") ));
	m_inputDlg->setOKEnabled(proposedSize == m_fileSize);
}

void iARawFileParamDlg::guessParameters(QString fileName)
{
	QRegularExpression sizeRegEx("(\\d+)x(\\d+)x(\\d+)");
	// TODO: also recognize trailing k/K for 1000
	auto sizeMatch = sizeRegEx.match(fileName);
	if (sizeMatch.hasMatch())
	{
		QVector<int> sizeVec{ sizeMatch.captured(1).toInt(), sizeMatch.captured(2).toInt(), sizeMatch.captured(3).toInt()};
		m_inputDlg->setValue("Size", QVariant::fromValue<QVector<int>>(sizeVec));
	}
	QString spcGrp("(\\d+(?:[.-]\\d+)?)(um|mm)");
	QRegularExpression spcRegEx1(QString("%1x%2x%3").arg(spcGrp).arg(spcGrp).arg(spcGrp));
	auto spc1Match = spcRegEx1.match(fileName);
	if (spc1Match.hasMatch())
	{
		QVector<double> spcVec{ spc1Match.captured(1).replace("-", ".").toDouble(), spc1Match.captured(3).replace("-", ".").toDouble(),  spc1Match.captured(5).replace("-", ".").toDouble() };
		m_inputDlg->setValue("Spacing", QVariant::fromValue<QVector<double>>(spcVec));
	}
	else
	{
		QRegularExpression spcRegEx2(spcGrp);
		auto spc2Match = spcRegEx2.match(fileName);
		if (spc2Match.hasMatch())
		{
			m_inputDlg->setValue("Spacing X", spc2Match.captured(1).replace("-", "."));
			m_inputDlg->setValue("Spacing Y", spc2Match.captured(1).replace("-", "."));
			m_inputDlg->setValue("Spacing Z", spc2Match.captured(1).replace("-", "."));
		}
	}
	QRegularExpression scalarTypeRegEx("(\\d+)bit");
	auto scalarTypeMatch = scalarTypeRegEx.match(fileName);
	if (scalarTypeMatch.hasMatch())
	{
		auto bits = scalarTypeMatch.captured(1);
		int vtkTypeID = (bits == "8") ? VTK_UNSIGNED_CHAR
			: (bits == "16")          ? VTK_UNSIGNED_SHORT
			: (bits == "32")          ? VTK_UNSIGNED_INT
			: (bits == "64")          ? VTK_UNSIGNED_LONG_LONG
									  : -1;
		if (vtkTypeID == -1)
		{
			LOG(lvlWarn, QString("Invalid bits string %1 cannot be resolved to a type").arg(bits));
		}
		else
		{
			QString scalarTypeStr = mapVTKTypeToReadableDataType(vtkTypeID);
			m_inputDlg->setValue("Data Type", scalarTypeStr);
		}
	}
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
