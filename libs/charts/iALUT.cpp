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

#include "iALUT.h"

#include "iAColorTheme.h"
#include "iALog.h"
#include "iALookupTable.h"
#include "iAVtkVersion.h"    // required for VTK < 9.0

#include <QColor>
#include <QDirIterator>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <vtkColorTransferFunction.h>
#include <vtkLookupTable.h>
#include <vtkPiecewiseFunction.h>

QMap<QString, vtkSmartPointer<vtkColorTransferFunction>> iALUT::m_colorMaps;

QStringList iALUT::colorMapNames()
{
	return m_colorMaps.keys();
}

bool jsonValToDouble(QJsonValue const & v, double & d)
{
	if (v.isDouble())
	{
		d = v.toDouble();
		return true;
	}
	if (v.isString())
	{
		bool ok;
		d = v.toString().toDouble(&ok);
		if (ok)
		{
			return true;
		}
	}
	return false;
}

void iALUT::loadMaps(QString const& folder)
{
	QDir::Filters filters = QDir::Files;
	QDirIterator::IteratorFlags flags = QDirIterator::NoIteratorFlags;
	QStringList nameFilters;
	nameFilters << "*.json";
	QDirIterator it(folder, nameFilters, filters, flags);
	while (it.hasNext())
	{
		QString fileName = it.next();
		QFile jsonFile(fileName);
		jsonFile.open(QIODevice::ReadOnly | QIODevice::Text);
		auto jsonContent = jsonFile.readAll();
		jsonFile.close();
		QJsonDocument d = QJsonDocument::fromJson(jsonContent);
		auto rootObj = d.object();
		auto jsonName = rootObj.value("name");
		if (!jsonName.isString())
		{
			LOG(lvlWarn,
				QString("Invalid color map in file %1: name %2 is not a string!")
					.arg(fileName)
					.arg(jsonName.toString()));
			continue;
		}
		auto name = jsonName.toString();

		double colorRange = 0;
		if (!jsonValToDouble(rootObj.value("colorRange"), colorRange) )
		{
			LOG(lvlWarn,
				QString("Invalid color map in file %1: colorRange %2 is not a integer/double value!")
					.arg(fileName)
					.arg(rootObj.value("colorRange").toString()));
			continue;
		}

		auto jsonColors = rootObj.value("colors");
		if (!jsonColors.isArray())
		{
			LOG(lvlWarn,
				QString("Invalid color map in file %1: colors %2 is not an array!")
					.arg(fileName)
					.arg(jsonColors.toString()));
			continue;
		}
		auto jsonColorsArray = jsonColors.toArray();

		auto ctf = vtkSmartPointer<vtkColorTransferFunction>::New();
		ctf->SetColorSpaceToLab();
		bool validColors = true;
		for (int c=0; c<jsonColorsArray.size(); ++c)
		{
			double rgb[3];
			if (!jsonColorsArray.at(c).isArray() || jsonColorsArray.at(c).toArray().size() != 3)
			{
				LOG(lvlWarn,
					QString("Invalid color map in file %1: color with index %2 is not an array, or does not have expected size 3!")
						.arg(fileName)
						.arg(jsonColors.toString()));
				validColors = false;
				break;
			}
			auto jsonRGB = jsonColorsArray.at(c).toArray();
			for (int i = 0; i < 3; ++i)
			{

				if (!jsonValToDouble(jsonRGB.at(i), rgb[i]))
				{
					LOG(lvlWarn,
						QString("Invalid color in file %1: color component %2 of color %3 (%4) is not a double!")
							.arg(fileName)
							.arg(i)
							.arg(c)
							.arg(jsonRGB.at(i).toString()));
					validColors = false;
					break;
				}
				rgb[i] /= colorRange;
			}
			ctf->AddRGBPoint(static_cast<double>(c) / (jsonColorsArray.size() - 1), rgb[0], rgb[1], rgb[2]);
		}
		if (validColors)
		{
			m_colorMaps[name] = ctf;
		}
	}
}

int iALUT::BuildLUT( vtkSmartPointer<vtkLookupTable> pLUT, double const * lutRange, QString colorMap, int numCols /*= 256 */)
{
	QColor c;
	if (!m_colorMaps.contains(colorMap))
	{
		LOG(lvlWarn, QString("Invalid color map name %1!").arg(colorMap));
		return 0;
	}
	auto ctf = m_colorMaps[colorMap];
#if VTK_VERSION_NUMBER <= VTK_VERSION_CHECK(8, 0, 0)
	double lutRangeNonConst[2];
	std::copy(lutRange, lutRange + 2, lutRangeNonConst);
	pLUT->SetRange(lutRangeNonConst);
	pLUT->SetTableRange(lutRangeNonConst);
#else
	pLUT->SetRange( lutRange );
	pLUT->SetTableRange( lutRange );
#endif
	pLUT->SetNumberOfColors( numCols );
	for( int i = 0; i < numCols; ++i )
	{
		double rgb[3];
		ctf->GetColor( static_cast<double>(i) / numCols, rgb );
		pLUT->SetTableValue( i, rgb[0], rgb[1], rgb[2] );
	}
	pLUT->Build();
	return ctf->GetSize();
}

int iALUT::BuildLUT( vtkSmartPointer<vtkLookupTable> pLUT, double rangeFrom, double rangeTo, QString colorMap, int numCols /*= 256 */)
{
	double lutRange[2] = { rangeFrom, rangeTo };
	return BuildLUT( pLUT, lutRange, colorMap, numCols);
}

iALookupTable iALUT::Build(double const * lutRange, QString colorMap, int numCols, double /*alpha*/)
{
	vtkSmartPointer<vtkLookupTable> vtkLUT(vtkSmartPointer<vtkLookupTable>::New());
	BuildLUT(vtkLUT, lutRange, colorMap, numCols);
	iALookupTable result(vtkLUT);
	return result;
}

vtkSmartPointer<vtkPiecewiseFunction> iALUT::BuildLabelOpacityTF(int labelCount)
{
	auto result = vtkSmartPointer<vtkPiecewiseFunction>::New();
	result->AddPoint(0.0, 0.0);
	for (int i = 0; i < labelCount; ++i)
	{
		result->AddPoint(i + 1, 1.0);
	}
	return result;
}

vtkSmartPointer<vtkLookupTable> iALUT::BuildLabelColorTF(int labelCount, iAColorTheme const * colorTheme)
{
	auto result = vtkSmartPointer<vtkLookupTable>::New();
	result->SetNumberOfTableValues(labelCount + 1);
	result->SetRange(0, labelCount);
	result->SetTableValue(0.0, 0.0, 0.0, 0.0, 0.0);   // value 0 is transparent
	for (int i = 0; i < labelCount; ++i)
	{
		QColor c(colorTheme->color(i));
		result->SetTableValue(i + 1,
			c.red() / 255.0,
			c.green() / 255.0,
			c.blue() / 255.0,
			1.0);	                                  // all other labels are opaque
	}
	result->Build();
	return result;
}
