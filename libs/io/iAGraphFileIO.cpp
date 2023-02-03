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
#include "iAGraphFileIO.h"

#include "iAAABB.h"
#include "iAValueTypeVectorHelpers.h"

#include <vtkCellData.h>
#include <vtkDoubleArray.h>
#include <vtkLine.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkUnsignedCharArray.h>

#include <QColor>
#include <QFile>
#include <QSet>
#include <QTextStream>

iAGraphFileIO::iAGraphFileIO() : iAFileIO(iADataSetType::Graph, iADataSetType::None)
{
	addAttr(m_params[Load], "Spacing", iAValueType::Vector3, variantVector<double>({ 1.0, 1.0, 1.0 }));
	addAttr(m_params[Load], "Swap XY", iAValueType::Boolean, false);
}


void createValueNamesAndArrays(QStringList& header, QSet<int> mappedIndices, QStringList& valNames, std::vector<vtkSmartPointer<vtkDoubleArray>>& allVals, std::vector<int>& valIndices)
{
	for (int v = 0; v < header.size(); ++v)
	{
		if (!mappedIndices.contains(v))
		{
			valNames.push_back(header[v]);
			valIndices.push_back(v);
		}
	}
	for (int v = 0; v < valNames.size(); ++v)
	{
		auto arr = vtkSmartPointer<vtkDoubleArray>::New();
		arr->SetName(valNames[v].toStdString().c_str());
		allVals.push_back(arr);
	}
}

