// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAFiberResult.h"

#include "iAFiberData.h"
#include "iARefDistCompute.h" // only for SimilarityMeasureCount!

#include "iACsvIO.h"
#include "iACsvVtkTableCreator.h"

#include <iASPLOMData.h>
#include <iALog.h>
#include <iAFileUtils.h> // for FindFiles

#include <vtkFloatArray.h>
#include <vtkTable.h>

#include <QDataStream>
#include <QFileInfo>
#include <QSettings>
#include <QTextStream>

#include <algorithm> // for std::fill

bool operator<(iAFiberSimilarity const & a, iAFiberSimilarity const & b)
{
	return a.dissimilarity < b.dissimilarity;
}

QDataStream &operator<<(QDataStream &out, const iAFiberSimilarity &s)
{
	out << s.index;
	out << s.dissimilarity;
	return out;
}

QDataStream &operator>>(QDataStream &in, iAFiberSimilarity &s)
{
	in >> s.index;
	in >> s.dissimilarity;
	return in;
}

QDataStream &operator<<(QDataStream &out, const iARefDiffFiberStepData &s)
{
	out << s.step;
	return out;
}

QDataStream &operator>>(QDataStream &in, iARefDiffFiberStepData &s)
{
	in >> s.step;
	return in;
}

QDataStream &operator<<(QDataStream &out, const iARefDiffFiberData &s)
{
	out << s.diff;
	out << s.dist;
	return out;
}

QDataStream &operator>>(QDataStream &in, iARefDiffFiberData &s)
{
	in >> s.diff;
	in >> s.dist;
	return in;
}

const QString iAFiberResultsCollection::FiakerFCPFormat("FIAKER FCP (same as FCP format, but no headers)");
const QString iAFiberResultsCollection::SimpleFormat("FIAKER Simple (no header, center coords, phi, theta, length)");

namespace
{
	const double SimpleConfigCoordShift = 74.5;

	iACsvConfig getFiakerFCPConfig()
	{
		iACsvConfig config = iACsvConfig::getFCPFiberFormat("");
		config.skipLinesStart = 0;
		config.containsHeader = false;
		config.visType = iAObjectVisType::Cylinder;
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
		config.objectType = iAObjectType::Fibers;
		config.computeLength = false;
		config.computeAngles = false;
		config.computeTensors = false;
		config.computeCenter = false;
		config.computeStartEnd = true;
		std::fill_n(config.offset, 3, SimpleConfigCoordShift);
		config.visType = iAObjectVisType::Cylinder;
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
		config.isDiameterFixed = true;
		config.fixedDiameterValue = 7;
		return config;
	}

	iAAABB computeFiberBBox(std::vector<iAVec3f> const& points, float radius)
	{
		iAAABB result;
		for (size_t p = 0; p < points.size(); ++p)
		{
			auto const& pt = points[p];
			result.addPointToBox(pt - radius);
			result.addPointToBox(pt + radius);
		}
		return result;
	}
	iAFiberData createFiberData(iAFiberResult const& result, size_t fiberID)
	{
		auto const& mapping = *result.objData->m_colMapping.get();
		auto it = result.objData->m_curvedFiberData.find(fiberID);
		return iAFiberData(
			result.objData->m_table, fiberID, mapping, (it != result.objData->m_curvedFiberData.end()) ? it->second : std::vector<iAVec3f>());
	}

	iAAABB boundingBoxForFiber(iAFiberData const& fiberData)
	{
		return computeFiberBBox(
			fiberData.curvedPoints.size() > 0 ? fiberData.curvedPoints : fiberData.pts, fiberData.diameter / 2.0);
	}

	void createResultFiberData(iAFiberResult& result)
	{
		result.fiberData.resize(result.fiberCount);
		for (size_t fiberID = 0; fiberID < result.fiberCount; ++fiberID)
		{
			result.fiberData[fiberID] = createFiberData(result, fiberID);
		}
	}

