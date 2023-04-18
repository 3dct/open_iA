// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAAABB.h"
#include <iAFilterDefault.h>
#include "iALog.h"
#include "iAStringHelper.h"    // for stringToVector
#include "iAToolsVTK.h"
#include "iAValueTypeVectorHelpers.h"

#include "iACsvIO.h"
#include "iACsvVtkTableCreator.h"

#include <vtkImageData.h>
#include <vtkTable.h>

namespace
{
	const QString CsvFileName("CSV filename");
	const QString Size("Size");
	const QString Columns("Columns");
	const QString MinCorner("Minimum corner");
	const QString MaxCorner("Maximum corner");
	const QString ContinueOnError("Continue on error");
}

IAFILTER_DEFAULT_CLASS(iASpatialFeatureSummary);
// maybe use iAFilter::adaptParametersToInput for getting boundaries of csv? but then we would need to parse csv before ...

iASpatialFeatureSummary::iASpatialFeatureSummary():
	iAFilter("Spatial Fiber Summary", "Feature Characteristics",
		"Compute a spatial summary of characteristics of fibers in a list (.csv file).<br/>"
		"This filter takes a table of characteristics of each of the fibers "
		"and computes the number of objects and average characteristics for spatial regions of a defined size.<br/>"
		"As input, the filter requires a feature characteristics table in a .csv file specified by the given <em>" + CsvFileName + "</em>. "
		"It also requires the <em>" + Size + "</em>, i.e. the number of voxels (cubic regions) for which the averages are computed in each direction. "
		"You can load the files in the FeatureScout tool to determine the dimensions of the dataset. "
		//"A .csv configuration is also required, specifying in which .csv columns the <em>Start point</em> and <em>End point</em> of the fibers can be found. "
		    // would require full .csv input spec? for now, let's only use FCP files
		"Note that currently the filter assumes the standard fiber .csv format used by FeatureScout for determining start- and end points of the fibers. "
		"It also requires a comma-separated list of <em>" + Columns + "</em>, "
		"i.e. the indices of the columns for which the average spatial summaries will be computed (one additional output image per column)."
		"If you want the images to cover a specific extent (i.e. bounding box), "
		"you can specify a <em>" + MinCorner + "</em> and a <em>" + MaxCorner + "</em> "
		"(i.e., minimum and maximum x, y and z coordinates); leave all at 0 for them to be automatically computed from the loaded fibers. "
		"", 0, 1)
{
	// Potential for improvement:
	//     - allow choosing CSV config
	//     - fancy GUI for choosing columns, and displaying the spacing of the resulting dataset for the chosen size
	//     - automatic spacing determination (for fixed size of 50x50x50 or so)
	//     - fiber density as 0..1 value
	addParameter(CsvFileName, iAValueType::FileNameOpen, ".csv");
	addParameter(Size, iAValueType::Vector3i, variantVector<int>({50, 50, 50}));
	addParameter(Columns, iAValueType::String, "7,8,9,10,11,17,18,19,20,21");
	addParameter(MinCorner, iAValueType::Vector3, variantVector<double>({ 0.0, 0.0, 0.0 }));
	addParameter(MaxCorner, iAValueType::Vector3, variantVector<double>({ 0.0, 0.0, 0.0 }));
	addParameter(ContinueOnError, iAValueType::Boolean, false);
	setOutputName(0, "Number of fibers");
	setOutputName(1, "Number of fiber start/end points");
}

