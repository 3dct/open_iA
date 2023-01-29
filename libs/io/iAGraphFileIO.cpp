// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAGraphFileIO.h"

#include "iAAABB.h"
#include "iAValueTypeVectorHelpers.h"

#include <vtkCellData.h>
#include <vtkLine.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkUnsignedCharArray.h>

#include <QColor>
#include <QFile>
#include <QTextStream>

iAGraphFileIO::iAGraphFileIO() : iAFileIO(iADataSetType::Graph, iADataSetType::None)
{
	addAttr(m_params[Load], "Spacing", iAValueType::Vector3, variantVector<double>({ 1.0, 1.0, 1.0 }));
}

std::shared_ptr<iADataSet> iAGraphFileIO::loadData(QString const& fileName, QVariantMap const& paramValues, iAProgress const& progress)
{
	// maybe we could also use vtkPDBReader, but not sure that's the right "PDB" file type...
	Q_UNUSED(progress);

	auto spacing = paramValues["Spacing"].value<QVector<double>>();

	vtkNew<vtkPolyData> myPolyData;

	QFile file(fileName);
	//const auto size = file.size();
	if (!file.open(QIODevice::ReadOnly))
	{
		LOG(lvlError,
			QString("Could not open file '%1' for reading! It probably does not exist!")
			.arg(fileName));
		return {};
	}
	QStringList origCSVInfo;
	QTextStream in(&file);
	// skip headers:
	for (size_t r = 0; r < 4; ++r)
	{
		in.readLine();
	}

	// read vertices
	vtkNew<vtkUnsignedCharArray> colors;
	colors->SetNumberOfComponents(3);
	colors->SetName("Colors");
	vtkNew<vtkPoints> pts;
	//vtkNew<vtkIdList> pointIds;
	//vtkNew<vtkCellArray> polyPoint;
	//size_t curVert = 0;
	QString line = "";
	int numberOfPoints = 0;

	iAAABB bbox;
	while (!in.atEnd() && line != "$$")
	{
		line = in.readLine();
		auto tokens = line.split("\t");
		if (tokens.size() == 7)
		{
			iAVec3d pos(
				tokens[2].toDouble() * spacing[0],
				tokens[3].toDouble() * spacing[1],
				tokens[4].toDouble() * spacing[2]
			);
			bbox.addPointToBox(pos);
			pts->InsertNextPoint(pos.data());
			QColor color(tokens[5]);
			//pointIds->InsertNextId(curVert);
			//polyPoint->InsertNextCell(pointIds);
			unsigned char c[3] = { static_cast<unsigned char>(color.red()), static_cast<unsigned char>(color.green()),
				static_cast<unsigned char>(color.blue()) };
			colors->InsertNextTypedTuple(c);
			++numberOfPoints;
		}
		//auto remains = file.bytesAvailable();
		//auto progress = ((size - remains) * 100) / size;
	}
	assert(numberOfPoints == pts->GetNumberOfPoints());

	// some axes are flipped in comparison to our image data:
	for (int i = 0; i < numberOfPoints; ++i)
	{
		double pt[3];
		pts->GetPoint(i, pt);
		std::swap(pt[0], pt[1]);
		//pt[0] = bbox.bottomRight().x() - pt[0];
		//pt[1] = bbox.bottomRight().y() - pt[1];
		pts->SetPoint(i, pt);
	}


	myPolyData->SetPoints(pts);
	LOG(lvlInfo, QString("%1 points in box %3").arg(pts->GetNumberOfPoints()).arg(toStr(bbox)));

	//myPolyData->SetVerts(polyPoint);
	//myPolyData->GetCellData()->SetScalars(colors);

	line = "";
	in.readLine();    // skip header

	// read edges
	vtkNew<vtkCellArray> lines;
	size_t numberOfLines = 0;
	while (!in.atEnd() && line != "$$")
	{
		line = in.readLine();
		auto tokens = line.split("\t");
		if (tokens.size() == 4)
		{

			vtkNew<vtkLine> lineNEW;
			bool ok;
			int pt1 = tokens[1].toInt(&ok) - 1;
			if (!ok || pt1 < 0 || pt1 >= pts->GetNumberOfPoints())
			{
				LOG(lvlInfo, QString("Invalid point index 1 in line %1: %2").arg(line).arg(pt1));
			}
			int pt2 = tokens[2].toInt(&ok) - 1;
			if (!ok || pt2 < 0 || pt2 >= pts->GetNumberOfPoints())
			{
				LOG(lvlInfo, QString("Invalid point index 2 in line %1: %2").arg(line).arg(pt2));
			}
			lineNEW->GetPointIds()->SetId(0, pt1);
			lineNEW->GetPointIds()->SetId(1, pt2);

			//LOG(lvlInfo, QString("inserting line : %1 -> %2").arg(pt1).arg(pt2));
			lines->InsertNextCell(lineNEW);
			++numberOfLines;
		}
		//auto remains = file.bytesAvailable();
		//auto progress = ((size - remains) * 100) / size;
	}
	//LOG(lvlInfo, QString("Number of lines: %1").arg(numberOfLines));

	// skip last section for now

	myPolyData->SetLines(lines);
	myPolyData->GetPointData()->AddArray(colors);

	return std::make_shared<iAGraphData>(myPolyData);
}

QString iAGraphFileIO::name() const
{
	return "Graph file";
}

QStringList iAGraphFileIO::extensions() const
{                             // pdb as in Brookhaven "Protein Data Bank" format (?)
	return QStringList{ "txt", "pdb" };
}
