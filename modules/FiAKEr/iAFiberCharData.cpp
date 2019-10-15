/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iAFiberCharData.h"

#include "iARefDistCompute.h" // only for SimilarityMeasureCount!

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

bool operator<(iAFiberSimilarity const & a, iAFiberSimilarity const & b)
{
	return a.similarity < b.similarity;
}

const QString iAFiberResultsCollection::LegacyFormat("FIAKER Legacy Format");
const QString iAFiberResultsCollection::SimpleFormat("FIAKER Simple Format");

namespace
{
	const double SimpleConfigCoordShift = 74.5;

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
		std::fill_n(config.offset, 3, SimpleConfigCoordShift);
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

iACsvConfig getCsvConfig(QString const & formatName)
{
	iACsvConfig result;
	QSettings settings;
	if (!result.load(settings, formatName))
	{
		if (formatName == iACsvConfig::LegacyFiberFormat)
			result = iACsvConfig::getLegacyFiberFormat("");
		else if (formatName == iACsvConfig::LegacyVoidFormat)
			result = iACsvConfig::getLegacyPoreFormat("");
		else if (formatName == iAFiberResultsCollection::LegacyFormat)
			result = getLegacyConfig();
		else if (formatName == iAFiberResultsCollection::SimpleFormat)
			result = getSimpleConfig();
		else
			DEBUG_LOG(QString("Invalid format %1!").arg(formatName));
	}
	return result;
}

void addColumn(vtkSmartPointer<vtkTable> table, float value, char const * columnName, size_t numRows)
{
	vtkSmartPointer<vtkFloatArray> arrX = vtkSmartPointer<vtkFloatArray>::New();
	arrX->SetName(columnName);
	arrX->SetNumberOfValues(numRows);
	arrX->Fill(value);
	table->AddColumn(arrX);
}

iAFiberResultsCollection::iAFiberResultsCollection():
	spmData(new iASPLOMData()),
	optimStepMax(1),
	minFiberCount(std::numeric_limits<size_t>::max()),
	maxFiberCount(0),
	stepShift(0)
{}

bool iAFiberResultsCollection::loadData(QString const & path, iACsvConfig const & cfg, double stepShift, iAProgress * progress)
{
	folder = path;
	this->stepShift = stepShift;
	QStringList filters;
	filters << "*.csv";
	QStringList csvFileNames;

	FindFiles(path, filters, false, csvFileNames, Files);

	const int MaxDatasetCount = 100;
	if (csvFileNames.size() > MaxDatasetCount)
	{
		DEBUG_LOG(QString("The specified folder %1 contains %2 datasets; currently we only support loading up to %3 datasets!")
			.arg(path).arg(csvFileNames.size()).arg(MaxDatasetCount));
		return false;
	}
	int resultID = 0;
	std::vector<QString> paramNames;

	QStringList noCurvedFiberFiles;
	// load all datasets:
	for (QString csvFile : csvFileNames)
	{
		iACsvConfig config(cfg);
		config.fileName = csvFile;
		objectType = config.visType;
		iACsvIO io;
		iACsvVtkTableCreator tableCreator;
		if (!io.loadCSV(tableCreator, config))
		{
			DEBUG_LOG(QString("Could not load file '%1' - probably it's in a wrong format; skipping!").arg(csvFile));
			continue;
		}

		iAFiberCharData curData;
		curData.table = tableCreator.table();
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
					DEBUG_LOG(QString("Mapping does not match for result %1, column %2!").arg(csvFile).arg(curData.mapping->value(key)) );
					return false;
				}
			// (though actually same mapping should be guaranteed by using same config)
		}

		QString timeInfoPath(QFileInfo(csvFile).absolutePath() + "/" + QFileInfo(csvFile).baseName());
		QFileInfo timeInfo(timeInfoPath);

		// TODO: in case reading gets inefficient, look at pre-reserving the required amount of fields
		//       and using std::vector::swap to assign the sub-vectors!