namespace
{
	std::vector<iAVec3i> findCellIdsBresenham3D(iAVec3i const& start, iAVec3i const& end)
	{
		//bresenham 3D
		//http://www.ict.griffith.edu.au/anthony/info/graphics/bresenham.procs

		int i, l, m, n, x_inc, y_inc, z_inc, err_1, err_2, dx2, dy2, dz2;
		//int id = 0;
		std::vector<iAVec3i> cellIds;

		iAVec3i pixel = start;
		auto dir = end - start;
		x_inc = (dir.x() < 0) ? -1 : 1;
		l = std::abs(dir.x());
		y_inc = (dir.y() < 0) ? -1 : 1;
		m = std::abs(dir.y());
		z_inc = (dir.z() < 0) ? -1 : 1;
		n = std::abs(dir.z());
		dx2 = l << 1;
		dy2 = m << 1;
		dz2 = n << 1;

		if ((l >= m) && (l >= n))
		{
			err_1 = dy2 - l;
			err_2 = dz2 - l;
			for (i = 0; i < l; i++)
			{
				cellIds.push_back(pixel);
				//id++;
				if (err_1 > 0)
				{
					pixel[1] += y_inc;
					err_1 -= dx2;
				}
				if (err_2 > 0)
				{
					pixel[2] += z_inc;
					err_2 -= dx2;
				}
				err_1 += dy2;
				err_2 += dz2;
				pixel[0] += x_inc;
			}
		}
		else if ((m >= l) && (m >= n))
		{
			err_1 = dx2 - m;
			err_2 = dz2 - m;
			for (i = 0; i < m; i++)
			{
				cellIds.push_back(pixel);
				//id++;
				if (err_1 > 0)
				{
					pixel[0] += x_inc;
					err_1 -= dy2;
				}
				if (err_2 > 0)
				{
					pixel[2] += z_inc;
					err_2 -= dy2;
				}
				err_1 += dx2;
				err_2 += dz2;
				pixel[1] += y_inc;
			}
		}
		else
		{
			err_1 = dy2 - n;
			err_2 = dx2 - n;
			for (i = 0; i < n; i++)
			{
				cellIds.push_back(pixel);
				//id++;
				if (err_1 > 0)
				{
					pixel[1] += y_inc;
					err_1 -= dz2;
				}
				if (err_2 > 0)
				{
					pixel[0] += x_inc;
					err_2 -= dz2;
				}
				err_1 += dy2;
				err_2 += dx2;
				pixel[2] += z_inc;
			}
		}
		cellIds.push_back(pixel);
		//id++;

		return cellIds;
	}
}

