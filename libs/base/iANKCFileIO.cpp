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

std::vector<std::shared_ptr<iADataSet>> iANKCFileIO::load(QString const& fileName, iAProgress* progress, QVariantMap const& parameters)
{
	Q_UNUSED(parameters);

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

	iARawFileIO io;
	QVariantMap params;
	params[iARawFileIO::SpacingStr] = variantVector<double>({ 1.0, 1.0, 1.0 });
	params[iARawFileIO::OriginStr] = variantVector<double>({ 0.0, 0.0, 0.0 });
	params[iARawFileIO::SizeStr] = QVariant::fromValue(size);
	params[iARawFileIO::ByteOrderStr] = ByteOrder::BigEndianStr;
	params[iARawFileIO::DataTypeStr] = mapVTKTypeToReadableDataType(VTK_TYPE_UINT16);
	params[iARawFileIO::HeadersizeStr] = file.size() - (2ull * size[0] * size[1]);
	auto d = io.load(fileName, progress, params);

	auto replaceAndShift = iAFilterRegistry::filter("Replace and Shift");
	if (!replaceAndShift)
	{
		LOG(lvlError,
			QString("Reading NKC file %1 requires 'Replace and Shift' filter, but filter could not be found!")
			.arg(fileName));
		return {};
	}
	replaceAndShift->addInput(dynamic_cast<iAImageData*>(d[0].get())->image(), "");
	QVariantMap parametersReplaceAndShift;
	parametersReplaceAndShift["Value To Replace"] = 65533;
	parametersReplaceAndShift["Replacement"] = 0;
	replaceAndShift->run(parametersReplaceAndShift);

	auto dataTypeConversion = iAFilterRegistry::filter("Datatype Conversion");
	if (!dataTypeConversion)
	{
		LOG(lvlError,
			QString("Reading NKC file %1 requires 'Datatype Conversion' filter, but filter could not be found!")
			.arg(fileName));
		return {};
	}

	dataTypeConversion->addInput(replaceAndShift->output(0)->itkImage(), "");
	QVariantMap parametersConversion;
	parametersConversion["Data Type"] = "32 bit floating point number (7 digits, float)";
	parametersConversion["Rescale Range"] = false;
	parametersConversion["Automatic Input Range"] = true;
	parametersConversion["Use Full OutputRange"] = true;
	dataTypeConversion->run(parametersConversion);

	auto filterScale = iAFilterRegistry::filter("Shift and Scale");
	if (!filterScale)
	{
		LOG(lvlError,
			QString("Reading NKC file %1 requires 'Shift and Scale' filter, but filter could not be found!")
			.arg(fileName));
		return {};
	}
	filterScale->addInput(dataTypeConversion->output(0)->itkImage(), "");
	QVariantMap parametersScale;
	parametersScale["Shift"] = offset;
	parametersScale["Scale"] = scale;
	filterScale->run(parametersScale);

	vtkNew<vtkImageData> img;
	img->DeepCopy(filterScale->output(0)->vtkImage());
	return { std::make_shared<iAImageData>(fileName, img) };
}

QString iANKCFileIO::name() const
{
	return Name;
}

QStringList iANKCFileIO::extensions() const
{
	return QStringList{ "nkc" };
}