/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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
#include "iAFiberCharData.h"

#include "iARefDistCompute.h" // only for DistanceMetricCount!

#include "iACsvIO.h"
#include "iACsvVtkTableCreator.h"

#include "charts/iASPLOMData.h"
#include "iAConsole.h"
#include "io/iAFileUtils.h" // for FindFiles

#include <vtkFloatArray.h>
#include <vtkTable.h>

#include <QFileInfo>
#include <QSettings>
#include <QTextStream>

#include <algorithm> // for std::fill

bool operator<(iAFiberDistance const & a, iAFiberDistance const & b)
{
	return a.distance < b.distance;
}

const QString iAFiberResultsCollection::LegacyFormat("FIAKER Legacy Format");
const QString iAFiberResultsCollection::SimpleFormat("FIAKER Simple Format");

namespace
{
	const double CoordinateShift = 74.5;

	iACsvConfig getLegacyConfig()
	{
		iACsvConfig config = iACsvConfig::getLegacyFiberFormat("");
		config.skipLinesStart = 0;
		config.containsHeader = false;
		config.visType = iACsvConfig::Cylinders;
		return config;
	}

	iACsvConfig getSimpleConfig()
	{
		iACsvConfig config;
		config.encoding = "System";
		config.skipLinesStart = 0;
		config.skipLinesEnd = 0;
		config.containsHeader = false;
		config.columnSeparator = ",";
		config.decimalSeparator = ".";
		config.addAutoID = false;
		config.objectType = iAFeatureScoutObjectType::Fibers;
		config.computeLength = false;
		config.computeAngles = false;
		config.computeTensors = false;
		config.computeCenter = false;
		config.computeStartEnd = true;
		std::fill_n(config.offset, 3, CoordinateShift);
		config.visType = iACsvConfig::Cylinders;
		config.currentHeaders = QStringList() <<
			"ID" << "CenterX" << "CenterY" << "CenterZ" << "Phi" << "Theta" << "Length";
		config.selectedHeaders = config.currentHeaders;
		config.columnMapping.clear();
		config.columnMapping.insert(iACsvConfig::CenterX, 1);
		config.columnMapping.insert(iACsvConfig::CenterY, 2);
		config.columnMapping.insert(iACsvConfig::CenterZ, 3);
		config.columnMapping.insert(iACsvConfig::Phi, 4);
		config.columnMapping.insert(iACsvConfig::Theta, 5);
		config.columnMapping.insert(iACsvConfig::Length, 6);
		config.visType = iACsvConfig::Cylinders;
		config.isDiameterFixed = true;
		config.fixedDiameterValue = 7;
		return config;
	}
}

iACsvConfig getCsvConfig(QString const & csvFile, QString const & formatName)
{
	iACsvConfig result;
	QSettings settings;
	if (!result.load(settings, formatName))
	{
		if (formatName == iACsvConfig::LegacyFiberFormat)
			result = iACsvConfig::getLegacyFiberFormat(csvFile);
		else if (formatName == iACsvConfig::LegacyVoidFormat)
			result = iACsvConfig::getLegacyPoreFormat(csvFile);
		else if (formatName == iAFiberResultsCollection::LegacyFormat)
			result = getLegacyConfig();
		else if (formatName == iAFiberResultsCollection::SimpleFormat)
			result = getSimpleConfig();
		else
			DEBUG_LOG(QString("Invalid format %1!").arg(formatName));
	}
	result.fileName = csvFile;
	return result;
}

void addColumn(vtkSmartPointer<vtkTable> table, float value, char const * columnName, size_t numRows)
{
	vtkSmartPointer<vtkFloatArray> arrX = vtkSmartPointer<vtkFloatArray>::New();
	arrX->SetName(columnName);
	arrX->SetNumberOfValues(numRows);
#if (VTK_MAJOR_VERSION >= 8)
	arrX->Fill(value);
#else
	for (vtkIdType i=0; i<numRows; ++i)
	{
		arrX->SetValue(i, value);
	}
#endif
	table->AddColumn(arrX);
}

