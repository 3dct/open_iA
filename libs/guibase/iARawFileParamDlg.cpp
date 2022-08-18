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
#include "iARawFileIO.h"
#include "iALog.h"
#include "iAToolsVTK.h"    // for mapVTKTypeToReadableDataType, readableDataTypes, ...

#include "iAVectorInput.h"

#include <QComboBox>
#include <QFileInfo>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QRegularExpression>
#include <QSpinBox>

iARawFileParamDlg::iARawFileParamDlg(QString const& fileName, QWidget* parent, QString const& title,
	iAParameterDlg::ParamListT const& additionalParams, QMap<QString, QVariant> & paramValues, bool brightTheme) :
	m_accepted(false),
	m_brightTheme(brightTheme)
{
	QFileInfo info1(fileName);
	m_fileSize = info1.size();

	QStringList datatypeList(readableDataTypeList(false));
	selectOption(datatypeList, paramValues[iARawFileIO::DataTypeStr].toString());
	QStringList byteOrderList(ByteOrder::stringList());
	selectOption(byteOrderList, paramValues[iARawFileIO::ByteOrderStr].toString());
	iAParameterDlg::ParamListT params;
	addParameter(params, iARawFileIO::SizeStr, iAValueType::Vector3i, paramValues[iARawFileIO::SizeStr]);
	addParameter(params, iARawFileIO::SpacingStr, iAValueType::Vector3, paramValues[iARawFileIO::SpacingStr]);
	addParameter(params, iARawFileIO::OriginStr, iAValueType::Vector3, paramValues[iARawFileIO::OriginStr]);
	addParameter(params, iARawFileIO::HeadersizeStr, iAValueType::Discrete, paramValues[iARawFileIO::HeadersizeStr].toInt(), 0);
	addParameter(params, iARawFileIO::DataTypeStr, iAValueType::Categorical, datatypeList);
	addParameter(params, iARawFileIO::ByteOrderStr, iAValueType::Categorical, byteOrderList);
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

	connect(qobject_cast<iAVectorInput*>(m_inputDlg->paramWidget(iARawFileIO::SizeStr)), &iAVectorInput::valueChanged, this, &iARawFileParamDlg::checkFileSize);
	connect(qobject_cast<QSpinBox*>(m_inputDlg->paramWidget(iARawFileIO::HeadersizeStr)), QOverload<int>::of(&QSpinBox::valueChanged), this, &iARawFileParamDlg::checkFileSize);
	connect(qobject_cast<QComboBox*>(m_inputDlg->paramWidget(iARawFileIO::DataTypeStr)), QOverload<int>::of(&QComboBox::currentIndexChanged), this, &iARawFileParamDlg::checkFileSize);

	checkFileSize();

	if (m_inputDlg->exec() != QDialog::Accepted)
	{
		return;
	}
	auto newValues = m_inputDlg->parameterValues(); // maybe we can directly assign to paramValues?
	paramValues[iARawFileIO::SizeStr] = newValues[iARawFileIO::SizeStr];
	paramValues[iARawFileIO::SpacingStr] = newValues[iARawFileIO::SpacingStr];
	paramValues[iARawFileIO::OriginStr] = newValues[iARawFileIO::OriginStr];
	paramValues[iARawFileIO::HeadersizeStr] = newValues[iARawFileIO::HeadersizeStr];
	paramValues[iARawFileIO::DataTypeStr] = newValues[iARawFileIO::DataTypeStr];
	paramValues[iARawFileIO::ByteOrderStr] = newValues[iARawFileIO::ByteOrderStr];
	m_accepted = true;
}

void iARawFileParamDlg::checkFileSize()
{
	auto values = m_inputDlg->parameterValues();
	auto sizeVec = values[iARawFileIO::SizeStr].value<QVector<int>>();
	qint64
		sizeX = sizeVec[0],
		sizeY = sizeVec[1],
		sizeZ = sizeVec[2],
		voxelSize = mapVTKTypeToSize(mapReadableDataTypeToVTKType(values["Data Type"].toString())),
		headerSize = values[iARawFileIO::HeadersizeStr].toLongLong();
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
		m_inputDlg->setValue(iARawFileIO::SizeStr, QVariant::fromValue<QVector<int>>(sizeVec));
	}
	QString spcGrp("(\\d+(?:[.-]\\d+)?)(um|mm)");
	QRegularExpression spcRegEx1(QString("%1x%2x%3").arg(spcGrp).arg(spcGrp).arg(spcGrp));
	auto spc1Match = spcRegEx1.match(fileName);
	if (spc1Match.hasMatch())
	{
		QVector<double> spcVec{ spc1Match.captured(1).replace("-", ".").toDouble(), spc1Match.captured(3).replace("-", ".").toDouble(), spc1Match.captured(5).replace("-", ".").toDouble() };
		m_inputDlg->setValue(iARawFileIO::SpacingStr, QVariant::fromValue<QVector<double>>(spcVec));
	}
	else
	{
		QRegularExpression spcRegEx2(spcGrp);
		auto spc2Match = spcRegEx2.match(fileName);
		if (spc2Match.hasMatch())
		{

			QVector<double> spcVec{ spc2Match.captured(1).replace("-", ".").toDouble(), spc2Match.captured(1).replace("-", ".").toDouble(), spc2Match.captured(1).replace("-", ".").toDouble() };
			m_inputDlg->setValue(iARawFileIO::SpacingStr, QVariant::fromValue<QVector<double>>(spcVec));
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
			m_inputDlg->setValue(iARawFileIO::DataTypeStr, scalarTypeStr);
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
