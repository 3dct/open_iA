// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iACSVtoMHD.h"

#include <defines.h>          // for DIM
#include <iAConnector.h>
#include <iAImageData.h>
#include <iALog.h>
#include <iAProgress.h>
#include <iAToolsVTK.h>
#include <iAVtkDraw.h>

#include <QFile>
#include <QTextStream>

iACSVtoMHD::iACSVtoMHD() : iAFilter("CSV to MHD", "Uncertainty",
	"Read a CSV file and reshape it into an image of the given dimensions", 0, 1)
{
	QStringList pixelTypes (readableDataTypeList(false));
	addParameter("CSV FileName", iAValueType::String, "");
	addParameter("Output fileName", iAValueType::String, "");
	//addParameter("Field separator", iAValueType::String, ";");
	addParameter("Pixel Type", iAValueType::Categorical, pixelTypes);
	addParameter("Size X", iAValueType::Discrete, 1);
	addParameter("Size Y", iAValueType::Discrete, 1);
	addParameter("Size Z", iAValueType::Discrete, 1);
	addParameter("Spacing X", iAValueType::Continuous, 1);
	addParameter("Spacing Y", iAValueType::Continuous, 1);
	addParameter("Spacing Z", iAValueType::Continuous, 1);
	QStringList coordOrders;
	coordOrders
		<< "XYZ"
		<< "ZYX";
	addParameter("Coordinate Order", iAValueType::Categorical, coordOrders);
}

void iACSVtoMHD::performWork(QVariantMap const & parameters)
{
	int dim[3];
	dim[0] = parameters["Size X"].toUInt();
	dim[1] = parameters["Size Y"].toUInt();
	dim[2] = parameters["Size Z"].toUInt();
	double spacing[3];
	spacing[0] = parameters["Spacing X"].toUInt();
	spacing[1] = parameters["Spacing Y"].toUInt();
	spacing[2] = parameters["Spacing Z"].toUInt();
	auto img = allocateImage(mapReadableDataTypeToVTKType(parameters["Pixel Type"].toString()), dim, spacing);

	QString fileName(parameters["CSV FileName"].toString());
	QFile in(fileName);
	if (!in.open(QIODevice::ReadOnly | QIODevice::Text) ||
		!in.isOpen())
	{
		LOG(lvlError, QString("Couldn't open %1 for reading!").arg(fileName));
		return;
	}
	//QString fieldSeparator(parameters["Field separator"].toString());
	QTextStream inStream(&in);
	int curLine = 0;
	int x = 0, y = 0, z = 0;
	while (!inStream.atEnd())
	{
		QString line = inStream.readLine();
		//QStringList tokens = line.split(fieldSeparator);
		bool ok;
		double val = line.toDouble(&ok);
		if (!ok)
		{
			LOG(lvlError, QString("Error converting string '%1' to numeric in %2: %3!").arg(line).arg(fileName).arg(curLine));
		}
		//drawPixel(img, x, y, z, val);
		img->SetScalarComponentFromDouble(x, y, z, 0, val);
		++curLine;
		if (parameters["Coordinate Order"].toString() == "ZYX")
		{
			++z;
			if (z >= dim[2])
			{
				z = 0;
				++y;
				if (y >= dim[1])
				{
					y = 0;
					++x;
					if (x >= dim[0])
					{
						LOG(lvlError, QString("CSV content exceeds given dimensions, stopping conversion at line %1!").arg(curLine));
						break;
					}
				}
			}
		}
		else if (parameters["Coordinate Order"].toString() == "XYZ")
		{
			++x;
			if (x >= dim[0])
			{
				x = 0;
				++y;
				if (y >= dim[1])
				{
					y = 0;
					++z;
					if (z >= dim[2])
					{
						LOG(lvlWarn, QString("CSV content exceeds given dimensions, stopping conversion at line %1!").arg(curLine));
						break;
					}
				}
			}
		}
		else
		{
			LOG(lvlError, "Invalid Coordinate Order.");
			return;
		}
	}
	in.close();
	addOutput(std::make_shared<iAImageData>(img));
	QString outputFileName = parameters["Output fileName"].toString();
	if (!outputFileName.isEmpty())
	{
		storeImage(img, outputFileName);
	}
}