iAFiberResultsCollection::iAFiberResultsCollection():
	spmData(new iASPLOMData()),
	optimStepMax(1),
	minFiberCount(std::numeric_limits<size_t>::max()),
	maxFiberCount(0)
{}

bool iAFiberResultsCollection::loadData(QString const & path, QString const & configName, iAProgress * progress)
{
	folder = path;
	QStringList filters;
	filters << "*.csv";
	QStringList csvFileNames;

	FindFiles(path, filters, false, csvFileNames, Files);

	const int MaxDatasetCount = 25;
	if (csvFileNames.size() > MaxDatasetCount)
	{
		DEBUG_LOG(QString("The specified folder %1 contains %2 datasets; currently we only support loading up to %3 datasets!")
			.arg(path).arg(csvFileNames.size()).arg(MaxDatasetCount));
		return false;
	}
	int resultID = 0;
	std::vector<QString> paramNames;
	// load all datasets:
	for (QString csvFile : csvFileNames)
	{
		iACsvConfig config = getCsvConfig(csvFile, configName);
		iACsvIO io;
		iACsvVtkTableCreator tableCreator;
		if (!io.loadCSV(tableCreator, config))
		{
			DEBUG_LOG(QString("Could not load file '%1' - probably it's in a wrong format; skipping!").arg(csvFile));
			continue;
		}

		iAFiberCharData curData;
		curData.table = tableCreator.getTable();
		curData.fiberCount = curData.table->GetNumberOfRows();
		curData.mapping = io.getOutputMapping();
		curData.fileName = csvFile;
		if (curData.fiberCount < minFiberCount)
			minFiberCount = curData.fiberCount;
		if (curData.fiberCount > maxFiberCount)
			maxFiberCount = curData.fiberCount;

		if (result.empty())
			for (size_t h=0; h<io.getOutputHeaders().size(); ++h)
				paramNames.push_back(io.getOutputHeaders()[h]);
		else
		{
			// Check if output mapping is the same (it must be)!
			for (auto key: result[0].mapping->keys())
				if (curData.mapping->value(key) != result[0].mapping->value(key))
				{
					DEBUG_LOG(QString("Mapping does not match for result %1, column %2!"));
					return false;
				}
			// (though actually same mapping should be guaranteed by using same config)
		}

		QFileInfo timeInfo(QFileInfo(csvFile).absolutePath() + "/" + QFileInfo(csvFile).baseName());

		// TODO: in case reading gets inefficient, look at pre-reserving the required amount of fields
		//       and using std::vector::swap to assign the sub-vectors!

		size_t thisResultTimeStepMax = 1;
		if (timeInfo.exists() && timeInfo.isDir())
		{
			// read projection error info:
			QFile projErrorFile(timeInfo.absoluteFilePath() + "/projection_error.csv");
			if (!projErrorFile.open(QIODevice::ReadOnly | QIODevice::Text))
			{
				DEBUG_LOG(QString("Unable to open projection error file: %1").arg(projErrorFile.errorString()));
			}
			else
			{
				curData.projectionError.resize(curData.fiberCount);
				QTextStream in(&projErrorFile);
				size_t fiberID = 0;
				while (!in.atEnd())
				{
					QString line = in.readLine();
					QStringList valueStrList = line.split(",");
					if (valueStrList.size() < 2)
						continue;
					if (fiberID >= curData.fiberCount)
					{
						DEBUG_LOG(QString("Discrepancy: More lines in %1 file than there were fibers in the fiber description csv (%2)")
								  .arg(projErrorFile.fileName()).arg(curData.fiberCount));
						break;
					}
					auto & projErrFib = curData.projectionError[fiberID]; //.resize(valueStrList.size());
					for (int i = 0; i < valueStrList.size(); ++i)
					{
						if (valueStrList[i] == "nan")
							break;
						projErrFib.push_back(valueStrList[i].toDouble());
					}
					for (int i = 0; i < projErrFib.size(); ++i)
						projErrFib[i] -= projErrFib[projErrFib.size()-1];
					++fiberID;
				}
			}

			// fiber, timestep, value
			std::vector<std::vector<std::vector<double> > > fiberTimeValues;
			int curFiber = 0;
			do
			{
				QString fiberTimeCsv = QString("fiber%1_paramlog.csv").arg(curFiber, 3, 10, QChar('0'));
				QFileInfo fiberTimeCsvInfo(timeInfo.absoluteFilePath() + "/" + fiberTimeCsv);
				if (!fiberTimeCsvInfo.exists())
					break;
				std::vector<std::vector<double> > singleFiberValues;
				QFile file(fiberTimeCsvInfo.absoluteFilePath());
				if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
				{
					DEBUG_LOG(QString("Unable to open file: %1").arg(file.errorString()));
					break;
				}
				QTextStream in(&file);
				in.readLine(); // skip header line
				size_t lineNr = 1;
				while (!in.atEnd())
				{
					lineNr++;
					QString line = in.readLine();
					QStringList values = line.split(",");
					if (values.size() != 6)
					{
						DEBUG_LOG(QString("Invalid line %1 in file %2, there should be 6 entries but there are %3 (line: %4)")
							.arg(lineNr).arg(fiberTimeCsvInfo.fileName()).arg(values.size()).arg(line));
						continue;
					}
					double middlePoint[3];
					for (int i = 0; i < 3; ++i)
						middlePoint[i] = values[i].toDouble() + CoordinateShift; // middle point positions are shifted!
					double theta = values[4].toDouble();
					if (theta < 0)  // theta is encoded in -Pi, Pi instead of 0..Pi as we expect
						theta = 2*vtkMath::Pi() + theta;
					double phi = values[3].toDouble();
					double radius = values[5].toDouble() * 0.5;

					std::vector<double> timeStepValues(iAFiberCharData::FiberValueCount);
					// convert spherical to cartesian coordinates:
					double dir[3];
					dir[0] = radius * std::sin(phi) * std::cos(theta);
					dir[1] = radius * std::sin(phi) * std::sin(theta);
					dir[2] = radius * std::cos(phi);
					for (int i = 0; i<3; ++i)
					{
						timeStepValues[i] = middlePoint[i] + dir[i];
						timeStepValues[i+3] = middlePoint[i] - dir[i];
						timeStepValues[i+6] = middlePoint[i];
					}
					timeStepValues[9] = phi;
					timeStepValues[10] = theta;
					timeStepValues[11] = values[5].toDouble();
					timeStepValues[12] = curData.table->GetValue(curFiber, (*curData.mapping)[iACsvConfig::Diameter]).ToDouble();
					/*
					DEBUG_LOG(QString("Fiber %1, step %2: Start (%3, %4, %5) - End (%6, %7, %8)")
						.arg(curFiber)
						.arg(singleFiberValues.size())
						.arg(timeStepValues[0]).arg(timeStepValues[1]).arg(timeStepValues[2])
						.arg(timeStepValues[3]).arg(timeStepValues[4]).arg(timeStepValues[5]));
					*/
					singleFiberValues.push_back(timeStepValues);
				}
				if (singleFiberValues.size() > thisResultTimeStepMax)
				{
					thisResultTimeStepMax = singleFiberValues.size();
				}
				fiberTimeValues.push_back(singleFiberValues);
				++curFiber;
			} while (true);
			int fiberCount = curFiber;

			// transform from [fiber, timestep, value] to [timestep, fiber, value] indexing
			// TODO: make sure all datasets have the same max timestep count!
			curData.timeValues.resize(thisResultTimeStepMax);
			for (int t = 0; t < thisResultTimeStepMax; ++t)
			{
				curData.timeValues[t].resize(fiberCount);
				for (int f = 0; f < fiberCount; ++f)
				{
					curData.timeValues[t][f] = (t<fiberTimeValues[f].size())?fiberTimeValues[f][t] : fiberTimeValues[f][fiberTimeValues[f].size()-1];
				}
			}
		}
		if (thisResultTimeStepMax > optimStepMax)
		{
			if (optimStepMax > 1)
			{
				DEBUG_LOG(QString("In result %1, the maximum number of timesteps changes from %2 to %3! This shouldn't be a problem, but support for it is currently untested.")
					.arg(resultID).arg(optimStepMax).arg(thisResultTimeStepMax));
			}
			optimStepMax = thisResultTimeStepMax;
		}
		++resultID;
		progress->EmitProgress(resultID / csvFileNames.size());
		result.push_back(curData);
	}
	if (result.size() == 0)
	{
		DEBUG_LOG(QString("The specified folder %1 does not contain any valid csv files!").arg(path));
		return false;
	}

	// create SPM data:
	paramNames.push_back("StartXShift");
	paramNames.push_back("StartYShift");
	paramNames.push_back("StartZShift");
	paramNames.push_back("EndXShift");
	paramNames.push_back("EndYShift");
	paramNames.push_back("EndZShift");
	paramNames.push_back("XmShift");
	paramNames.push_back("YmShift");
	paramNames.push_back("ZmShift");
	paramNames.push_back("PhiDiff");
	paramNames.push_back("ThetaDiff");
	paramNames.push_back("LengthDiff");
	paramNames.push_back("DiameterDiff");

	paramNames.push_back("d_c¹");
	paramNames.push_back("d_c²");

	paramNames.push_back("d_p¹");
	paramNames.push_back("d_p²");
	paramNames.push_back("d_p³");

	paramNames.push_back("d_o¹");
	paramNames.push_back("d_o²");
	paramNames.push_back("d_o³");

	paramNames.push_back("ProjectionErrorReduction");
	paramNames.push_back("Result_ID");
	spmData->setParameterNames(paramNames);
	size_t numParams = spmData->numParams();
	size_t spmStartIdx = 0;
	for (resultID=0; resultID<result.size(); ++resultID)
	{
		auto & curData = result[resultID];
		size_t numTableColumns = curData.table->GetNumberOfColumns();
		for (int i = (iARefDistCompute::DistanceMetricCount+iAFiberCharData::FiberValueCount+iARefDistCompute::EndColumns); i >= iARefDistCompute::EndColumns; --i)
		{
			spmData->data()[numParams - i].resize(spmData->data()[numParams - i].size() + curData.fiberCount, 0);
		}
		for (size_t fiberID = 0; fiberID < curData.fiberCount; ++fiberID)
		{
			for (vtkIdType col = 0; col < numTableColumns; ++col)
			{
				double value = curData.table->GetValue(fiberID, col).ToDouble();
				spmData->data()[col].push_back(value);
			}
			spmData->data()[numParams-1].push_back(resultID);

			double projErrorRed = curData.projectionError.size() > 0 ?
				curData.projectionError[fiberID][0] - curData.projectionError[fiberID][curData.projectionError[fiberID].size() - 1]
					: 0;
			spmData->data()[numParams-2][spmStartIdx + fiberID] = projErrorRed;
			curData.table->SetValue(fiberID, numParams - 2, projErrorRed);
		}
		// TODO: reuse spmData also for 3d visualization?
		for (int col = 0; col < (iARefDistCompute::DistanceMetricCount+ iAFiberCharData::FiberValueCount+iARefDistCompute::EndColumns-1); ++col)
		{
			addColumn(curData.table, 0, spmData->parameterName(numTableColumns+col).toStdString().c_str(), curData.fiberCount);
		}
		addColumn(curData.table, resultID, spmData->parameterName(numParams-1).toStdString().c_str(), curData.fiberCount);

		spmStartIdx += curData.fiberCount;
	}

	spmData->updateRanges();

	return true;
}

iAFiberResultsLoader::iAFiberResultsLoader(QSharedPointer<iAFiberResultsCollection> results, QString const & path, QString const & configName):
	m_results(results),
	m_path(path),
	m_configName(configName)
{}

void iAFiberResultsLoader::run()
{
	if (!m_results->loadData(m_path, m_configName, &m_progress))
		emit failed(m_path);
}

iAProgress* iAFiberResultsLoader::progress()
{
	return &m_progress;
}