		size_t thisResultTimeStepMax = 1;
		if (timeInfo.exists() && timeInfo.isDir())
		{
			curData.timeData = iAFiberCharData::SimpleTimeData;
			// read projection error info:
			QFile projErrorFile(timeInfo.absoluteFilePath() + "/projection_error.csv");
			if (!projErrorFile.open(QIODevice::ReadOnly | QIODevice::Text))
			{
				//DEBUG_LOG(QString("Unable to open projection error file: %1").arg(projErrorFile.errorString()));
				curData.timeData = iAFiberCharData::NoTimeData;
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
			for (int curFiber=0; curFiber<curData.fiberCount; ++curFiber)
			{
				QString fiberTimeCsv = QString("fiber%1_paramlog.csv").arg(curFiber, 3, 10, QChar('0'));
				QFileInfo fiberTimeCsvInfo(timeInfo.absoluteFilePath() + "/" + fiberTimeCsv);
				if (!fiberTimeCsvInfo.exists())
				{
					DEBUG_LOG(QString("File '%1' does not exist!").arg(fiberTimeCsv));
					curData.timeData = iAFiberCharData::NoTimeData;
					break;
				}
				std::vector<std::vector<double> > singleFiberValues;
				QFile file(fiberTimeCsvInfo.absoluteFilePath());
				if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
				{
					DEBUG_LOG(QString("Unable to open file '%1': %2")
						.arg(fiberTimeCsv)
						.arg(file.errorString()));
					curData.timeData = iAFiberCharData::NoTimeData;
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
						middlePoint[i] = values[i].toDouble() + stepShift; // middle point positions are shifted!
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
				assert(singleFiberValues.size() > 0);
				if (singleFiberValues.size() > thisResultTimeStepMax)
				{
					thisResultTimeStepMax = singleFiberValues.size();
				}
				fiberTimeValues.push_back(singleFiberValues);
			}

			if (curData.timeData == iAFiberCharData::NoTimeData)
			{
				// check if we can load new, curved timestep data:
				std::vector<std::vector<std::vector<double> > > fiberTimeValues;
				curData.projectionError.resize(curData.fiberCount);
				curData.timeData = iAFiberCharData::CurvedTimeData;
				for (int curFiber = 0; curFiber < curData.fiberCount; ++curFiber)
				{
					QString fiberTimeCsv = QString("fiber_%1.csv").arg(curFiber, 4, 10, QChar('0'));
					QFileInfo fiberTimeCsvInfo(timeInfo.absoluteFilePath() + "/" + fiberTimeCsv);
					if (!fiberTimeCsvInfo.exists())
					{
						DEBUG_LOG(QString("File '%1' does not exist!").arg(fiberTimeCsv));
						curData.timeData = iAFiberCharData::NoTimeData;
						break;
					}
					auto & projErrFib = curData.projectionError[curFiber]; //.resize(valueStrList.size());
					std::vector<std::vector<double> > singleFiberValues;
					QFile file(fiberTimeCsvInfo.absoluteFilePath());
					if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
					{
						DEBUG_LOG(QString("Unable to open file: %1").arg(file.errorString()));
						curData.timeData = iAFiberCharData::NoTimeData;
						break;
					}
					QTextStream in(&file);
					size_t lineNr = 0;
					while (!in.atEnd())
					{
						lineNr++;
						QString line = in.readLine();
						QStringList values = line.split(",");
						if ((values.size()-1) %3 != 0)
						{
							DEBUG_LOG(QString("Invalid line %1 in file %2: The number of entries should be divisible by 3, but it's %3 (line: %4)")
								.arg(lineNr).arg(fiberTimeCsvInfo.fileName()).arg(values.size()).arg(line));
							continue;
						}
						std::vector<double> timeStepValues;
						bool ok;
						double projError = values[0].toDouble(&ok);
						if (!ok)
						{
							DEBUG_LOG(QString("Invalid line %1 in file %2: projection error value %3 is not double!")
								.arg(lineNr).arg(fiberTimeCsvInfo.fileName()).arg(values[0]));
						}
						projErrFib.push_back(projError);

						for (int j = 1; j < values.size(); ++j)
						{
							double curCoord = values[j].toDouble(&ok);
							if (!ok)
							{
								DEBUG_LOG(QString("Invalid line %1 in file %2: coordinate value %3 (%4th in line) is not double!")
									.arg(lineNr).arg(fiberTimeCsvInfo.fileName()).arg(values[j]).arg(j));
							}
							timeStepValues.push_back(curCoord);
						}
						singleFiberValues.push_back(timeStepValues);
					}
					assert(singleFiberValues.size() > 0);
					if (singleFiberValues.size() > thisResultTimeStepMax)
					{
						thisResultTimeStepMax = singleFiberValues.size();
					}
					fiberTimeValues.push_back(singleFiberValues);
				}
			}
			// transform from [fiber, timestep, value] to [timestep, fiber, value] indexing
			// TODO: make sure all datasets have the same max timestep count!
			if (curData.timeData != iAFiberCharData::NoTimeData)
			{
				curData.timeValues.resize(thisResultTimeStepMax);
				for (int t = 0; t < thisResultTimeStepMax; ++t)
				{
					curData.timeValues[t].resize(curData.fiberCount);
					for (int f = 0; f < curData.fiberCount; ++f)
					{
						curData.timeValues[t][f] = (t < fiberTimeValues[f].size()) ? fiberTimeValues[f][t] : fiberTimeValues[f][fiberTimeValues[f].size() - 1];
					}
				}
			}
			else
			{
				noCurvedFiberFiles.append(csvFile);
			}
		}

		QString curvedFileName(QFileInfo(csvFile).absolutePath() + "/curved/" + QFileInfo(csvFile).baseName() + "-CurvedFibrePoints.csv");
		if (readCurvedFiberInfo(curvedFileName, curData.curveInfo))
			curData.curvedFileName = curvedFileName;

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
		progress->emitProgress(resultID / csvFileNames.size());
		result.push_back(curData);
	}

	if (result.size() == 0)
	{
		DEBUG_LOG(QString("The specified folder %1 does not contain any valid csv files!").arg(path));
		return false;
	}
	if (noCurvedFiberFiles.size() > 0)
	{
		DEBUG_LOG(QString("\nThere seems to be curved fiber data available, "
			"but for files (%1) I could not find usable information; please check potential previous messages.\n"
			"FIAKER expects:\n"
			"Either a \"projection_error.csv\" and one \"fiberNNN_paramlog.csv\" per fiber "
			"(with NNN being a 3-digit identifier of the fiber ID, with leading zeros);\n"
			"OR a \"fiber_NNNN.csv\" per fiber for curved fiber timesteps "
			"(with NNN being a 4-digit identifier of the fiber ID, with leading zeros). "
			"In this format, each line specifies a time step, with values: "
			"projection error, firstX, firstY, firstZ, secondX, secondY, secondZ, ...")
			.arg(noCurvedFiberFiles.join(",")));
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

	auto similarityMeasures = iARefDistCompute::getSimilarityMeasureNames();
	for (auto name: similarityMeasures)
		paramNames.push_back(name);

	paramNames.push_back("Proj. Error Red.");
	paramNames.push_back("Result_ID");
	spmData->setParameterNames(paramNames);
	size_t numParams = spmData->numParams();
	size_t spmStartIdx = 0;
	for (resultID=0; resultID<result.size(); ++resultID)
	{
		auto & curData = result[resultID];
		size_t numTableColumns = curData.table->GetNumberOfColumns();
		for (int i = (iARefDistCompute::SimilarityMeasureCount+iAFiberCharData::FiberValueCount+iARefDistCompute::EndColumns); i >= iARefDistCompute::EndColumns; --i)
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
		for (int col = 0; col < (iARefDistCompute::SimilarityMeasureCount+ iAFiberCharData::FiberValueCount+iARefDistCompute::EndColumns-1); ++col)
		{
			addColumn(curData.table, 0, spmData->parameterName(numTableColumns+col).toStdString().c_str(), curData.fiberCount);
		}
		addColumn(curData.table, resultID, spmData->parameterName(numParams-1).toStdString().c_str(), curData.fiberCount);

		spmStartIdx += curData.fiberCount;
	}

	spmData->updateRanges();

	return true;
}

iAFiberResultsLoader::iAFiberResultsLoader(QSharedPointer<iAFiberResultsCollection> results,
	QString const & path, iACsvConfig const & config, double stepShift):
	m_results(results),
	m_path(path),
	m_config(config),
	m_stepShift(stepShift)
{}

void iAFiberResultsLoader::run()
{
	if (!m_results->loadData(m_path, m_config, m_stepShift, &m_progress))
		emit failed(m_path);
	else
		emit success();
}

iAProgress* iAFiberResultsLoader::progress()
{
	return &m_progress;
}