void iASpatialFeatureSummary::performWork(QVariantMap const & parameters)
{
	QString csvFileName = parameters[CsvFileName].toString();
	
	QVector<int> columns;
	if (!parameters[Columns].toString().isEmpty())
	{
		columns = stringToVector<QVector<int>, int>(parameters[Columns].toString());
	}

	// load csv:
	iACsvIO io;
	iACsvVtkTableCreator tableCreator;
	auto config = iACsvConfig::getFCPFiberFormat(csvFileName);
	if (!io.loadCSV(tableCreator, config))
	{
		LOG(lvlError, QString("Error loading csv file %1!").arg(csvFileName));
		return;
	}
	auto headers = io.getOutputHeaders();
	auto csvTable = tableCreator.table();

	// compute overall bounding box:
	iAVec3i startIdx(config.columnMapping[iACsvConfig::StartX], config.columnMapping[iACsvConfig::StartY], config.columnMapping[iACsvConfig::StartZ]);
	iAVec3i endIdx(config.columnMapping[iACsvConfig::EndX], config.columnMapping[iACsvConfig::EndY], config.columnMapping[iACsvConfig::EndZ]);

	auto minCorner = iAVec3d(parameters[MinCorner].value<QVector<double>>().data());
	auto maxCorner = iAVec3d(parameters[MaxCorner].value<QVector<double>>().data());
	iAVec3d nullVec(0, 0, 0);
	iAAABB overallBB;
	if (minCorner != nullVec || maxCorner != nullVec)
	{
		overallBB.addPointToBox(minCorner);
		overallBB.addPointToBox(maxCorner);
	}
	else
	{
		for (int o = 0; o < csvTable->GetNumberOfRows(); ++o)
		{
			iAVec3d startPoint(csvTable->GetValue(o, startIdx[0]).ToDouble(), csvTable->GetValue(o, startIdx[1]).ToDouble(), csvTable->GetValue(o, startIdx[2]).ToDouble());
			iAVec3d endPoint(csvTable->GetValue(o, endIdx[0]).ToDouble(), csvTable->GetValue(o, endIdx[1]).ToDouble(), csvTable->GetValue(o, endIdx[2]).ToDouble());
			overallBB.addPointToBox(startPoint);
			overallBB.addPointToBox(endPoint);
		}
	}

	// compute image measurements
	auto metaOrigin = overallBB.minCorner().data();
	auto metaDim = parameters[Size].value<QVector<int>>();
	auto metaSpacing = (overallBB.maxCorner() - overallBB.minCorner()) / iAVec3i(metaDim.data());
	LOG(lvlDebug, QString("Creating image; ") +
		QString("dimensions: %1x%2x%3; ")
		.arg(metaDim[0]).arg(metaDim[1]).arg(metaDim[2]) +
		QString("spacing: %1, %2, %3; ").arg(metaSpacing[0]).arg(metaSpacing[1]).arg(metaSpacing[2]) +
		QString("origin: %1, %2, %3; ").arg(metaOrigin[0]).arg(metaOrigin[1]).arg(metaOrigin[2]));

	
	// determine number of fibers in each cell:
	auto numberOfFibersImage = allocateImage(VTK_INT, metaDim.data(), metaSpacing.data());
	auto numberOfPointsImage = allocateImage(VTK_INT, metaDim.data(), metaSpacing.data());
	fillImage(numberOfFibersImage, 0.0);
	fillImage(numberOfPointsImage, 0.0);
	for (int o = 0; o < csvTable->GetNumberOfRows(); ++o)
	{
		iAVec3i startVoxel(iAVec3d(csvTable->GetValue(o, startIdx[0]).ToDouble(), csvTable->GetValue(o, startIdx[1]).ToDouble(), csvTable->GetValue(o, startIdx[2]).ToDouble()) / metaSpacing);
		iAVec3i endVoxel(iAVec3d(csvTable->GetValue(o, endIdx[0]).ToDouble(), csvTable->GetValue(o, endIdx[1]).ToDouble(), csvTable->GetValue(o, endIdx[2]).ToDouble()) / metaSpacing);
		numberOfPointsImage->SetScalarComponentFromDouble(startVoxel.x(), startVoxel.y(), startVoxel.z(), 0,
			numberOfPointsImage->GetScalarComponentAsDouble(startVoxel.x(), startVoxel.y(), startVoxel.z(), 0) + 1);
		numberOfPointsImage->SetScalarComponentFromDouble(endVoxel.x(), endVoxel.y(), endVoxel.z(), 0,
			numberOfPointsImage->GetScalarComponentAsDouble(endVoxel.x(), endVoxel.y(), endVoxel.z(), 0) + 1);
		auto cellIds = findCellIdsBresenham3D(startVoxel, endVoxel);
		for (auto c : cellIds)
		{
			if (c[0] < 0 || c[0] >= metaDim[0] || c[1] < 0 || c[1] >= metaDim[1] || c[2] < 0 || c[2] >= metaDim[2])
			{
				LOG(lvlWarn, QString("Invalid coordinate %1; given volume dimensions are probably too small to contain the fibers in the .csv!").arg(c.toString()));
				if (parameters[ContinueOnError].toBool())
				{
					continue;
				}
				else
				{
					return;
				}
			}
			numberOfFibersImage->SetScalarComponentFromDouble(c.x(), c.y(), c.z(), 0,
				numberOfFibersImage->GetScalarComponentAsDouble(c.x(), c.y(), c.z(), 0) + 1);
		}
	}
	addOutput(numberOfFibersImage);
	addOutput(numberOfPointsImage);
	auto r = numberOfFibersImage->GetScalarRange();
	LOG(lvlDebug, QString("Number of fibers: from %1 to %2").arg(r[0]).arg(r[1]));

	// determine characteristics averages:
	auto curOutput = 2;
	for (auto col : columns)
	{
		auto metaImage = allocateImage(VTK_DOUBLE, metaDim.data(), metaSpacing.data());
		metaImage->SetOrigin(metaOrigin);
		fillImage(metaImage, 0.0);
		for (int o = 0; o < csvTable->GetNumberOfRows(); ++o)
		{
			iAVec3i startVoxel(iAVec3d(csvTable->GetValue(o, startIdx[0]).ToDouble(), csvTable->GetValue(o, startIdx[1]).ToDouble(), csvTable->GetValue(o, startIdx[2]).ToDouble()) / metaSpacing);
			iAVec3i endVoxel(iAVec3d(csvTable->GetValue(o, endIdx[0]).ToDouble(), csvTable->GetValue(o, endIdx[1]).ToDouble(), csvTable->GetValue(o, endIdx[2]).ToDouble()) / metaSpacing);
			auto val = csvTable->GetValue(o, col).ToDouble();
			auto cellIds = findCellIdsBresenham3D(startVoxel, endVoxel);
			for (auto c : cellIds)
			{
				if (c[0] < 0 || c[0] >= metaDim[0] || c[1] < 0 || c[1] >= metaDim[1] || c[2] < 0 || c[2] >= metaDim[2])
				{
					LOG(lvlWarn, QString("Invalid coordinate %1").arg(c.toString()));
					continue;
				}
				auto cellValue = metaImage->GetScalarComponentAsDouble(c.x(), c.y(), c.z(), 0);
				auto numOfFibersInCell = numberOfFibersImage->GetScalarComponentAsDouble(c.x(), c.y(), c.z(), 0);
				assert(numOfFibersInCell != 0); // should not happen, since above we visited the same cells
				metaImage->SetScalarComponentFromDouble(c.x(), c.y(), c.z(), 0, cellValue + val / numOfFibersInCell	);
			}
		}
		setOutputName(curOutput, headers[col]);
		addOutput(metaImage);
		auto rng = metaImage->GetScalarRange();
		LOG(lvlDebug, QString("Output %1: values from %2 to %3").arg(headers[col]).arg(rng[0]).arg(rng[1]));
		++curOutput;
	}
}