	void boundingBoxesForFibers(std::vector<iAAABB>& fiberBBs, std::vector<iAFiberData> const& resultFiberData)
	{
		fiberBBs.resize(resultFiberData.size());
		for (size_t fiberID = 0; fiberID < fiberBBs.size(); ++fiberID)
		{
			fiberBBs[fiberID] = boundingBoxForFiber(resultFiberData[fiberID]);
		}
	}
}

iACsvConfig getCsvConfig(QString const & formatName)
{
	iACsvConfig result;
	QSettings settings;
	if (!result.load(settings, formatName))
	{
		if (formatName == iACsvConfig::FCPFiberFormat)
		{
			result = iACsvConfig::getFCPFiberFormat("");
		}
		else if (formatName == iACsvConfig::FCVoidFormat)
		{
			result = iACsvConfig::getFCVoidFormat("");
		}
		else if (formatName == iAFiberResultsCollection::FiakerFCPFormat)
		{
			result = getFiakerFCPConfig();
		}
		else if (formatName == iAFiberResultsCollection::SimpleFormat)
		{
			result = getSimpleConfig();
		}
		else
		{
			LOG(lvlError, QString("Invalid format %1!").arg(formatName));
		}
	}
	return result;
}

void addColumn(vtkSmartPointer<vtkTable> table, double value, char const * columnName, size_t numRows)
{
	auto arrX = vtkSmartPointer<vtkFloatArray>::New();
	arrX->SetName(columnName);
	arrX->SetNumberOfValues(numRows);
	arrX->Fill(value);
	table->AddColumn(arrX);
}

void mergeBoundingBoxes(iAAABB& bbox, std::vector<iAAABB> const& fiberBBs)
{
	if (fiberBBs.size() == 0)
	{
		LOG(lvlWarn, "Invalid call to mergeBoundingBoxes: empty list of bounding boxes!");
		return;
	}
	bbox = fiberBBs[0];
	for (size_t bbID = 1; bbID < fiberBBs.size(); ++bbID)
	{
		bbox.merge(fiberBBs[bbID]);
	}
}



iAFiberResultsCollection::iAFiberResultsCollection():
	spmData(new iASPLOMData()),
	minFiberCount(std::numeric_limits<size_t>::max()),
	maxFiberCount(0),
	optimStepMax(1),
	stepShift(0),
	m_resultIDColumn(0),
	m_projectionErrorColumn(0)
{}

