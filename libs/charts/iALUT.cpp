// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "iALUT.h"

#include "iAColorTheme.h"
#include "iALog.h"
#include "iALookupTable.h"
#include "iAToolsVTK.h"      // for convertTFToLUT

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

int iALUT::BuildLUT( vtkSmartPointer<vtkLookupTable> pLUT, double const * lutRange, QString colorMap, int numCols /*= 256 */, bool reverse)
{
	if (!m_colorMaps.contains(colorMap))
	{
		LOG(lvlWarn, QString("Invalid color map name %1!").arg(colorMap));
		return 0;
	}
	auto ctf = m_colorMaps[colorMap];
	convertTFToLUT(pLUT, ctf, nullptr, numCols, lutRange, reverse);
	return ctf->GetSize();
}

int iALUT::BuildLUT(vtkSmartPointer<vtkLookupTable> pLUT, double rangeFrom, double rangeTo, QString colorMap,
	int numCols /*= 256 */, bool reverse /*= false*/)
{
	double lutRange[2] = { rangeFrom, rangeTo };
	return BuildLUT( pLUT, lutRange, colorMap, numCols, reverse);
}

iALookupTable iALUT::Build(double const * lutRange, QString colorMap, int numCols, double /*alpha*/, bool reverse /*= false*/)
{
	vtkSmartPointer<vtkLookupTable> vtkLUT(vtkSmartPointer<vtkLookupTable>::New());
	BuildLUT(vtkLUT, lutRange, colorMap, numCols, reverse);
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