std::shared_ptr<iADataSet> iAGraphFileIO::loadData(QString const& fileName, QVariantMap const& paramValues, iAProgress const& p)
{
	auto spacing = paramValues["Spacing"].value<QVector<double>>();

	vtkNew<vtkPolyData> myPolyData;

	QFile file(fileName);
	auto const fileSize = file.size();
	if (!file.open(QIODevice::ReadOnly))
	{
		LOG(lvlError,
			QString("Could not open file '%1' for reading! It probably does not exist!")
			.arg(fileName));
		return {};
	}
	QStringList origCSVInfo;
	QTextStream in(&file);

	// skip header comments:
	QString line;
	do
	{
		line = in.readLine();
	} while (line.startsWith("%%"));

	// current line contains vertex column headers:
	QStringList vertexHeader = line.split("\t");
	// determine column mapping for coordinates:
	int vertexIDIdx = vertexHeader.indexOf("id");
	int xIdx = vertexHeader.indexOf("x");
	int yIdx = vertexHeader.indexOf("y");
	int zIdx = vertexHeader.indexOf("z");
	int vertexColorIdx = vertexHeader.indexOf("color");
	if (xIdx == -1 || yIdx == -1 || zIdx == -1 || vertexColorIdx == -1)
	{
		LOG(lvlError, QString("An expected column (x, y, z or color) was not found!"));
		return {};
	}
	QSet<int> mappedVertexIndices{ vertexIDIdx, xIdx, yIdx, zIdx, vertexColorIdx };
	QStringList vertexValueNames;
	std::vector<vtkSmartPointer<vtkDoubleArray>> allVertexValues;
	std::vector<int> vertexValIdx;
	createValueNamesAndArrays(vertexHeader, mappedVertexIndices, vertexValueNames, allVertexValues, vertexValIdx);

	vtkNew<vtkUnsignedCharArray> vertexColors;
	vertexColors->SetNumberOfComponents(3);
	vertexColors->SetName("VertexColors");
	vtkNew<vtkPoints> pts;

	int numberOfVertices = 0;
	line = in.readLine();
	iAAABB bbox;
	while (!in.atEnd() && line != "$$")
	{
		std::vector<double> curVertexValues;
		auto tokens = line.split("\t");
		if (tokens.size() != vertexHeader.size())
		{
			throw std::runtime_error(QString("Vertex line %1 has %2 tokens instead of the %3 expected from parsing the header!")
				.arg(line).arg(tokens.size()).arg(vertexHeader.size()).toStdString());
		}
		auto vertID = tokens[vertexIDIdx].toInt();
		if (vertID != numberOfVertices + 1)
		{
			throw std::runtime_error(QString("Non-sequential vertex IDs - line %1 has an id value of %2, but we are reading the %3th vertex.")
				.arg(line).arg(vertID).arg(numberOfVertices + 1).toStdString());
		}
		bool ok1, ok2, ok3;
		iAVec3d pos(
			tokens[xIdx].toDouble(&ok1) * spacing[0],
			tokens[yIdx].toDouble(&ok2) * spacing[1],
			tokens[zIdx].toDouble(&ok3) * spacing[2]
		);
		if (!ok1 || !ok2 || !ok3)
		{
			throw std::runtime_error(QString("Invalid vertex position value in line %1!").arg(line).toStdString());
		}
		bbox.addPointToBox(pos);
		pts->InsertNextPoint(pos.data());
		QColor color(tokens[vertexColorIdx]);
		unsigned char c[3] = {
			static_cast<unsigned char>(color.red()),
			static_cast<unsigned char>(color.green()),
			static_cast<unsigned char>(color.blue()) };
		vertexColors->InsertNextTypedTuple(c);
		for (size_t i = 0; i < vertexValIdx.size(); ++i)
		{
			bool ok;
			allVertexValues[i]->InsertNextTuple1(tokens[vertexValIdx[i]].toDouble(&ok));
			if (!ok)
			{
				throw std::runtime_error(QString("Vertex line %1: Invalid additional value %2 (field %3)")
					.arg(line).arg(tokens[vertexValIdx[i]]).arg(vertexValueNames[i]).toStdString());
			}
		}
		++numberOfVertices;
		auto progress = file.pos() * 100 / fileSize;
		p.emitProgress(progress);
		line = in.readLine();
	}
	assert(numberOfVertices == pts->GetNumberOfPoints());

	if (paramValues["Swap XY"].toBool())
	{
		// x and y axes are swapped in comparison to our image data - integrate into above loop?
		for (int i = 0; i < numberOfVertices; ++i)
		{
			double pt[3];
			pts->GetPoint(i, pt);
			std::swap(pt[0], pt[1]);
			//pt[0] = bbox.bottomRight().x() - pt[0];
			//pt[1] = bbox.bottomRight().y() - pt[1];
			pts->SetPoint(i, pt);
		}
	}

	myPolyData->SetPoints(pts);
	LOG(lvlInfo, QString("%1 points in box %3").arg(pts->GetNumberOfPoints()).arg(toStr(bbox)));

	// parse edge values: 
	line = in.readLine();
	QStringList edgeHeader = line.split("\t");
	int edgeIDIdx = edgeHeader.indexOf("id");
	int pt1Idx = edgeHeader.indexOf("Vert_1");
	int pt2Idx = edgeHeader.indexOf("Vert_2");
	int edgeColorIdx = edgeHeader.indexOf("color");
	QSet<int> mappedEdgeIndices{ edgeIDIdx, pt1Idx, pt2Idx, edgeColorIdx };
	QStringList edgeValueNames;
	std::vector<vtkSmartPointer<vtkDoubleArray>> allEdgeValues;
	std::vector<int> edgeValIdx;
	createValueNamesAndArrays(edgeHeader, mappedEdgeIndices, edgeValueNames, allEdgeValues, edgeValIdx);

	vtkNew<vtkUnsignedCharArray> edgeColors;
	edgeColors->SetNumberOfComponents(3);
	edgeColors->SetName("EdgeColors");

	// read edges
	vtkNew<vtkCellArray> lines;
	line = in.readLine();
	while (!in.atEnd() && !line.isEmpty())
	{
		auto tokens = line.split("\t");
		if (tokens.size() != edgeHeader.size())
		{
			throw std::runtime_error(QString("Edge line %1 has %2 tokens instead of the %3 expected from parsing the header!")
				.arg(line).arg(tokens.size()).arg(edgeHeader.size()).toStdString());
		}

		vtkNew<vtkLine> lineNEW;
		bool ok;
		int pt1 = tokens[pt1Idx].toInt(&ok) - 1;
		if (!ok || pt1 < 0 || pt1 >= pts->GetNumberOfPoints())
		{
			throw std::runtime_error(QString("Invalid point index 1 in edge line %1: %2").arg(line).arg(pt1).toStdString());
		}
		int pt2 = tokens[pt2Idx].toInt(&ok) - 1;
		if (!ok || pt2 < 0 || pt2 >= pts->GetNumberOfPoints())
		{
			throw std::runtime_error(QString("Invalid point index 2 in edge line %1: %2").arg(line).arg(pt2).toStdString());
		}
		lineNEW->GetPointIds()->SetId(0, pt1);
		lineNEW->GetPointIds()->SetId(1, pt2);

		lines->InsertNextCell(lineNEW);

		QColor color(tokens[edgeColorIdx]);
		unsigned char c[3] = {
			static_cast<unsigned char>(color.red()),
			static_cast<unsigned char>(color.green()),
			static_cast<unsigned char>(color.blue()) };
		edgeColors->InsertNextTypedTuple(c);

		for (size_t i = 0; i < edgeValIdx.size(); ++i)
		{
			allEdgeValues[i]->InsertNextTuple1(tokens[edgeValIdx[i]].toDouble(&ok));
			if (!ok)
			{
				throw std::runtime_error(QString("Edge line %1: Invalid additional value %2 (field %3)")
					.arg(line).arg(tokens[edgeValIdx[i]]).arg(edgeValueNames[i]).toStdString());
			}
		}

		auto progress = file.pos() * 100 / fileSize;
		p.emitProgress(progress);

		line = in.readLine();
	}
	//LOG(lvlInfo, QString("Number of lines: %1").arg(numberOfLines));

	// skip last section for now

	myPolyData->SetLines(lines);
	myPolyData->GetPointData()->AddArray(vertexColors);
	for (int v=0; v < allVertexValues.size(); ++v)
	{
		myPolyData->GetPointData()->AddArray(allVertexValues[v]);
	}

	myPolyData->GetCellData()->AddArray(edgeColors);
	for (int v = 0; v < allEdgeValues.size(); ++v)
	{
		myPolyData->GetCellData()->AddArray(allEdgeValues[v]);
	}

	return std::make_shared<iAGraphData>(myPolyData, vertexValueNames, edgeValueNames);
}

QString iAGraphFileIO::name() const
{
	return "Graph file";
}

QStringList iAGraphFileIO::extensions() const
{                             // pdb as in Brookhaven "Protein Data Bank" format (?)
	return QStringList{ "txt", "pdb" };
}