bool iAFiberResultsCollection::loadData(QString const & path, iACsvConfig const & cfg, double newStepShift, iAProgress * progress, bool & abort)
{
	folder = path;
	stepShift = newStepShift;
	QStringList filters;
	filters << "*.csv";
	QStringList csvFileNames;

	findFiles(path, filters, false, csvFileNames, Files);

	const int MaxDatasetCount = 1000;
	if (csvFileNames.size() > MaxDatasetCount)
	{
		LOG(lvlError, QString("The specified folder %1 contains %2 datasets; currently we only support loading up to %3 datasets!")
			.arg(path).arg(csvFileNames.size()).arg(MaxDatasetCount));
		return false;
	}
	int loadedResults = 0;
	std::vector<QString> paramNames;

	QStringList noStepFiberFiles;
	QString stepInfoErrorMsgs;
	size_t totalFiberCount = 0;
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
			LOG(lvlError, QString("Could not load file '%1' - probably it's in a wrong format; skipping!").arg(csvFile));
			continue;
		}
		iAFiberResult curData;
		curData.objData = std::make_shared<iAObjectsData>(QFileInfo(csvFile).completeBaseName(), objectType, tableCreator.table(), io.outputMapping());
		curData.fiberCount = curData.objData->m_table->GetNumberOfRows();
		totalFiberCount += curData.fiberCount;
		if (curData.fiberCount > std::numeric_limits<int>::max())
		{
			LOG(lvlError, QString("Large number of objects (%1) detected - currently only up to %2 objects are supported!")
				.arg(curData.fiberCount).arg(std::numeric_limits<int>::max()));
		}
		curData.fileName = csvFile;
		if (curData.fiberCount < minFiberCount)
		{
			minFiberCount = curData.fiberCount;
		}
		if (curData.fiberCount > maxFiberCount)
		{
			maxFiberCount = curData.fiberCount;
		}

		if (result.empty())
		{
			for (int h = 0; h < io.outputHeaders().size(); ++h)
			{
				paramNames.push_back(io.outputHeaders()[h]);
			}
		}
		else
		{
			// Check if output mapping is the same (it must be)!
			for (auto key : result[0].objData->m_colMapping->keys())
			{
				if (curData.objData->m_colMapping->value(key) != result[0].objData->m_colMapping->value(key))
				{
					LOG(lvlError, QString("Mapping does not match for result %1, column %2!").arg(csvFile).arg(curData.objData->m_colMapping->value(key)));
					return false;
				}
			}
			// (though actually same mapping should be guaranteed by using same config)
		}

		QString stepInfoPath(QFileInfo(csvFile).absolutePath() + "/" + QFileInfo(csvFile).completeBaseName());
		QFileInfo stepInfo(stepInfoPath);

		// TODO: in case reading gets inefficient, look at pre-reserving the required amount of fields
		//       and using std::vector::swap to assign the sub-vectors!

		size_t thisResultStepMax = 1;
		curData.stepData = iAFiberResult::NoStepData;
		if (stepInfo.exists() && stepInfo.isDir())
		{
			// LOG(lvlInfo, "Looking for optimization step info in old format...");
			// read projection error info:
			QFile projErrorFile(stepInfo.absoluteFilePath() + "/projection_error.csv");

			// fiber, step, value
			std::vector<std::vector<std::vector<double> > > fiberStepValues;

			if (!projErrorFile.open(QIODevice::ReadOnly | QIODevice::Text))
			{
				stepInfoErrorMsgs += QString("Unable to open projection error file '%1': %2.\n")
					.arg(stepInfo.absoluteFilePath() + "/projection_error.csv")
					.arg(projErrorFile.errorString());
			}
			else
			{
				curData.projectionError.resize(curData.fiberCount);
				QTextStream inProjError(&projErrorFile);
				size_t fiberID = 0;
				while (!inProjError.atEnd())
				{
					QString line = inProjError.readLine();
					QStringList valueStrList = line.split(",");
					if (valueStrList.size() < 2)
					{
						continue;
					}
					if (fiberID >= curData.fiberCount)
					{
						LOG(lvlError, QString("Discrepancy: More lines in %1 file than there were fibers in the fiber description csv (%2).\n")
								  .arg(projErrorFile.fileName()).arg(curData.fiberCount));
						break;
					}
					auto & projErrFib = curData.projectionError[fiberID]; //.resize(valueStrList.size());
					for (int i = 0; i < valueStrList.size(); ++i)
					{
						if (valueStrList[i] == "nan")
						{
							break;
						}
						projErrFib.push_back(valueStrList[i].toDouble());
					}
					for (int i = 0; i < projErrFib.size(); ++i)
					{
						projErrFib[i] -= projErrFib[projErrFib.size() - 1];
					}
					++fiberID;
				}

				for (int curFiber=0; static_cast<size_t>(curFiber) < curData.fiberCount; ++curFiber)
				{
					QString fiberStepCsv = QString("fiber%1_paramlog.csv").arg(curFiber, 3, 10, QChar('0'));
					QFileInfo fiberStepCsvInfo(stepInfo.absoluteFilePath() + "/" + fiberStepCsv);
					if (!fiberStepCsvInfo.exists())
					{
						stepInfoErrorMsgs += QString("File '%1' does not exist.\n")
							.arg(fiberStepCsvInfo.absoluteFilePath());
						break;
					}
					std::vector<std::vector<double> > singleFiberValues;
					QFile fileFiberStepCsv(fiberStepCsvInfo.absoluteFilePath());
					if (!fileFiberStepCsv.open(QIODevice::ReadOnly | QIODevice::Text))
					{
						stepInfoErrorMsgs += QString("Unable to open file '%1': %2\n")
							.arg(fiberStepCsvInfo.absoluteFilePath())
							.arg(fileFiberStepCsv.errorString());
						break;
					}
					QTextStream inFiberStepCsv(&fileFiberStepCsv);
					inFiberStepCsv.readLine(); // skip header line
					size_t lineNr = 1;
					while (!inFiberStepCsv.atEnd())
					{
						lineNr++;
						QString line = inFiberStepCsv.readLine();
						QStringList values = line.split(",");
						if (values.size() != 6)
						{
							LOG(lvlError, QString("Invalid line %1 in file %2, there should be 6 entries but there are %3 (line: %4).\n")
								.arg(lineNr)
								.arg(fiberStepCsvInfo.fileName())
								.arg(values.size())
								.arg(line));
							continue;
						}
						double middlePoint[3];
						for (int i = 0; i < 3; ++i)
						{
							middlePoint[i] = values[i].toDouble() + stepShift; // middle point positions are shifted!
						}
						double theta = values[4].toDouble();
						if (theta < 0)  // theta is encoded in -Pi, Pi instead of 0..Pi as we expect
						{
							theta = 2 * vtkMath::Pi() + theta;
						}
						double phi = values[3].toDouble();
						double radius = values[5].toDouble() * 0.5;

						static const int StepFiberValuesCount = 13;
						std::vector<double> stepValues(StepFiberValuesCount);
						// convert spherical to cartesian coordinates:
						double dir[3];
						dir[0] = radius * std::sin(phi) * std::cos(theta);
						dir[1] = radius * std::sin(phi) * std::sin(theta);
						dir[2] = radius * std::cos(phi);
						for (int i = 0; i<3; ++i)
						{
							stepValues[i] = middlePoint[i] + dir[i];
							stepValues[i+3] = middlePoint[i] - dir[i];
							stepValues[i+6] = middlePoint[i];
						}
						stepValues[9] = phi;
						stepValues[10] = theta;
						stepValues[11] = values[5].toDouble();
						stepValues[12] = curData.objData->m_table->GetValue(curFiber, (*curData.objData->m_colMapping)[iACsvConfig::Diameter]).ToDouble();
						/*
						LOG(lvlInfo, QString("Fiber %1, step %2: Start (%3, %4, %5) - End (%6, %7, %8)")
							.arg(curFiber)
							.arg(singleFiberValues.size())
							.arg(stepValues[0]).arg(stepValues[1]).arg(stepValues[2])
							.arg(stepValues[3]).arg(stepValues[4]).arg(stepValues[5]));
						*/
						singleFiberValues.push_back(stepValues);
					}
					assert(singleFiberValues.size() > 0);
					if (singleFiberValues.size() > thisResultStepMax)
					{
						thisResultStepMax = singleFiberValues.size();
					}
					fiberStepValues.push_back(singleFiberValues);
				}
				if (fiberStepValues.size() == curData.fiberCount)
				{
					curData.stepData = iAFiberResult::SimpleStepData;
				}
				else
				{
					stepInfoErrorMsgs += QString("OLD format loader: Expected to load steps for %1 fibers, but only got information for %2.\n")
						.arg(curData.fiberCount)
						.arg(fiberStepValues.size());
				}
			}

			if (curData.stepData == iAFiberResult::NoStepData)
			{
				//LOG(lvlInfo, "Looking for optimization step info in new (curved) format...");
				// check if we can load new, curved step data:
				curData.projectionError.resize(curData.fiberCount);
				for (int curFiber = 0; static_cast<size_t>(curFiber) < curData.fiberCount; ++curFiber)
				{
					QString fiberStepCsv = QString("fiber_%1.csv").arg(curFiber, 4, 10, QChar('0'));
					QFileInfo fiberStepCsvInfo(stepInfo.absoluteFilePath() + "/" + fiberStepCsv);
					if (!fiberStepCsvInfo.exists())
					{
						stepInfoErrorMsgs += QString("File '%1' does not exist.\n").arg(fiberStepCsv);
						break;
					}
					auto & projErrFib = curData.projectionError[curFiber]; //.resize(valueStrList.size());
					std::vector<std::vector<double> > singleFiberValues;
					QFile file(fiberStepCsvInfo.absoluteFilePath());
					if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
					{
						stepInfoErrorMsgs += QString("Unable to open file '%1': %2.\n")
							.arg(fiberStepCsvInfo.absoluteFilePath())
							.arg(file.errorString());
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
							LOG(lvlError, QString("Invalid line %1 in file %2: The number of entries should be divisible by 3, but it's %3 (line: %4).")
								.arg(lineNr).arg(fiberStepCsvInfo.fileName()).arg(values.size()).arg(line));
						}
						std::vector<double> stepValues;
						bool ok;
						double projError = values[0].toDouble(&ok);
						if (!ok)
						{
							LOG(lvlError, QString("Invalid line %1 in file %2: projection error value %3 is not double!")
								.arg(lineNr).arg(fiberStepCsvInfo.fileName()).arg(values[0]));
						}
						projErrFib.push_back(projError);

						for (int j = 1; j < values.size(); ++j)
						{
							double curCoord = values[j].toDouble(&ok);
							if (!ok)
							{
								LOG(lvlError, QString("Invalid line %1 in file %2: coordinate value %3 (%4th in line) is not double!")
									.arg(lineNr).arg(fiberStepCsvInfo.fileName()).arg(values[j]).arg(j));
							}
							stepValues.push_back(curCoord);
						}
						singleFiberValues.push_back(stepValues);
					}
					assert(singleFiberValues.size() > 0);
					if (singleFiberValues.size() > thisResultStepMax)
					{
						thisResultStepMax = singleFiberValues.size();
					}
					fiberStepValues.push_back(singleFiberValues);
				}
				if (fiberStepValues.size() == curData.fiberCount)
				{
					curData.stepData = iAFiberResult::CurvedStepData;
				}
				else
				{
					stepInfoErrorMsgs += QString("NEW format (curved step info) loader: Expected to load steps for %1 fibers, but only got information for %2.\n")
						.arg(curData.fiberCount)
						.arg(fiberStepValues.size());
				}
			}
			// transform from [fiber, step, value] to [step, fiber, value] indexing
			// TODO: make sure all datasets have the same max step count!
			if (curData.stepData != iAFiberResult::NoStepData)
			{
				curData.stepValues.resize(thisResultStepMax);
				for (size_t t = 0; t < thisResultStepMax; ++t)
				{
					curData.stepValues[t].resize(curData.fiberCount);
					for (size_t f = 0; f < curData.fiberCount; ++f)
					{
						curData.stepValues[t][f] = (t < fiberStepValues[f].size()) ?
							fiberStepValues[f][t] :
							fiberStepValues[f][fiberStepValues[f].size() - 1];
					}
				}
			}
			else
			{
				noStepFiberFiles.append(csvFile);
			}
		}

		QString curvedFileName(QFileInfo(csvFile).absolutePath() + "/curved/" + QFileInfo(csvFile).completeBaseName() + "-CurvedFibrePoints.csv");
		if (readCurvedFiberInfo(curvedFileName, curData.objData->m_curvedFiberData))
		{
			curData.curvedFileName = curvedFileName;
		}

		if (thisResultStepMax > optimStepMax)
		{
			if (optimStepMax > 1)
			{
				LOG(lvlInfo, QString("Result %1 has a new maximum number of steps %2 (was %3).")
					.arg(loadedResults).arg(thisResultStepMax).arg(optimStepMax));
			}
			optimStepMax = thisResultStepMax;
		}
		++loadedResults;
		progress->emitProgress(loadedResults * 100.0 / csvFileNames.size());
		if (abort)
		{
			return false;
		}
		result.push_back(curData);
	}

	if (result.size() == 0)
	{
		LOG(lvlError, QString("The specified folder %1 does not contain any valid csv files!").arg(path));
		return false;
	}
	if (noStepFiberFiles.size() > 0)
	{
		LOG(lvlWarn, QString("\nThere seems to be fiber data available for optimization/iteration steps, "
			"but for files (%1) I could not find usable information. "
			"Maybe this debug output is helpful: '%2'.\n"
			"FIAKER expects:\n"
			"Either a \"projection_error.csv\" and one \"fiberNNN_paramlog.csv\" per fiber "
			"(with NNN being a 3-digit identifier of the fiber ID, with leading zeros);\n"
			"OR a \"fiber_NNNN.csv\" per fiber for curved fiber steps "
			"(with NNN being a 4-digit identifier of the fiber ID, with leading zeros). "
			"In this format, each line specifies a step, with values: "
			"projection error, firstX, firstY, firstZ, secondX, secondY, secondZ, ...")
			.arg(noStepFiberFiles.join(","))
			.arg(stepInfoErrorMsgs)
		);
	}

	progress->setStatus("Creating SPM table...");
	paramNames.push_back("Result_ID");
	paramNames.push_back("Proj. Error Red.");

	spmData->setParameterNames(paramNames, totalFiberCount);
	size_t numParams = spmData->numParams();
	//size_t spmStartIdx = 0;
	m_resultIDColumn = static_cast<uint>(numParams) - 2;
	m_projectionErrorColumn = static_cast<uint>(numParams) - 1;
	for (size_t resultID=0; resultID < result.size() && !abort; ++resultID)
	{
		auto & curData = result[resultID];
		vtkIdType numTableColumns = curData.objData->m_table->GetNumberOfColumns();

		addColumn(curData.objData->m_table, resultID, spmData->parameterName(m_resultIDColumn).toStdString().c_str(), curData.fiberCount);
		addColumn(curData.objData->m_table, resultID, spmData->parameterName(m_projectionErrorColumn).toStdString().c_str(), curData.fiberCount);
		for (size_t fiberID = 0; fiberID < curData.fiberCount; ++fiberID)
		{
			//size_t spmFiberID = spmStartIdx + fiberID;
			for (vtkIdType col = 0; col < numTableColumns; ++col)
			{
				double value = curData.objData->m_table->GetValue(fiberID, col).ToDouble();
				spmData->data()[col].push_back(value);
			}
			spmData->data()[m_resultIDColumn].push_back(resultID);

			double projErrorRed = curData.projectionError.size() > 0 ?
				curData.projectionError[fiberID][0] - curData.projectionError[fiberID][curData.projectionError[fiberID].size() - 1]
					: 0;
			spmData->data()[m_projectionErrorColumn].push_back(projErrorRed);

			curData.objData->m_table->SetValue(fiberID, m_resultIDColumn, resultID);
			curData.objData->m_table->SetValue(fiberID, m_projectionErrorColumn, projErrorRed);
		}
		// TODO: reuse spmData also for 3d visualization?
		//spmStartIdx += curData.fiberCount;

		progress->emitProgress(resultID * 100.0 / result.size());
	}
	spmData->updateRanges();

	progress->setStatus("Creating fiber data objects");
	for (size_t resultID = 0; resultID < result.size() && !abort; ++resultID)
	{
		createResultFiberData(result[resultID]);
		progress->emitProgress(resultID * 100 / result.size());
	}

	progress->setStatus("Computing bounding boxes");
	for (size_t resultID = 0; resultID < result.size() && !abort; ++resultID)
	{
		boundingBoxesForFibers(result[resultID].fiberBB, result[resultID].fiberData);
		progress->emitProgress(resultID * 100 / result.size());
		mergeBoundingBoxes(result[resultID].bbox, result[resultID].fiberBB);
	}
	return !abort;
}

iAFiberResultsLoader::iAFiberResultsLoader(std::shared_ptr<iAFiberResultsCollection> results,
	QString const & path, iACsvConfig const & config, double stepShift):
	m_results(results),
	m_path(path),
	m_config(config),
	m_stepShift(stepShift),
	m_aborted(false)
{}

void iAFiberResultsLoader::run()
{
	if (!m_results->loadData(m_path, m_config, m_stepShift, &m_progress, m_aborted))
	{
		emit failed(m_path);
	}
	else
	{
		emit success();
	}
}

void iAFiberResultsLoader::abort()
{
	m_aborted = true;
}

bool iAFiberResultsLoader::isAborted() const
{
	return m_aborted;
}

iAProgress* iAFiberResultsLoader::progress()
{
	return &m_progress;
}
