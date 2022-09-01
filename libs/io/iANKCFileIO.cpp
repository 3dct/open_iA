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
#include "iANKCFileIO.h"

#include "iAConnector.h"
#include "iAFilter.h"
#include "iAFilterRegistry.h"
#include "iARawFileIO.h"
#include "iAToolsVTK.h"    // for ByteOrder
#include "iAValueTypeVectorHelpers.h"

#include <vtkImageData.h>

#include <QFile>
#include <QRegularExpression>
#include <QTextStream>

const QString iANKCFileIO::Name("NKC files");

iANKCFileIO::iANKCFileIO() : iAFileIO(iADataSetType::Volume, iADataSetType::None)
{
}

std::vector<std::shared_ptr<iADataSet>> iANKCFileIO::loadData(QString const& fileName, QVariantMap const& paramValues, iAProgress* progress)
{
	Q_UNUSED(paramValues);

	QFile file(fileName);
	file.open(QFile::ReadOnly | QFile::Text);
	QTextStream in(&file);
	auto text = in.readAll();

	QVector<int> size(3);
	QRegularExpression regexColumns("number of columns : (\\d*)\\D");
	QRegularExpressionMatch matchColumns = regexColumns.match(text);
	if (matchColumns.hasMatch())
	{
		QString columns = matchColumns.captured(1);
		size[0] = columns.toInt();
	}
	QRegularExpression regexRows("number of raws : (\\d*)\\D");
	QRegularExpressionMatch matchRows = regexRows.match(text);
	if (matchRows.hasMatch())
	{
		QString rows = matchRows.captured(1);
		size[1] = rows.toInt();
	}
	size[2] = 1;

	int offset = 0;
	QRegularExpression regexOffset("value offset : (\\d*)\\D");
	QRegularExpressionMatch matchOffset = regexOffset.match(text);
	if (matchOffset.hasMatch())
	{
		QString Offset = matchOffset.captured(1);
		offset = Offset.toInt();
	}

	float scale = 1;
	QRegularExpression regexScale("value coefficient : (\\d.\\d*E?-?\\d?)\\D");
	QRegularExpressionMatch matchScale = regexScale.match(text);
	if (matchScale.hasMatch())
	{
		QString scaleStr = matchScale.captured(1);
		scale = scaleStr.toFloat();
	}

	QString PixelValue = "";
	QRegularExpression regexPixelValuee("c-scan unit : (dB|%)\\D");
	QRegularExpressionMatch matchPixelValue = regexPixelValuee.match(text);
	if (matchScale.hasMatch())
	{
		PixelValue = matchPixelValue.captured(1);
	}

	iARawFileIO io;
	QVariantMap rawFileParamValues;
	rawFileParamValues[iARawFileIO::SpacingStr] = variantVector<double>({ 1.0, 1.0, 1.0 });
	rawFileParamValues[iARawFileIO::OriginStr] = variantVector<double>({ 0.0, 0.0, 0.0 });
	rawFileParamValues[iARawFileIO::SizeStr] = QVariant::fromValue(size);
	rawFileParamValues[iARawFileIO::ByteOrderStr] = ByteOrder::BigEndianStr;
	rawFileParamValues[iARawFileIO::DataTypeStr] = mapVTKTypeToReadableDataType(VTK_TYPE_UINT16);
	rawFileParamValues[iARawFileIO::HeadersizeStr] = file.size() - (2ull * size[0] * size[1]);
	auto d = io.load(fileName, rawFileParamValues, progress);

	auto replaceAndShift = iAFilterRegistry::filter("Replace and Shift");
	if (!replaceAndShift)
	{
		LOG(lvlError,
			QString("Reading NKC file %1 requires 'Replace and Shift' filter, but filter could not be found!")
			.arg(fileName));
		return {};
	}
	replaceAndShift->addInput(d[0]);
	QVariantMap paramValuesReplaceAndShift;
	paramValuesReplaceAndShift["Value To Replace"] = 65533;
	paramValuesReplaceAndShift["Replacement"] = 0;
	replaceAndShift->run(paramValuesReplaceAndShift);

	auto dataTypeConversion = iAFilterRegistry::filter("Datatype Conversion");
	if (!dataTypeConversion)
	{
		LOG(lvlError,
			QString("Reading NKC file %1 requires 'Datatype Conversion' filter, but filter could not be found!")
			.arg(fileName));
		return {};
	}

	dataTypeConversion->addInput(replaceAndShift->output(0));
	QVariantMap paramValuesConversion;
	paramValuesConversion["Data Type"] = "32 bit floating point number (7 digits, float)";
	paramValuesConversion["Rescale Range"] = false;
	paramValuesConversion["Automatic Input Range"] = true;
	paramValuesConversion["Use Full OutputRange"] = true;
	dataTypeConversion->run(paramValuesConversion);

	auto filterScale = iAFilterRegistry::filter("Shift and Scale");
	if (!filterScale)
	{
		LOG(lvlError,
			QString("Reading NKC file %1 requires 'Shift and Scale' filter, but filter could not be found!")
			.arg(fileName));
		return {};
	}
	filterScale->addInput(dataTypeConversion->output(0));
	QVariantMap paramValuesScale;
	paramValuesScale["Shift"] = offset;
	paramValuesScale["Scale"] = scale;
	filterScale->run(paramValuesScale);

	auto dataSet = filterScale->output(0);
	dataSet->setMetaData("PixelValueFormat", PixelValue);
	return { dataSet };
}

QString iANKCFileIO::name() const
{
	return Name;
}

QStringList iANKCFileIO::extensions() const
{
	return QStringList{"nkc"};
}

bool iANKCFileIO::s_bRegistered = iAFileTypeRegistry::addFileType<iANKCFileIO>();