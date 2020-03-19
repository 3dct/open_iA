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
#include "iACSVtoMHD.h"

#include <defines.h>          // for DIM
#include <iAConnector.h>
#include <iAConsole.h>
#include <iAProgress.h>
#include <iAToolsVTK.h>
#include <iAVtkDraw.h>
#include <mdichild.h>

#include <vtkImageData.h>

#include <QTextStream>

iACSVtoMHD::iACSVtoMHD() : iAFilter("CSV to MHD", "Uncertainty",
	"Read a CSV file and reshape it into an image of the given dimensions", 0, 1)
{
	QStringList pixelTypes (readableDataTypeList(false));
	addParameter("CSV FileName", String, "");
	addParameter("Output fileName", String, "");
	//addParameter("Field separator", String, ";");
	addParameter("Pixel Type", Categorical, pixelTypes);
	addParameter("Size X", Discrete, 1);
	addParameter("Size Y", Discrete, 1);
	addParameter("Size Z", Discrete, 1);
	addParameter("Spacing X", Continuous, 1);
	addParameter("Spacing Y", Continuous, 1);
	addParameter("Spacing Z", Continuous, 1);
	QStringList coordOrders;
	coordOrders
		<< "XYZ"
		<< "ZYX";
	addParameter("Coordinate Order", Categorical, coordOrders);
}

IAFILTER_CREATE(iACSVtoMHD)

void iACSVtoMHD::performWork(QMap<QString, QVariant> const & parameters)
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
		DEBUG_LOG(QString("Couldn't open %1 for reading!").arg(fileName));
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
			DEBUG_LOG(QString("Error converting string '%1' to numeric in %2: %3!").arg(line).arg(fileName).arg(curLine));
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
						DEBUG_LOG(QString("CSV content exceeds given dimensions, stopping conversion at line %1!").arg(curLine));
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
						DEBUG_LOG(QString("CSV content exceeds given dimensions, stopping conversion at line %1!").arg(curLine));
						break;
					}
				}
			}
		}
		else
		{
			DEBUG_LOG("Invalid Coordinate Order.");
			return;
		}
	}
	in.close();
	addOutput(img);
	QString outputFileName = parameters["Output fileName"].toString();
	if (!outputFileName.isEmpty())
	{
		storeImage(img, outputFileName);
	}
}
