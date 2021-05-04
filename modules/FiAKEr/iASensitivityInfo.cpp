/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iASensitivityInfo.h"

// Charts
#include <iASPLOMData.h>
#include <iAScatterPlotWidget.h>
#include <iAScatterPlotViewData.h>

#include <iAFileUtils.h>
#include <iAColorTheme.h>
#include <iAJobListView.h>
#include <iALog.h>
#include <iALUT.h>
#include <iAMathUtility.h>
#include <iAPerformanceHelper.h>
#include <iARunAsync.h>
#include <iAStackedBarChart.h>    // for add HeaderLabel
#include <iAStringHelper.h>
#include <iAVec3.h>

// guibase:
#include <qthelper/iAQTtoUIConnector.h>
#include <qthelper/iAWidgetSettingsMapper.h>

// qthelper:
#include <iADockWidgetWrapper.h>

// objectvis:
#include <iA3DColoredPolyObjectVis.h>

// FeatureScout
#include "iACsvVectorTableCreator.h"

// Segmentation
#include "iAVectorTypeImpl.h"
#include "iAVectorDistanceImpl.h"

// FIAKER
#include "iAAlgorithmInfo.h"
#include "iAFiberCharData.h"
#include "iAFiberCharUIData.h"
#include "iAFiberData.h"
#include "iAMeasureSelectionDlg.h"
#include "iAMultidimensionalScaling.h"
#include "iAParameterInfluenceView.h"
#include "iARefDistCompute.h"
#include "iASensitivityDialog.h"
#include "ui_DissimilarityMatrix.h"
#include "ui_SensitivitySettings.h"

#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkSmartPointer.h>
#include <vtkTable.h>
#include <vtkVariant.h>

// for mesh differences:
// {
#include "iARendererViewSync.h"
//#include "vtkPolyDataBooleanFilter.h"

#include <vtkActor.h>
#include <vtkCornerAnnotation.h>
//#include <vtkCleanPolyData.h>
//#include <vtkBooleanOperationPolyDataFilter.h>
#include <vtkRendererCollection.h>
#include <vtkTextProperty.h>
//#include <vtkTriangleFilter.h>

// for sampled points display:
#include <vtkVertexGlyphFilter.h>
#include <vtkUnsignedCharArray.h>
#include <vtkPointData.h>
#include <vtkProperty.h>
// }

#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QLabel>
#include <QMainWindow>
#include <QMessageBox>
#include <QRadioButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QTableView>
#include <QtConcurrent>
#include <QTextStream>
#include <QVBoxLayout>

#include <array>
#include <set>

QDataStream& operator<<(QDataStream& out, const iAResultPairInfo& pairInfo)
{
	out << pairInfo.avgDissim;
	out << pairInfo.fiberDissim;
	return out;
}

QDataStream& operator>>(QDataStream& in, iAResultPairInfo& pairInfo)
{
	in >> pairInfo.avgDissim;
	in >> pairInfo.fiberDissim;
	return in;
}

namespace
{
	const int LayoutSpacing = 4;
	const QString DefaultStackedBarColorTheme("Brewer Accent (max. 8)");
	QStringList const& AggregationNames()
	{
		static QStringList Names = QStringList() << "Mean left+right" << "Left only" << "Right only" << "Mean of all neighbours in STAR";
		return Names;
	}


	QColor ParamColor(150, 150, 255, 255);
	QColor OutputColor(255, 200, 200, 255);

	// needs to match definition in iAParameterInfluenceView. Maybe unify somewhere:
	QColor SelectedResultPlotColor(235, 184, 31, 255);

	QColor ScatterPlotPointColor(80, 80, 80, 128);

	const int SelectedAggregationMeasureIdx = 3;

	const int SPMDSXOffset          = 4;
	const int SPMDSYOffset          = 3;
	const int SPIDOffset            = 2;
	const int SPDissimilarityOffset = 1;
}

// Factor out as generic CSV reading class also used by iACsvIO?
bool readParameterCSV(QString const& fileName, QString const& encoding, QString const& columnSeparator,
	iACsvTableCreator& tblCreator, size_t resultCount, int skipColumns)
{
	LOG(lvlDebug, QString("Reading file %1, skip %2 columns").arg(fileName).arg(skipColumns));
	if (!QFile::exists(fileName))
	{
		LOG(lvlError, "Error loading csv file, file does not exist.");
		return false;
	}
	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		LOG(lvlError, QString("Unable to open file '%1': %2").arg(fileName).arg(file.errorString()));
		return false;
	}
	QTextStream in(&file);
#if QT_VERSION < QT_VERSION_CHECK(5, 99, 0)
	in.setCodec(encoding.toStdString().c_str());
#else
	auto encOpt = QStringConverter::encodingForName(encoding.toStdString().c_str());
	QStringConverter::Encoding enc = encOpt.has_value() ? encOpt.value() : QStringConverter::Utf8;
	in.setEncoding(enc);
#endif
	auto headers = in.readLine().split(columnSeparator);
	for (int i = 0; i < skipColumns; ++i)
	{
		headers.removeAt(headers.size() - 1);
	}
	tblCreator.initialize(headers, resultCount);
	size_t row = 0;
	while (!in.atEnd() && row < resultCount)
	{
		QString line = in.readLine();
		if (line.trimmed().isEmpty()) // skip empty lines
		{
			continue;
		}
		tblCreator.addRow(row,
			stringToVector<std::vector<double>, double>(line, columnSeparator, headers.size(), false));
		++row;
	}
	// check for extra content at end - but skip empty lines:
	while (!in.atEnd())
	{
		QString line = in.readLine();
		if (!line.trimmed().isEmpty())
		{
			LOG(lvlError,
				QString("Line %1: Expected no more lines, or only empty ones, but got line '%2' instead!")
					.arg(row)
					.arg(line));
			return false;
		}
		++row;
	}
	return true;
}

using HistogramType = QVector<double>;

double distributionDifference(HistogramType const& distr1, HistogramType const& distr2, int diffType)
{
	assert(distr1.size() == distr2.size());
	QSharedPointer<iAVectorType> dist1Ptr(new iARefVector<HistogramType>(distr1));
	QSharedPointer<iAVectorType> dist2Ptr(new iARefVector<HistogramType>(distr2));
	if (diffType == 0)
	{
		/*
		// (start of) distance between AVERAGEs - (can that be useful?)
		// approximate average over all values by building sum weighted by histogram
		for (int i = 0; i < distr1.size(); ++i)
		{

		}
		*/
		iAL2NormDistance l2Dist;
		return l2Dist.GetDistance(dist1Ptr, dist2Ptr);
	}
	else if (diffType == 1)
	{
		iAJensenShannonDistance jsDist;
		return jsDist.GetDistance(dist1Ptr, dist2Ptr);
	}
	else
	{
		LOG(lvlError, QString("invalid diffType %1").arg(diffType));
		return 0;
	}
}

namespace
{
std::array<iAVec3f, 2> computeFiberBBox(std::vector<iAVec3f> const& points, float radius)
{
	std::array<iAVec3f, 2> result;
	result[0].fill(std::numeric_limits<float>::max());
	result[1].fill(std::numeric_limits<float>::lowest());
	for (int p = 0; p < points.size(); ++p)
	{
		auto const & pt = points[p].data();
		for (int i = 0; i < 3; ++i)
		{
			result[0][i] = std::min(result[0][i], pt[i] - radius);
			result[1][i] = std::max(result[1][i], pt[i] + radius);
		}
	}
	return result;
}
using FiberBBT = std::array<iAVec3f, 2>;

inline bool boundingBoxesIntersect(FiberBBT const& bb1, FiberBBT const& bb2)
{
	const int MinIdx = 0, MaxIdx = 1;
	return
		bb1[MaxIdx].x() > bb2[MinIdx].x() && bb2[MaxIdx].x() > bb1[MinIdx].x() &&
		bb1[MaxIdx].y() > bb2[MinIdx].y() && bb2[MaxIdx].y() > bb1[MinIdx].y() &&
		bb1[MaxIdx].z() > bb2[MinIdx].z() && bb2[MaxIdx].z() > bb1[MinIdx].z();
}

std::vector<int> intersectingBoundingBox(FiberBBT const& fixedFiberBB, std::vector<FiberBBT> const& otherFiberBBs)
{
	std::vector<int> resultIDs;
	for (int r = 0; r < otherFiberBBs.size(); ++r)
	{
		if (boundingBoxesIntersect(fixedFiberBB, otherFiberBBs[r]))
		{
			resultIDs.push_back(r);
		}
	}
	return resultIDs;
}

iAFiberData createFiberData(iAFiberCharData const& result, size_t fiberID)
{
	auto const& mapping = *result.mapping.data();
	auto it = result.curveInfo.find(fiberID);
	return iAFiberData(result.table, fiberID, mapping,
		(it != result.curveInfo.end()) ? it->second : std::vector<iAVec3f>());
}

std::vector<iAFiberData> createResultFiberData(iAFiberCharData const& result)
{
	std::vector<iAFiberData> fiberData(result.fiberCount);
	for (size_t fiberID=0; fiberID < result.fiberCount; ++fiberID)
	{
		fiberData[fiberID] = createFiberData(result, fiberID);
	}
	return fiberData;
}

FiberBBT boundingBoxForFiber(iAFiberData const & fiberData)
{
	return computeFiberBBox(fiberData.curvedPoints.size() > 0 ?
		fiberData.curvedPoints : fiberData.pts, fiberData.diameter / 2.0);
}

std::vector<FiberBBT> boundingBoxesForFibers(std::vector<iAFiberData> const & resultFiberData)
{
	std::vector<FiberBBT> fibersBBox(resultFiberData.size());
	for (size_t fiberID = 0; fiberID < fibersBBox.size(); ++fiberID)
	{
		fibersBBox[fiberID] = boundingBoxForFiber(resultFiberData[fiberID]);
	}
	return fibersBBox;
}

} // namespace

void iASensitivityInfo::abort()
{
	m_aborted = true;
}

QSharedPointer<iASensitivityInfo> iASensitivityInfo::create(QMainWindow* child,
	QSharedPointer<iAFiberResultsCollection> data, QDockWidget* nextToDW, int histogramBins, int skipColumns,
	std::vector<iAFiberCharUIData> const& resultUIs, iAVtkWidget* main3DWidget, QString parameterSetFileName,
	QVector<int> const & charSelected, QVector<int> const & charDiffMeasure, iASettings const & projectFile)
{
	if (parameterSetFileName.isEmpty())
	{
		parameterSetFileName = QFileDialog::getOpenFileName(child,
			"Sensitivity: Parameter Sets file", data->folder,
			"Comma-Separated Values (*.csv);;");
	}
	if (parameterSetFileName.isEmpty())
	{
		LOG(lvlInfo, "Empty parameter set filename / aborted.");
		return QSharedPointer<iASensitivityInfo>();
	}
	iACsvVectorTableCreator tblCreator;
	if (!readParameterCSV(parameterSetFileName, "UTF-8", ",", tblCreator, data->result.size(), skipColumns))
	{
		return QSharedPointer<iASensitivityInfo>();
	}
	auto const & paramValues = tblCreator.table();
	auto const& paramNames = tblCreator.header();
	// csv assumed to contain header line (names of parameters), and one row per parameter set;
	// parameter set contains an ID as first column and a filename as last row
	if (paramValues.size() <= 1 || paramValues[0].size() <= 3)
	{
		LOG(lvlError, QString("Invalid parameter set file: expected at least 2 data rows (actual: %1) "
			"and at least 2 columns (ID and one parameter; actual: %2")
			.arg(paramValues.size() > 0 ? paramValues[0].size() : -1)
			.arg(paramValues.size())
		);
		return QSharedPointer<iASensitivityInfo>();
	}
	// data in m_paramValues is indexed [col(=parameter index)][row(=parameter set index)]
	QSharedPointer<iASensitivityInfo> sensitivity(
		new iASensitivityInfo(data, parameterSetFileName, skipColumns, paramNames, paramValues, child, nextToDW, resultUIs, main3DWidget));

	// find min/max, for all columns
	sensitivity->m_paramMin.resize(static_cast<int>(paramValues.size()));
	sensitivity->m_paramMax.resize(static_cast<int>(paramValues.size()));
	//LOG(lvlInfo, QString("Parameter values size: %1x%2").arg(paramValues.size()).arg(paramValues[0].size()));
	for (int p = 0; p < paramValues.size(); ++p)
	{
		sensitivity->m_paramMin[p] = *std::min_element(paramValues[p].begin(), paramValues[p].end());
		sensitivity->m_paramMax[p] = *std::max_element(paramValues[p].begin(), paramValues[p].end());
	}

	// countOfVariedParams = number of parameters for which min != max (except column 0, which is the ID):
	for (int p = 1; p < sensitivity->m_paramMin.size(); ++p)
	{
		if (sensitivity->m_paramMin[p] != sensitivity->m_paramMax[p])
		{
			sensitivity->m_variedParams.push_back(p);
		}
	}
	if (sensitivity->m_variedParams.size() == 0)
	{
		LOG(lvlError, "Invalid sampling: No parameter was varied!");
		return QSharedPointer<iASensitivityInfo>();
	}

	//LOG(lvlInfo, QString("Found the following parameters to vary (number: %1): %2")
	//	.arg(sensitivity->m_variedParams.size())
	//	.arg(joinAsString(sensitivity->m_variedParams, ",", [&paramNames](int const& i) { return paramNames[i]; })));

	// find out how many additional parameter sets were added per STAR:
	//   - go to first value row; take value of first varied parameter as v
	//   - go down rows, as long as either
	//        first varied parameter has same value as v
	//        or distance of current value of first varied parameter is a multiple
	//        of the distance between its first row value and second row value
	double checkValue0 = paramValues[sensitivity->m_variedParams[0]][0];
	const double RemainderCheckEpsilon = 1e-12;
	double curCheckValue = paramValues[sensitivity->m_variedParams[0]][1];
	double diffCheck = std::abs(curCheckValue - checkValue0);
	//LOG(lvlDebug, QString("checkValue0=%1, curCheckValue=%2, diffCheck=%3").arg(checkValue0).arg(curCheckValue).arg(diffCheck));
	double remainder = 0;
	int row = 2;
	// first, continue, as long as first varied parameter is a multiple of diffCheck away from checkValue0:
	while (row < paramValues[sensitivity->m_variedParams[0]].size() &&
		!dblApproxEqual(curCheckValue, checkValue0) &&
		(remainder < RemainderCheckEpsilon || 	// "approximately a multiple" is not so easy with double
		(std::abs(diffCheck - remainder) < RemainderCheckEpsilon))) // remainder could also be close to but smaller than diffCheck
	{
		curCheckValue = paramValues[sensitivity->m_variedParams[0]][row];
		remainder = std::abs(std::fmod(std::abs(curCheckValue - checkValue0), diffCheck));
		//LOG(lvlDebug, QString("Row %1: curCheckValue=%2, remainder=%3")
		//	.arg(row).arg(curCheckValue).arg(remainder));
		++row;
	}
	// then, continue, as long as first varied parameter is (approximately) the same as checkValue0
	while (row < paramValues[sensitivity->m_variedParams[0]].size()  &&
		dblApproxEqual(curCheckValue, checkValue0) )
	{
		curCheckValue = paramValues[sensitivity->m_variedParams[0]][row];
		//LOG(lvlDebug, QString("Row %1: curCheckValue=%2").arg(row).arg(curCheckValue));
		++row;
	}

	sensitivity->m_starGroupSize = row - 1;
	sensitivity->m_numOfSTARSteps = (sensitivity->m_starGroupSize - 1) / sensitivity->m_variedParams.size();
	if (paramValues[0].size() % sensitivity->m_starGroupSize != 0)
	{
		LOG(lvlError, QString("Expected a number of STAR groups of size %1; "
			"but %2 (the number of samples) isn't divisible by that number!")
			.arg(sensitivity->m_starGroupSize).arg(paramValues[0].size()));
		return QSharedPointer<iASensitivityInfo>();
	}

	LOG(lvlDebug, QString("In %1 parameter sets, found %2 varying parameters, in STAR groups of %3 (parameter branch size: %4)")
		.arg(paramValues[0].size())
		.arg(sensitivity->m_variedParams.size())
		.arg(sensitivity->m_starGroupSize)
		.arg(sensitivity->m_numOfSTARSteps)
	);
	//LOG(lvlInfo,QString("Determined that there are groups of size: %1; number of STAR points per parameter: %2")
	//	.arg(sensitivity->m_starGroupSize)
	//	.arg(sensitivity->m_numOfSTARSteps)
	//);

	// select output features to compute sensitivity for:
	// - the loaded and computed ones (length, orientation, ...)
	// - dissimilarity measure(s)
	if (charSelected.isEmpty())
	{
		iASensitivityDialog dlg(data);
		if (dlg.exec() != QDialog::Accepted)
		{
			return QSharedPointer<iASensitivityInfo>();
		}
		sensitivity->m_charSelected = dlg.selectedCharacteristics();
		sensitivity->m_charDiffMeasure = dlg.selectedDiffMeasures();
	}
	else
	{
		sensitivity->m_charSelected = charSelected;
		sensitivity->m_charDiffMeasure = charDiffMeasure;
	}
	for (int j = sensitivity->m_charSelected.size() - 1; j >= 0; --j)
	{
		int charIdx = sensitivity->m_charSelected[j];
		// make sure of all histograms for the same characteristic have the same range
		if (data->spmData->paramRange(charIdx)[0] == data->spmData->paramRange(charIdx)[1])
		{
			LOG(lvlInfo,
				QString("Characteristic %1 does not vary, excluding from analysis!")
					.arg(data->spmData->parameterName(charIdx)));
			sensitivity->m_charSelected.remove(j);
		}
	}
	//sensitivity->dissimMeasure = dlg.selectedMeasures();
	sensitivity->m_histogramBins = histogramBins;
	if (sensitivity->m_charSelected.size() == 0 || sensitivity->m_charDiffMeasure.size() == 0)
	{
		QMessageBox::warning(child, "Sensitivity", "You have to select at least one characteristic and at least one measure!", QMessageBox::Ok);
		return QSharedPointer<iASensitivityInfo>();
	}
	if (!QFile::exists(sensitivity->dissimilarityMatrixCacheFileName()))
	{
		iAMeasureSelectionDlg selectMeasure;
		if (selectMeasure.exec() != QDialog::Accepted)
		{
			return QSharedPointer<iASensitivityInfo>();
		}
		sensitivity->m_resultDissimMeasures = selectMeasure.measures();
		sensitivity->m_resultDissimOptimMeasureIdx = selectMeasure.optimizeMeasureIdx();
	}
	sensitivity->m_projectToLoad = projectFile;
	auto futureWatcher = runAsync([sensitivity]
		{
			sensitivity->compute();
		},
		[sensitivity]
		{
			sensitivity->createGUI();
		}, child);
	iAJobListView::get()->addJob("Sensitivity computation", &sensitivity->m_progress, futureWatcher, sensitivity.data());
	return sensitivity;
}

iASensitivityInfo::iASensitivityInfo(QSharedPointer<iAFiberResultsCollection> data,
	QString const& parameterFileName, int skipColumns, QStringList const& paramNames,
	std::vector<std::vector<double>> const & paramValues, QMainWindow* child, QDockWidget* nextToDW,
	std::vector<iAFiberCharUIData> const & resultUIs, iAVtkWidget* main3DWidget) :
	m_data(data),
	m_paramNames(paramNames),
	m_paramValues(paramValues),
	m_parameterFileName(parameterFileName),
	m_skipColumns(skipColumns),
	m_child(child),
	m_nextToDW(nextToDW),
	m_aborted(false),
	m_resultUIs(resultUIs),
	m_main3DWidget(main3DWidget)
{
}

void getBestMatches2(iAFiberData const& fiber, std::vector<iAFiberData> const& otherFibers,
	QVector<QVector<iAFiberSimilarity>>& bestMatches, std::vector<int> const& candidates,
	double diagonalLength, double maxLength, std::vector<std::pair<int, bool>>& measuresToCompute)
{
	int bestMatchesStartIdx = bestMatches.size();
	assert(measuresToCompute.size() < std::numeric_limits<int>::max());
	assert(bestMatchesStartIdx + measuresToCompute.size() < std::numeric_limits<int>::max());
	int numOfNewMeasures = static_cast<int>(measuresToCompute.size());
	bestMatches.resize(bestMatchesStartIdx + numOfNewMeasures);
	auto maxNumberOfCloseFibers = std::min(static_cast<int>(candidates.size()),
		std::min(iARefDistCompute::MaxNumberOfCloseFibers,
			static_cast<iARefDistCompute::ContainerSizeType>(otherFibers.size())));
	for (int d = 0; d < numOfNewMeasures; ++d)
	{
		QVector<iAFiberSimilarity> similarities;
		similarities.resize(static_cast<int>(candidates.size()));
		for (iARefDistCompute::ContainerSizeType bestMatchID = 0; bestMatchID < candidates.size(); ++bestMatchID)
		{
			size_t refFiberID = candidates[bestMatchID];
			similarities[bestMatchID].index = static_cast<quint32>(refFiberID);
			double curDissimilarity =
				getDissimilarity(fiber, otherFibers[refFiberID], measuresToCompute[d].first, diagonalLength, maxLength);
			if (std::isnan(curDissimilarity))
			{
				curDissimilarity = 0;
			}
			similarities[bestMatchID].dissimilarity = curDissimilarity;
		}
		std::sort(similarities.begin(), similarities.end());
		std::copy(similarities.begin(), similarities.begin() + maxNumberOfCloseFibers,
			std::back_inserter(bestMatches[bestMatchesStartIdx + d]));
	}
}

void iASensitivityInfo::compute()
{
	m_progress.setStatus("Storing parameter set values");
	for (int p = 0; p < m_paramValues[0].size(); p += m_starGroupSize)
	{
		QVector<double> parameterSet;
		for (int v = 0; v < m_paramValues.size(); ++v)
		{
			parameterSet.push_back(m_paramValues[v][p]);
		}
		paramSetValues.push_back(parameterSet);
		m_progress.emitProgress(static_cast<int>(100 * p / m_paramValues[0].size()));
	}

	m_progress.setStatus("Computing characteristics distribution (histogram) for all results.");
	// TODO: common storage for that in data!
	m_charHistograms.resize(static_cast<int>(m_data->result.size()));

	int numCharSelected = m_charSelected.size();
	/*
	for (auto selCharIdx = 0; selCharIdx < m_charSelected.size(); ++selCharIdx)
	{
		double rangeMin = m_data->spmData->paramRange(m_charSelected[selCharIdx])[0];
		double rangeMax = m_data->spmData->paramRange(m_charSelected[selCharIdx])[1];
		LOG(lvlInfo, QString("Characteristic idx=%1, charIdx=%2 (%3): %4-%5")
			.arg(selCharIdx).arg(m_charSelected[selCharIdx]).arg(charactName(selCharIdx))
			.arg(rangeMin).arg(rangeMax));
	}
	*/

	for (int rIdx = 0; rIdx < m_data->result.size(); ++rIdx)
	{
		m_progress.emitProgress(static_cast<int>(100 * rIdx / m_data->result.size()));
		auto const& r = m_data->result[rIdx];
		// TODO: skip some columns? like ID...
		m_charHistograms[rIdx].reserve(numCharSelected);
		for (auto charIdx: m_charSelected)
		{
			// make sure of all histograms for the same characteristic have the same range
			double rangeMin = m_data->spmData->paramRange(charIdx)[0];
			double rangeMax = m_data->spmData->paramRange(charIdx)[1];
			std::vector<double> fiberData(r.fiberCount);
			for (size_t fiberID = 0; fiberID < r.fiberCount; ++fiberID)
			{
				fiberData[fiberID] = r.table->GetValue(fiberID, charIdx).ToDouble();
			}
			auto histogram = createHistogram(fiberData, m_histogramBins, rangeMin, rangeMax);
			m_charHistograms[rIdx].push_back(histogram);
		}
	}
	if (m_aborted)
	{
		return;
	}

	// for each characteristic
	//     for each varied parameter
	//         for each selected characteristics difference measure
	//             for each "aggregation type" - left only, right only, average/? over full range
	//                 for each point in parameter space (of base sampling method)
	//                     compute local change by that difference measure

	m_progress.setStatus("Computing characteristics sensitivities.");
	const int NumOfVarianceAggregation = 4;

	paramStep.fill(0.0, m_variedParams.size());
	sensitivityField.resize(numCharSelected);
	aggregatedSensitivities.resize(numCharSelected);
	for (int charIdx = 0; charIdx < numCharSelected && !m_aborted; ++charIdx)
	{
		m_progress.emitProgress(100 * charIdx / numCharSelected);
		//int charactID = m_charSelected[charIdx];
		//auto charactName = m_data->spmData->parameterName(charactID);
		//LOG(lvlDebug, QString("Characteristic %1 (%2):").arg(charIdx).arg(charactName));
		sensitivityField[charIdx].resize(m_charDiffMeasure.size());
		aggregatedSensitivities[charIdx].resize(m_charDiffMeasure.size());
		for (int diffMeasureIdx = 0; diffMeasureIdx < m_charDiffMeasure.size(); ++diffMeasureIdx)
		{
			//LOG(lvlDebug, QString("    Difference Measure %1 (%2)").arg(diffMeasure).arg(DistributionDifferenceMeasureNames()[diffMeasure]));
			auto& field = sensitivityField[charIdx][diffMeasureIdx];
			field.resize(NumOfVarianceAggregation);
			auto& agg = aggregatedSensitivities[charIdx][diffMeasureIdx];
			agg.resize(NumOfVarianceAggregation);
			for (int i = 0; i < NumOfVarianceAggregation; ++i)
			{
				field[i].resize(m_variedParams.size());
				agg[i].fill(0.0, m_variedParams.size());
			}
			for (int paramIdx = 0; paramIdx < m_variedParams.size(); ++paramIdx)
			{
				for (int i = 0; i < NumOfVarianceAggregation; ++i)
				{
					field[i][paramIdx].resize(paramSetValues.size());
				}
				// TODO: unify with other loops over STARs
				//QString paramName(m_paramNames[m_variedParams[paramIdx]]);
				//LOG(lvlDebug, QString("  Parameter %1 (%2):").arg(paramIdx).arg(paramName));
				int origParamColIdx = m_variedParams[paramIdx];
				// aggregation types:
				//     - for now: one step average, left only, right only, average over all steps
				//     - future: overall (weighted) average, values over multiples of step size, ...
				int numAllLeft = 0,
					numAllRight = 0,
					numAllLeftRight = 0,
					numAllTotal = 0;
				for (int paramSetIdx = 0; paramSetIdx < paramSetValues.size(); ++paramSetIdx)
				{
					int resultIdxGroupStart = m_starGroupSize * paramSetIdx;
					int resultIdxParamStart = resultIdxGroupStart + 1 + paramIdx * m_numOfSTARSteps;

					// first - then + steps (both skipped if value +/- step exceeds bounds
					double groupStartParamVal = m_paramValues[origParamColIdx][resultIdxGroupStart];
					double paramStartParamVal = m_paramValues[origParamColIdx][resultIdxParamStart];
					double paramDiff = paramStartParamVal - groupStartParamVal;
					//LOG(lvlDebug, QString("      Parameter Set %1; start: %2 (value %3), param start: %4 (value %5); diff: %6")
					//	.arg(paramSetIdx).arg(resultIdxGroupStart).arg(groupStartParamVal)
					//	.arg(resultIdxParamStart).arg(paramStartParamVal).arg(paramDiff));

					if (paramStep[paramIdx] == 0)
					{
						paramStep[paramIdx] = std::abs(paramDiff);
						LOG(lvlInfo, QString("Param Step (param %1): %2").arg(paramIdx).arg(paramStep[paramIdx]));
					}

					double leftVar = 0;
					int numLeftRight = 0;
					if (paramDiff > 0)
					{
						leftVar = distributionDifference(
							m_charHistograms[resultIdxGroupStart][charIdx],
							m_charHistograms[resultIdxParamStart][charIdx],
							m_charDiffMeasure[diffMeasureIdx]);
						//LOG(lvlDebug, QString("        Left var available: %1").arg(leftVar));
						++numLeftRight;
						++numAllLeft;
					}

					int k = 1;
					while (paramDiff > 0 && k < m_numOfSTARSteps)
					{
						double paramVal = m_paramValues[origParamColIdx][resultIdxParamStart + k];
						paramDiff = paramStartParamVal - paramVal;
						++k;
					}
					double rightVar = 0;
					if (paramDiff < 0) // additional check required??
					{
						int firstPosStepIdx = resultIdxParamStart + (k - 1);
						rightVar = distributionDifference(
							m_charHistograms[resultIdxGroupStart][charIdx],
							m_charHistograms[firstPosStepIdx][charIdx],
							m_charDiffMeasure[diffMeasureIdx]);
						//LOG(lvlDebug, QString("        Right var available: %1").arg(rightVar));
						++numLeftRight;
						++numAllRight;
					}
					double sumTotal = 0;
					bool wasSmaller = true;
					for (int i = 0; i < m_numOfSTARSteps; ++i)
					{
						int compareIdx = (i==0)? resultIdxGroupStart : (resultIdxParamStart + i - 1);
						double paramVal = m_paramValues[origParamColIdx][resultIdxParamStart + i];
						if (paramVal > paramStartParamVal && wasSmaller)
						{
							wasSmaller = false;
							compareIdx = resultIdxGroupStart;
						}
						double difference = distributionDifference(
							m_charHistograms[compareIdx][charIdx],
							m_charHistograms[resultIdxParamStart + i][charIdx],
							m_charDiffMeasure[diffMeasureIdx]);
						sumTotal += difference;
					}
					numAllLeftRight += numLeftRight;
					numAllTotal += m_numOfSTARSteps;
					double meanLeftRightVar = (leftVar + rightVar) / numLeftRight;
					double meanTotal = sumTotal / m_numOfSTARSteps;
					//LOG(lvlDebug, QString("        (left+right)/(numLeftRight=%1) = %2").arg(numLeftRight).arg(meanLeftRightVar));
					//LOG(lvlDebug, QString("        (sum total var = %1) / (m_numOfSTARSteps = %2)  = %3")
					//	.arg(sumTotal).arg(m_numOfSTARSteps).arg(meanTotal));
					field[0][paramIdx][paramSetIdx] = meanLeftRightVar;
					field[1][paramIdx][paramSetIdx] = leftVar;
					field[2][paramIdx][paramSetIdx] = rightVar;
					field[3][paramIdx][paramSetIdx] = meanTotal;

					agg[0][paramIdx] += meanLeftRightVar;
					agg[1][paramIdx] += leftVar;
					agg[2][paramIdx] += rightVar;
					agg[3][paramIdx] += meanTotal;
				}
				assert(numAllLeftRight == (numAllLeft + numAllRight));
				agg[0][paramIdx] /= numAllLeftRight;
				agg[1][paramIdx] /= numAllLeft;
				agg[2][paramIdx] /= numAllRight;
				agg[3][paramIdx] /= numAllTotal;
				//LOG(lvlDebug, QString("      LeftRight=%1, Left=%2, Right=%3, Total=%4")
				//	.arg(agg[0]).arg(agg[1]).arg(agg[2]).arg(agg[3]));
			}
		}
	}
	if (m_aborted)
	{
		return;
	}
	m_progress.setStatus("Computing fiber count histogram.");

	QVector<double> fiberCounts;
	for (int resultIdx = 0; resultIdx < m_data->result.size(); ++resultIdx)
	{
		fiberCounts.push_back(m_data->result[resultIdx].fiberCount);
	}
	m_fiberCountRange[0] = m_fiberCountRange[1] = std::numeric_limits<double>::infinity();
	fiberCountHistogram = createHistogram(fiberCounts, m_histogramBins,
		m_fiberCountRange[0], m_fiberCountRange[1], true);
	if (m_aborted)
	{
		return;
	}

	m_progress.setStatus("Computing fiber count sensitivities.");
	// TODO: unify with other loops over STARs
	sensitivityFiberCount.resize(NumOfVarianceAggregation);
	aggregatedSensitivitiesFiberCount.resize(NumOfVarianceAggregation);
	for (int i = 0; i < NumOfVarianceAggregation; ++i)
	{
		sensitivityFiberCount[i].resize(m_variedParams.size());
		aggregatedSensitivitiesFiberCount[i].fill(0.0, m_variedParams.size());
	}
	for (int paramIdx = 0; paramIdx < m_variedParams.size() && !m_aborted; ++paramIdx)
	{
		m_progress.emitProgress(100 * paramIdx / m_variedParams.size());
		int origParamColIdx = m_variedParams[paramIdx];
		for (int i = 0; i < NumOfVarianceAggregation; ++i)
		{
			sensitivityFiberCount[i][paramIdx].resize(paramSetValues.size());
		}
		int numAllLeft = 0,
			numAllRight = 0,
			numAllLeftRight = 0,
			numAllTotal = 0;
		for (int paramSetIdx = 0; paramSetIdx < paramSetValues.size(); ++paramSetIdx)
		{
			int resultIdxGroupStart = m_starGroupSize * paramSetIdx;
			int resultIdxParamStart = resultIdxGroupStart + 1 + paramIdx * m_numOfSTARSteps;

			// first - then + steps (both skipped if value +/- step exceeds bounds
			double groupStartParamVal = m_paramValues[origParamColIdx][resultIdxGroupStart];
			double paramStartParamVal = m_paramValues[origParamColIdx][resultIdxParamStart];
			double paramDiff = paramStartParamVal - groupStartParamVal;
			//LOG(lvlDebug, QString("      Parameter Set %1; start: %2 (value %3), param start: %4 (value %5); diff: %6")
			//	.arg(paramSetIdx).arg(resultIdxGroupStart).arg(groupStartParamVal)
			//	.arg(resultIdxParamStart).arg(paramStartParamVal).arg(paramDiff));

			if (paramStep[paramIdx] == 0)
			{
				paramStep[paramIdx] = std::abs(paramDiff);
			}

			double leftVar = 0;
			int numLeftRight = 0;
			if (paramDiff > 0)
			{
				leftVar = std::abs(static_cast<double>(m_data->result[resultIdxGroupStart].fiberCount)
					- m_data->result[resultIdxParamStart].fiberCount);
				//LOG(lvlDebug, QString("        Left var available: %1").arg(leftVar));
				++numLeftRight;
				++numAllLeft;
			}

			int k = 1;
			while (paramDiff > 0 && k < m_numOfSTARSteps)
			{
				double paramVal = m_paramValues[origParamColIdx][resultIdxParamStart + k];
				paramDiff = paramStartParamVal - paramVal;
				++k;
			}
			double rightVar = 0;
			if (paramDiff < 0) // additional check required??
			{
				int firstPosStepIdx = resultIdxParamStart + (k - 1);
				rightVar = std::abs(static_cast<double>(m_data->result[resultIdxGroupStart].fiberCount)
					- m_data->result[firstPosStepIdx].fiberCount);
				//LOG(lvlDebug, QString("        Right var available: %1").arg(rightVar));
				++numLeftRight;
				++numAllRight;
			}
			double sumTotal = 0;
			bool wasSmaller = true;
			for (int i = 0; i < m_numOfSTARSteps; ++i)
			{
				int compareIdx = (i == 0) ? resultIdxGroupStart : (resultIdxParamStart + i - 1);
				double paramVal = m_paramValues[origParamColIdx][resultIdxParamStart + i];
				if (paramVal > paramStartParamVal && wasSmaller)
				{
					wasSmaller = false;
					compareIdx = resultIdxGroupStart;
				}
				double difference = std::abs(static_cast<double>(m_data->result[compareIdx].fiberCount)
					- m_data->result[resultIdxParamStart + i].fiberCount);
				sumTotal += difference;
			}
			numAllLeftRight += numLeftRight;
			numAllTotal += m_numOfSTARSteps;
			double meanLeftRightVar = (leftVar + rightVar) / numLeftRight;
			double meanTotal = sumTotal / m_numOfSTARSteps;
			//LOG(lvlDebug, QString("        (left+right)/(numLeftRight=%1) = %2").arg(numLeftRight).arg(meanLeftRightVar));
			//LOG(lvlDebug, QString("        (sum total var = %1) / (m_numOfSTARSteps = %2)  = %3")
			//	.arg(sumTotal).arg(m_numOfSTARSteps).arg(meanTotal));
			sensitivityFiberCount[0][paramIdx][paramSetIdx] = meanLeftRightVar;
			sensitivityFiberCount[1][paramIdx][paramSetIdx] = leftVar;
			sensitivityFiberCount[2][paramIdx][paramSetIdx] = rightVar;
			sensitivityFiberCount[3][paramIdx][paramSetIdx] = meanTotal;

			aggregatedSensitivitiesFiberCount[0][paramIdx] += meanLeftRightVar;
			aggregatedSensitivitiesFiberCount[1][paramIdx] += leftVar;
			aggregatedSensitivitiesFiberCount[2][paramIdx] += rightVar;
			aggregatedSensitivitiesFiberCount[3][paramIdx] += meanTotal;
		}
	}
	if (m_aborted)
	{
		return;
	}

	m_progress.setStatus("Compute variation histogram");
	//charHistHist.resize(numCharSelected);
	charHistVar.resize(numCharSelected);
	charHistVarAgg.resize(numCharSelected);
	for (int charIdx = 0; charIdx < numCharSelected && !m_aborted; ++charIdx)
	{
		m_progress.emitProgress(100 * charIdx / numCharSelected);
		//charHistHist[charIdx].resize(NumOfVarianceAggregation);
		charHistVar[charIdx].resize(NumOfVarianceAggregation);
		charHistVarAgg[charIdx].resize(NumOfVarianceAggregation);
		for (int aggIdx = 0; aggIdx < NumOfVarianceAggregation && !m_aborted; ++aggIdx)
		{
			//charHistHist[charIdx][aggIdx].resize(m_variedParams.size());
			charHistVar[charIdx][aggIdx].resize(m_variedParams.size());
			charHistVarAgg[charIdx][aggIdx].resize(m_variedParams.size());
		}
		for (int paramIdx = 0; paramIdx < m_variedParams.size() && !m_aborted; ++paramIdx)
		{
			for (int aggIdx = 0; aggIdx < NumOfVarianceAggregation && !m_aborted; ++aggIdx)
			{
				//charHistHist[charIdx][aggIdx][paramIdx].resize(m_histogramBins);
				charHistVar[charIdx][aggIdx][paramIdx].resize(m_histogramBins);
				charHistVarAgg[charIdx][aggIdx][paramIdx].fill(0.0, m_histogramBins);
			}
			// TODO: unify with other loops over STARs?
			int origParamColIdx = m_variedParams[paramIdx];

			for (int bin = 0; bin < m_histogramBins; ++bin)
			{
				int numAllLeft = 0,
					numAllRight = 0,
					numAllTotal = 0;
				for (int aggIdx = 0; aggIdx < NumOfVarianceAggregation && !m_aborted; ++aggIdx)
				{
					//charHistHist[charIdx][aggIdx][paramIdx][bin].resize(paramSetValues.size());
					charHistVar[charIdx][aggIdx][paramIdx][bin].resize(paramSetValues.size());
				}
				for (int paramSetIdx = 0; paramSetIdx < paramSetValues.size(); ++paramSetIdx)
				{
					int resultIdxGroupStart = m_starGroupSize * paramSetIdx;
					int resultIdxParamStart = resultIdxGroupStart + 1 + paramIdx * m_numOfSTARSteps;
					/*
					for (int aggIdx = 0; aggIdx < NumOfVarianceAggregation && !m_aborted; ++aggIdx)
					{
						charHistHist[charIdx][aggIdx][paramIdx][bin][paramSetIdx].resize(paramSetValues.size());
					}
					*/
					// first - then + steps (both skipped if value +/- step exceeds bounds
					double groupStartParamVal = m_paramValues[origParamColIdx][resultIdxGroupStart];
					double paramStartParamVal = m_paramValues[origParamColIdx][resultIdxParamStart];
					double paramDiff = paramStartParamVal - groupStartParamVal;
					//LOG(lvlDebug, QString("      Parameter Set %1; start: %2 (value %3), param start: %4 (value %5); diff: %6")
					//	.arg(paramSetIdx).arg(resultIdxGroupStart).arg(groupStartParamVal)
					//	.arg(resultIdxParamStart).arg(paramStartParamVal).arg(paramDiff));

					if (paramStep[paramIdx] == 0)
					{
						paramStep[paramIdx] = std::abs(paramDiff);
					}

					int numLeftRight = 0;
					/*
					for (int agg = 0; agg < NumOfVarianceAggregation; ++agg)
					{
						charHistHist[charIdx][agg][paramIdx][bin][paramSetIdx].push_back(m_charHistograms[resultIdxGroupStart][charIdx][bin]);
					}
					*/
					charHistVar[charIdx][0][paramIdx][bin][paramSetIdx] = 0;
					if (paramDiff > 0)
					{
						// left-only:
						//charHistHist[charIdx][0][paramIdx][bin][paramSetIdx].push_back(m_charHistograms[resultIdxParamStart][charIdx][bin]);
						// left+right:
						//charHistHist[charIdx][2][paramIdx][bin][paramSetIdx].push_back(m_charHistograms[resultIdxGroupStart][charIdx][bin]);
						charHistVar[charIdx][0][paramIdx][bin][paramSetIdx] =
							std::abs(m_charHistograms[resultIdxGroupStart][charIdx][bin] - m_charHistograms[resultIdxParamStart][charIdx][bin]);
						charHistVarAgg[charIdx][0][paramIdx][bin] += charHistVar[charIdx][0][paramIdx][bin][paramSetIdx];
						charHistVarAgg[charIdx][2][paramIdx][bin] += charHistVar[charIdx][0][paramIdx][bin][paramSetIdx];
						//LOG(lvlDebug, QString("        Left var available: %1").arg(leftVar));
						++numLeftRight;
						++numAllLeft;
					}

					int k = 1;
					while (paramDiff > 0 && k < m_numOfSTARSteps)
					{
						double paramVal = m_paramValues[origParamColIdx][resultIdxParamStart + k];
						paramDiff = paramStartParamVal - paramVal;
						++k;
					}
					charHistVar[charIdx][1][paramIdx][bin][paramSetIdx] = 0;
					if (paramDiff < 0) // additional check required??
					{
						int firstPosStepIdx = resultIdxParamStart + (k - 1);
						// left-only:
						//charHistHist[charIdx][1][paramIdx][bin][paramSetIdx].push_back(m_charHistograms[firstPosStepIdx][charIdx][bin]);
						// left+right:
						//charHistHist[charIdx][2][paramIdx][bin][paramSetIdx].push_back(m_charHistograms[firstPosStepIdx][charIdx][bin]);
						charHistVar[charIdx][1][paramIdx][bin][paramSetIdx] =
							std::abs(m_charHistograms[resultIdxGroupStart][charIdx][bin] - m_charHistograms[firstPosStepIdx][charIdx][bin]);
						charHistVarAgg[charIdx][1][paramIdx][bin] += charHistVar[charIdx][1][paramIdx][bin][paramSetIdx];
						charHistVarAgg[charIdx][2][paramIdx][bin] += charHistVar[charIdx][1][paramIdx][bin][paramSetIdx];
						//LOG(lvlDebug, QString("        Right var available: %1").arg(rightVar));
						++numLeftRight;
						++numAllRight;
					}
					charHistVar[charIdx][2][paramIdx][bin][paramSetIdx] /= numLeftRight;
					bool wasSmaller = true;
					charHistVar[charIdx][3][paramIdx][bin][paramSetIdx] = 0;
					numAllTotal += m_numOfSTARSteps;
					for (int i = 0; i < m_numOfSTARSteps; ++i)
					{
						//charHistHist[charIdx][3][paramIdx][bin][paramSetIdx].push_back(m_charHistograms[resultIdxParamStart + i][charIdx][bin]);
						int compareIdx = (i == 0) ? resultIdxGroupStart : (resultIdxParamStart + i - 1);
						double paramVal = m_paramValues[origParamColIdx][resultIdxParamStart + i];
						if (paramVal > paramStartParamVal && wasSmaller)
						{
							wasSmaller = false;
							compareIdx = resultIdxGroupStart;
						}
						charHistVar[charIdx][3][paramIdx][bin][paramSetIdx] +=
							std::abs(m_charHistograms[compareIdx][charIdx][bin] - m_charHistograms[resultIdxParamStart + i][charIdx][bin]);
					}
					charHistVar[charIdx][3][paramIdx][bin][paramSetIdx] /= m_numOfSTARSteps;//charHistHist[charIdx][3][paramIdx][bin][paramSetIdx].size();
					charHistVarAgg[charIdx][3][paramIdx][bin] += charHistVar[charIdx][3][paramIdx][bin][paramSetIdx];
				}
				assert(numAllTotal == paramSetValues.size() * m_numOfSTARSteps);
				charHistVarAgg[charIdx][0][paramIdx][bin] /= numAllLeft;
				charHistVarAgg[charIdx][1][paramIdx][bin] /= numAllRight;
				charHistVarAgg[charIdx][2][paramIdx][bin] /= (numAllLeft + numAllRight);
				charHistVarAgg[charIdx][3][paramIdx][bin] /= numAllTotal;
			}
		}
	}
	if (m_aborted)
	{
		return;
	}

	m_progress.setStatus("Computing average characteristics histogram.");
	charHistAvg.resize(numCharSelected);
	for (int charIdx = 0; charIdx < numCharSelected && !m_aborted; ++charIdx)
	{
		charHistAvg[charIdx].fill(0, m_histogramBins);
		for (int resultIdx = 0; resultIdx < m_charHistograms.size(); ++resultIdx)
		{
			for (int bin = 0; bin < m_histogramBins; ++bin)
			{
				charHistAvg[charIdx][bin] += m_charHistograms[resultIdx][charIdx][bin];
			}
		}
		for (int bin = 0; bin < m_histogramBins; ++bin)
		{
			charHistAvg[charIdx][bin] /= m_charHistograms.size();
		}
	}

	m_progress.setStatus("Loading cached dissimilarities between all result pairs.");
	m_progress.emitProgress(0);
	QVector<int> measures;

	if (readDissimilarityMatrixCache(measures))
	{
		m_resultDissimMeasures.clear();
		for (auto m : measures)
		{
			m_resultDissimMeasures.push_back(std::make_pair(m, true));
		}
	}
	else
	{
		m_progress.setStatus("Creating fiber data and bounding boxes for all results.");
		int measureCount = static_cast<int>(m_resultDissimMeasures.size());
		int resultCount = static_cast<int>(m_data->result.size());
		m_resultDissimMatrix = iADissimilarityMatrixType(resultCount,
			QVector<iAResultPairInfo>(resultCount, iAResultPairInfo(measureCount)));

		for (size_t m = 0; m < m_resultDissimMeasures.size(); ++m)
		{
			measures.push_back(m_resultDissimMeasures[m].first);
		}

		//m_progress.setStatus("Creating fiber data objects.");
		std::vector<std::vector<iAFiberData>> resFib(resultCount);
		for (int resultID = 0; resultID < resultCount && !m_aborted; ++resultID)
		{
			resFib[resultID] = createResultFiberData(m_data->result[resultID]);
		}

		//m_progress.setStatus("Computing bounding boxes for all fibers.");
		std::vector<std::vector<FiberBBT>> fiberBoundingBox(resultCount);
		for (int resultID = 0; resultID < resultCount && !m_aborted; ++resultID)
		{
			fiberBoundingBox[resultID] = boundingBoxesForFibers(resFib[resultID]);
		}

		m_progress.setStatus("Computing dissimilarity between all result pairs.");

		//double overallPairs = resultCount * (resultCount - 1) / 2;
		// for all result pairs r1 and r2, for every fiber f in r1 find those fibers in r2 which best match f
		double overallPairs = static_cast<double>(resultCount) * resultCount;
		size_t curCheckedPairs = 0;
		for (int r1 = 0; r1 < resultCount && !m_aborted; ++r1)
		{
			auto& res1 = m_data->result[r1];
			auto const& mapping = *res1.mapping.data();
			// TODO: only center -> should use bounding box instead!
			double const* cxr = m_data->spmData->paramRange(mapping[iACsvConfig::CenterX]),
				* cyr = m_data->spmData->paramRange(mapping[iACsvConfig::CenterY]),
				* czr = m_data->spmData->paramRange(mapping[iACsvConfig::CenterZ]);
			double a = cxr[1] - cxr[0], b = cyr[1] - cyr[0], c = czr[1] - czr[0];
			double diagonalLength = std::sqrt(std::pow(a, 2) + std::pow(b, 2) + std::pow(c, 2));
			double const* lengthRange = m_data->spmData->paramRange(mapping[iACsvConfig::Length]);
			double maxLength = lengthRange[1] - lengthRange[0];
			for (int r2 = 0; r2 < resultCount && !m_aborted; ++r2)
			{
				auto& mat = m_resultDissimMatrix[r1][r2];
				for (int m = 0; m < measureCount; ++m)
				{
					mat.avgDissim[m] = 0;
				}
				if (r1 == r2)
				{
					continue;
				}
				m_progress.setStatus(QString("Computing dissimilarity between results %1 and %2.").arg(r1).arg(r2));
				int r1FibCount = static_cast<int>(m_data->result[r1].fiberCount);
				std::vector<int> r2MatchCount(measureCount, 0);
				auto& dissimilarities = mat.fiberDissim;
				dissimilarities.resize(r1FibCount);
				// not ideal: for loop seems to be not ideally parallelizable,
				// one spike where 100% is used, then going down to nearly 0, until next loop starts
				int noCanDo = 0;
				size_t candSum = 0;
#pragma omp parallel for reduction(+ : noCanDo, candSum)
				for (int fiberID = 0; fiberID < r1FibCount; ++fiberID)
				{
					auto candidates =
						intersectingBoundingBox(fiberBoundingBox[r1][fiberID], fiberBoundingBox[r2]);
					if (candidates.size() == 0)
					{
						++noCanDo; // thread-safe
						continue;
					}
					candSum += candidates.size();
					getBestMatches2(resFib[r1][fiberID], resFib[r2], dissimilarities[fiberID],
						candidates, diagonalLength, maxLength, m_resultDissimMeasures);
					for (int m = 0; m < measureCount; ++m)
					{
						if (dissimilarities[fiberID][m].size() > 0)
						{
							++r2MatchCount[m];
							mat.avgDissim[m] += dissimilarities[fiberID][m][0].dissimilarity;
						}
					}
				}
				LOG(lvlDebug, QString("Result %1x%2: %3 candidates on average, %4 with no bounding box intersections out of %5")
					.arg(r1).arg(r2).arg(candSum / r1FibCount).arg(noCanDo).arg(r1FibCount));
				for (int m = 0; m < measureCount; ++m)
				{
					mat.avgDissim[m] /= r2MatchCount[m];
				}
				++curCheckedPairs;
				m_progress.emitProgress(curCheckedPairs * 100.0 / overallPairs);
			}
		}
		if (m_aborted)
		{
			return;
		}
		writeDissimilarityMatrixCache(measures);
	}
	if (m_resultDissimMatrix.size() == 0)
	{
		LOG(lvlWarn, "Dissimilarity matrix not available!");
		return;
	}

	// determine dissimilarity ranges:
	m_resultDissimRanges.resize(m_resultDissimMeasures.size());
	for (int m = 0; m < m_resultDissimRanges.size(); ++m)
	{
		m_resultDissimRanges[m].first = std::numeric_limits<double>::max();
		m_resultDissimRanges[m].second = std::numeric_limits<double>::lowest();
	}
	for (int r1 = 1; r1 < m_data->result.size() && !m_aborted; ++r1)
	{
		for (int r2 = 0; r2 < m_data->result.size() && !m_aborted; ++r2)
		{
			if (r1 == r2)
			{
				continue;
			}
			for (int m = 0; m < m_resultDissimRanges.size(); ++m)
			{
				double dissim = m_resultDissimMatrix[r1][r2].avgDissim[m];
				if (dissim < m_resultDissimRanges[m].first)
				{
					m_resultDissimRanges[m].first = dissim;
				}
				if (dissim > m_resultDissimRanges[m].second)
				{
					m_resultDissimRanges[m].second = dissim;
				}
			}
		}
	}
	for (int m = 0; m < m_resultDissimRanges.size(); ++m)
	{
		LOG(lvlDebug,
			QString("m: %1; range: %2..%3")
				.arg(m)
				.arg(m_resultDissimRanges[m].first)
				.arg(m_resultDissimRanges[m].second));
	}
	if (m_aborted)
	{
		return;
	}

	// dissimilarity measure (index in m_resultDissimMeasures)
	// variation aggregation (see iASensitivityInfo::create)
	// parameter index (second index in paramSetValues / allParamValues)
	// parameter set index (first index in paramSetValues)
	//sensDissimField;
	//aggregatedSensDissim;

	int measureCount = static_cast<int>(m_resultDissimMeasures.size());
		// TODO: unify with other loops over STARs
	sensDissimField.resize(measureCount);
	aggregatedSensDissim.resize(measureCount);

	for (int m = 0; m < measureCount && !m_aborted; ++m)
	{
		QVector<double> dissimValuesUsed;
		sensDissimField[m].resize(NumOfVarianceAggregation);
		aggregatedSensDissim[m].resize(NumOfVarianceAggregation);
		for (int a = 0; a < NumOfVarianceAggregation; ++a)
		{
			sensDissimField[m][a].resize(m_variedParams.size());
			aggregatedSensDissim[m][a].fill(0.0, m_variedParams.size());
		}
		for (int paramIdx = 0; paramIdx < m_variedParams.size() && !m_aborted; ++paramIdx)
		{
			m_progress.emitProgress(100 * paramIdx / m_variedParams.size());
			int origParamColIdx = m_variedParams[paramIdx];
			for (int a = 0; a < NumOfVarianceAggregation; ++a)
			{
				sensDissimField[m][a][paramIdx].resize(paramSetValues.size());
			}
			int numAllLeft = 0, numAllRight = 0, numAllLeftRight = 0, numAllTotal = 0;
			for (int paramSetIdx = 0; paramSetIdx < paramSetValues.size(); ++paramSetIdx)
			{
				int resultIdxGroupStart = m_starGroupSize * paramSetIdx;
				int resultIdxParamStart = resultIdxGroupStart + 1 + paramIdx * m_numOfSTARSteps;

				// first - then + steps (both skipped if value +/- step exceeds bounds
				double groupStartParamVal = m_paramValues[origParamColIdx][resultIdxGroupStart];
				double paramStartParamVal = m_paramValues[origParamColIdx][resultIdxParamStart];
				double paramDiff = paramStartParamVal - groupStartParamVal;
				//LOG(lvlDebug, QString("      Parameter Set %1; start: %2 (value %3), param start: %4 (value %5); diff: %6")
				//	.arg(paramSetIdx).arg(resultIdxGroupStart).arg(groupStartParamVal)
				//	.arg(resultIdxParamStart).arg(paramStartParamVal).arg(paramDiff));

				if (paramStep[paramIdx] == 0)
				{
					paramStep[paramIdx] = std::abs(paramDiff);
				}

				double leftVar = 0;
				int numLeftRight = 0;
				if (paramDiff > 0)
				{
					leftVar = m_resultDissimMatrix[resultIdxGroupStart][resultIdxParamStart].avgDissim[m];
						//std::abs(static_cast<double>(m_data->result[resultIdxGroupStart].fiberCount) - m_data->result[resultIdxParamStart].fiberCount);
					//LOG(lvlDebug, QString("        Left var available: %1").arg(leftVar));
					++numLeftRight;
					++numAllLeft;
				}

				int k = 1;
				while (paramDiff > 0 && k < m_numOfSTARSteps)
				{
					double paramVal = m_paramValues[origParamColIdx][resultIdxParamStart + k];
					paramDiff = paramStartParamVal - paramVal;
					++k;
				}
				double rightVar = 0;
				if (paramDiff < 0)  // additional check required??
				{
					int firstPosStepIdx = resultIdxParamStart + (k - 1);
					rightVar = m_resultDissimMatrix[resultIdxGroupStart][firstPosStepIdx].avgDissim[m];
						// std::abs(static_cast<double>(m_data->result[resultIdxGroupStart].fiberCount) -m_data->result[firstPosStepIdx].fiberCount);
					//LOG(lvlDebug, QString("        Right var available: %1").arg(rightVar));
					++numLeftRight;
					++numAllRight;
				}
				double sumTotal = 0;
				bool wasSmaller = true;
				for (int i = 0; i < m_numOfSTARSteps; ++i)
				{
					int compareIdx = (i == 0) ? resultIdxGroupStart : (resultIdxParamStart + i - 1);
					double paramVal = m_paramValues[origParamColIdx][resultIdxParamStart + i];
					if (paramVal > paramStartParamVal && wasSmaller)
					{
						wasSmaller = false;
						compareIdx = resultIdxGroupStart;
					}
					double difference = m_resultDissimMatrix[compareIdx][resultIdxParamStart + i].avgDissim[m];
					dissimValuesUsed.push_back(difference);
						//std::abs(static_cast<double>(m_data->result[compareIdx].fiberCount) -	m_data->result[resultIdxParamStart + i].fiberCount);
					sumTotal += difference;
				}
				numAllLeftRight += numLeftRight;
				numAllTotal += m_numOfSTARSteps;
				double meanLeftRightVar = (leftVar + rightVar) / numLeftRight;
				double meanTotal = sumTotal / m_numOfSTARSteps;
				//LOG(lvlDebug, QString("        (left+right)/(numLeftRight=%1) = %2").arg(numLeftRight).arg(meanLeftRightVar));
				//LOG(lvlDebug, QString("        (sum total var = %1) / (m_numOfSTARSteps = %2)  = %3")
				//	.arg(sumTotal).arg(m_numOfSTARSteps).arg(meanTotal));
				sensDissimField[m][0][paramIdx][paramSetIdx] = meanLeftRightVar;
				sensDissimField[m][1][paramIdx][paramSetIdx] = leftVar;
				sensDissimField[m][2][paramIdx][paramSetIdx] = rightVar;
				sensDissimField[m][3][paramIdx][paramSetIdx] = meanTotal;

				aggregatedSensDissim[m][0][paramIdx] += meanLeftRightVar;
				aggregatedSensDissim[m][1][paramIdx] += leftVar;
				aggregatedSensDissim[m][2][paramIdx] += rightVar;
				aggregatedSensDissim[m][3][paramIdx] += meanTotal;
			}
		}
		QPair<double,double> dissimRange;
		dissimRange.first = dissimRange.second = std::numeric_limits<double>::infinity();
		auto dissimHistogram = createHistogram(dissimValuesUsed, m_histogramBins, dissimRange.first, dissimRange.second, false);
		m_dissimRanges.push_back(dissimRange);
		m_dissimHistograms.push_back(dissimHistogram);
	}
}

QString iASensitivityInfo::dissimilarityMatrixCacheFileName() const
{
	return m_data->folder + "/cache/dissimilarityMatrix.cache";
}

namespace
{
	const QString DissimilarityMatrixCacheFileIdentifier("DissimilarityMatrixCache");
	const quint32 DissimilarityMatrixCacheFileVersion(3);
	// change from v1 to v2:
	//   - changed data types in iAFiberSimilarity:
	//     - index        : quint64 -> quint32
	//     - dissimilarity: double -> float
	// change from v2 to v3:
	//   - non-symmetric matrix (to find respective, "directed" best matches in the other result)
}

bool iASensitivityInfo::readDissimilarityMatrixCache(QVector<int>& measures)
{
	QFile cacheFile(dissimilarityMatrixCacheFileName());
	// unify with verifyOpenCacheFile?
	if (!cacheFile.exists())
	{
		return false;
	}
	if (!cacheFile.open(QFile::ReadOnly))
	{
		LOG(lvlError, QString("Couldn't open file %1 for reading!").arg(cacheFile.fileName()));
		return false;
	}
	// unify with readResultRefComparison / common cache file version/identifier pattern?
	QDataStream in(&cacheFile);
	in.setVersion(CacheFileQtDataStreamVersion);
	QString identifier;
	in >> identifier;
	if (identifier != DissimilarityMatrixCacheFileIdentifier)
	{
		LOG(lvlError, QString("FIAKER cache file '%1': Unknown cache file format - found identifier %2 does not match expected identifier %3. Please delete file and let it be recreated!")
			.arg(cacheFile.fileName())
			.arg(identifier).arg(DissimilarityMatrixCacheFileIdentifier));
		return false;
	}
	quint32 version;
	in >> version;
	if (version < DissimilarityMatrixCacheFileVersion)
	{
		LOG(lvlError, QString("FIAKER cache file '%1': Too old, incompatible cache version %2; "
					"")
				.arg(cacheFile.fileName())
				.arg(version));
		return false;
	}
	if (version > DissimilarityMatrixCacheFileVersion)
	{
		LOG(lvlError, QString("FIAKER cache file '%1': Invalid or too high version number (%2), expected %3 or less. Please delete file and let it be recreated!")
			.arg(cacheFile.fileName())
			.arg(version).arg(DissimilarityMatrixCacheFileVersion));
		return false;
	}
	in >> measures;
	in >> m_resultDissimMatrix;
	cacheFile.close();
	return true;
}

void iASensitivityInfo::writeDissimilarityMatrixCache(QVector<int> const& measures) const
{
	QFileInfo fi (QFileInfo(dissimilarityMatrixCacheFileName()).absolutePath());
	if ( (fi.exists() && !fi.isDir()) || !QDir(fi.absoluteFilePath()).mkpath("."))
	{
		LOG(lvlError, QString("Could not create output directory '%1'").arg(fi.absoluteFilePath()));
		return;
	}
	QFile cacheFile(dissimilarityMatrixCacheFileName());
	if (!cacheFile.open(QFile::WriteOnly))
	{
		LOG(lvlError, QString("Couldn't open file %1 for writing!").arg(cacheFile.fileName()));
		return;
	}
	QDataStream out(&cacheFile);
	out.setVersion(CacheFileQtDataStreamVersion);
	// write header:
	out << DissimilarityMatrixCacheFileIdentifier;
	out << DissimilarityMatrixCacheFileVersion;

	out << measures;
	out << m_resultDissimMatrix;
}

typedef iAQTtoUIConnector<QWidget, Ui_SensitivitySettings> iASensitivitySettingsUI;

class iASensitivitySettingsView: public iASensitivitySettingsUI
{
	iAWidgetMap m_settingsWidgetMap;
	iAQRadioButtonVector m_rgChartType;
	const QString ProjectMeasure = "SensitivityCharacteristicsMeasure";
	const QString ProjectAggregation = "SensitivityAggregation";
	const QString ProjectDissimilarity = "SensitivityDissimilarity";
	const QString ProjectChartType = "SensitivityChartType";
	const QString ProjectColorScale = "SensitivitySPColorScale";

public:
	iASensitivitySettingsView(iASensitivityInfo* sensInf)
	{
		cmbboxMeasure->addItems(DistributionDifferenceMeasureNames());
		cmbboxAggregation->addItems(AggregationNames());
		cmbboxAggregation->setCurrentIndex(SelectedAggregationMeasureIdx);

		QStringList dissimilarities;
		for (auto dissim : sensInf->m_resultDissimMeasures)
		{
			dissimilarities << getAvailableDissimilarityMeasureNames()[dissim.first];
		}
		cmbboxDissimilarity->addItems(dissimilarities);

		cmbboxSPColorMap->addItems(iALUT::GetColorMapNames());
		cmbboxSPColorMap->setCurrentText("Brewer single hue 5c grays");

		cmbboxSPHighlightColorScale->addItems(iAColorThemeManager::instance().availableThemes());
		cmbboxSPHighlightColorScale->setCurrentText("Brewer Set3 (max. 12)");

		connect(cmbboxMeasure, QOverload<int>::of(&QComboBox::currentIndexChanged), sensInf, &iASensitivityInfo::changeMeasure);
		connect(cmbboxAggregation, QOverload<int>::of(&QComboBox::currentIndexChanged), sensInf, &iASensitivityInfo::changeAggregation);

		connect(cmbboxDissimilarity, QOverload<int>::of(&QComboBox::currentIndexChanged), sensInf, &iASensitivityInfo::updateDissimilarity);
		connect(cmbboxSPColorMap, QOverload<int>::of(&QComboBox::currentIndexChanged), sensInf, &iASensitivityInfo::updateSPDifferenceColors);
		connect(cmbboxSPHighlightColorScale, QOverload<int>::of(&QComboBox::currentIndexChanged), sensInf,
			&iASensitivityInfo::updateSPHighlightColors);

		connect(rbBar, &QRadioButton::toggled, sensInf, &iASensitivityInfo::histoChartTypeToggled);
		connect(rbLines, &QRadioButton::toggled, sensInf, &iASensitivityInfo::histoChartTypeToggled);

		cmbboxAggregation->setMinimumWidth(80);
		cmbboxMeasure->setMinimumWidth(80);
		cmbboxDissimilarity->setMinimumWidth(80);

		m_rgChartType.push_back(rbBar);
		m_rgChartType.push_back(rbLines);
		m_settingsWidgetMap.insert(ProjectMeasure, cmbboxMeasure);
		m_settingsWidgetMap.insert(ProjectAggregation, cmbboxAggregation);
		m_settingsWidgetMap.insert(ProjectDissimilarity, cmbboxDissimilarity);
		m_settingsWidgetMap.insert(ProjectChartType, &m_rgChartType);
		m_settingsWidgetMap.insert(ProjectColorScale, cmbboxSPColorMap);
	}
	void loadSettings(iASettings const & s)
	{
		::loadSettings(s, m_settingsWidgetMap);
	}
	void saveSettings(QSettings& s)
	{
		::saveSettings(s, m_settingsWidgetMap);
	}
	int dissimMeasIdx() const
	{
		return cmbboxDissimilarity->currentIndex();
	}
	QString spColorMap() const
	{
		return cmbboxSPColorMap->currentText();
	}
};


// TODO: needed? remove!
class iAParameterListView : public QWidget
{
public:
	iAParameterListView(QStringList const& paramNames,
		std::vector<std::vector<double>> const& paramValues,
		QVector<int> variedParams,
		iADissimilarityMatrixType const& dissimMatrix)
	{
		auto paramScrollArea = new QScrollArea();
		paramScrollArea->setWidgetResizable(true);
		auto paramList = new QWidget();
		paramScrollArea->setWidget(paramList);
		paramScrollArea->setContentsMargins(0, 0, 0, 0);
		auto paramListLayout = new QGridLayout();
		paramListLayout->setSpacing(LayoutSpacing);
		paramListLayout->setContentsMargins(1, 0, 1, 0);
		paramListLayout->setColumnStretch(0, 1);
		paramListLayout->setColumnStretch(1, 2);

		enum ParamColumns
		{
			NameCol = 0,
			MatrixCol
		};
		addHeaderLabel(paramListLayout, NameCol, "Parameter", QSizePolicy::Fixed);
		addHeaderLabel(paramListLayout, MatrixCol, "Sensitivity Matrix", QSizePolicy::Expanding);
		for (auto p : variedParams)
		{
			auto paramNameLabel = new QLabel(paramNames[p]);
			paramNameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
			paramListLayout->addWidget(paramNameLabel, p + 1, NameCol);

			auto paramMatrix = new iAMatrixWidget(dissimMatrix, paramValues, false, false);
			paramMatrix->setSortParameter(p);
			paramMatrix->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
			paramMatrix->setData(0);
			paramMatrix->setLookupTable(iALUT::Build(paramMatrix->range(), iALUT::GetColorMapNames()[0], 255, 255));
			m_matrixPerParam.push_back(paramMatrix);
			paramListLayout->addWidget(paramMatrix, p + 1, MatrixCol);
		}
		setLayout(paramListLayout);
	}

	void dissimMatrixMeasureChanged(int idx)
	{
		for (auto paramMatrix : m_matrixPerParam)
		{
			paramMatrix->setData(idx);
			paramMatrix->update();
		}
	}

	void dissimMatrixColorMapChanged(int idx)
	{
		for (auto paramMatrix : m_matrixPerParam)
		{
			paramMatrix->setLookupTable(iALUT::Build(paramMatrix->range(), iALUT::GetColorMapNames()[idx], 255, 255));
			paramMatrix->update();
		}
	}
private:
	std::vector<iAMatrixWidget*> m_matrixPerParam;
};


class iAColorMapWidget: public QWidget
{
public:
	iAColorMapWidget()
		: m_lut(new iALookupTable())
	{	// create default lookup table:
		m_lut->allocate(2);
		m_lut->setColor(0, ScatterPlotPointColor);
		m_lut->setColor(1, ScatterPlotPointColor);
		m_lut->setRange(0, 1);
	}
	void setColorMap(QSharedPointer<iALookupTable> lut)
	{
		m_lut = lut;
	}
private:
	const int ScalarBarPadding = 5;
	QSharedPointer<iALookupTable> m_lut;
	void paintEvent(QPaintEvent* ev) override
	{
		Q_UNUSED(ev);
		QPainter p(this);
		QString minStr = dblToStringWithUnits(m_lut->getRange()[0]);
		QString maxStr = dblToStringWithUnits(m_lut->getRange()[1]);
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
		int textWidth = std::max(p.fontMetrics().horizontalAdvance(minStr), p.fontMetrics().horizontalAdvance(maxStr));
#else
		int textWidth = std::max(fm.width(minStr), fm.width(maxStr));
#endif
		int scalarBarWidth = geometry().width() - 2 * ScalarBarPadding - textWidth;
		// Draw scalar bar (duplicated from iAQSplom!)
		QPoint topLeft(ScalarBarPadding+textWidth, ScalarBarPadding);

		QRect colorBarRect(topLeft.x(), topLeft.y(), scalarBarWidth, height() - 2 * ScalarBarPadding);
		QLinearGradient grad(topLeft.x(), topLeft.y(), topLeft.x(), topLeft.y() + colorBarRect.height());
		QMap<double, QColor>::iterator it;
		for (size_t i = 0; i < m_lut->numberOfValues(); ++i)
		{
			double rgba[4];
			m_lut->getTableValue(i, rgba);
			QColor color(rgba[0] * 255, rgba[1] * 255, rgba[2] * 255, rgba[3] * 255);
			double key = 1 - (static_cast<double>(i) / (m_lut->numberOfValues() - 1));
			grad.setColorAt(key, color);
		}
		p.fillRect(colorBarRect, grad);
		p.drawRect(colorBarRect);
		// Draw color bar / name of parameter used for coloring
		int colorBarTextX = topLeft.x() - (textWidth + ScalarBarPadding);
		p.drawText(colorBarTextX, topLeft.y() + p.fontMetrics().height(), maxStr);
		p.drawText(colorBarTextX, height() - ScalarBarPadding, minStr);
	}
};

struct iAPolyDataRenderer
{
	vtkSmartPointer<vtkRenderer> renderer;
	std::vector<vtkSmartPointer<vtkPolyData>> data;
	std::vector<vtkSmartPointer<vtkActor>> actor;
	vtkSmartPointer<vtkCornerAnnotation> text;

	vtkSmartPointer<vtkPolyData> diffPoints;
	vtkSmartPointer<vtkPolyDataMapper> diffPtMapper;
	vtkSmartPointer<vtkActor> diffActor;
};

class iASensitivityGUI
{
public:
	iASensitivityGUI():
		m_paramInfluenceView(nullptr),
		m_settings(nullptr),
		m_scatterPlot(nullptr),
		m_colorMapWidget(nullptr),
		m_dwParamInfluence(nullptr),
		m_matrixWidget(nullptr),
		m_parameterListView(nullptr),
		m_algoInfo(nullptr),
		m_diff3DWidget(nullptr),
		m_diff3DRenderManager(/*sharedCamera = */true),
		m_diff3DEmptyRenderer(vtkSmartPointer<vtkRenderer>::New()),
		m_diff3DEmptyText(vtkSmartPointer<vtkCornerAnnotation>::New())
	{
		m_diff3DEmptyText->SetLinearFontScaleFactor(2);
		m_diff3DEmptyText->SetNonlinearFontScaleFactor(1);
		m_diff3DEmptyText->SetMaximumFontSize(18);
		m_diff3DEmptyText->SetText(2, "No Fiber/Result selected");
		auto textColor = qApp->palette().color(QPalette::Text);
		m_diff3DEmptyText->GetTextProperty()->SetColor(textColor.redF(), textColor.greenF(), textColor.blueF());
		auto bgColor = qApp->palette().color(QPalette::Window);
		m_diff3DEmptyRenderer->SetBackground(bgColor.redF(), bgColor.greenF(), bgColor.blueF());
		m_diff3DEmptyRenderer->AddViewProp(m_diff3DEmptyText);
	}

	//! @{ Param Influence List
	iAParameterInfluenceView* m_paramInfluenceView;
	//! @}

	//! Overall settings
	iASensitivitySettingsView* m_settings;

	//! scatter plot for the MDS plot of all results
	iAScatterPlotWidget* m_scatterPlot;
	//! lookup table for points in scatter plot
	QSharedPointer<iALookupTable> m_lut;
	iAColorMapWidget* m_colorMapWidget;

	iADockWidgetWrapper* m_dwParamInfluence;

	QSharedPointer<iASPLOMData> m_mdsData;

	iADissimilarityMatrixType m_dissimilarityMatrix;
	iAMatrixWidget* m_matrixWidget;
	iAParameterListView* m_parameterListView;
	iAAlgorithmInfo* m_algoInfo;

	iAVtkWidget* m_diff3DWidget;
	iARendererViewSync m_diff3DRenderManager;
	std::vector<QSharedPointer<iAPolyDataRenderer>> m_diff3DRenderers;

	vtkSmartPointer<vtkRenderer> m_diff3DEmptyRenderer;
	vtkSmartPointer<vtkCornerAnnotation> m_diff3DEmptyText;

	void updateScatterPlotLUT(int starGroupSize, int numOfSTARSteps, size_t resultCount, int numInputParams,
		iADissimilarityMatrixType const & resultDissimMatrix, QVector<QPair<double, double> > const & dissimRanges,
		int measureIdx, QString const & colorScaleName)
	{
		//LOG(lvlDebug, "\nNEW LUT:");
		std::set<int> hiGrp;
		std::set<std::pair<int, int> > hiGrpParam;
		std::set<int> hiGrpAll;
		auto const& hp = m_scatterPlot->viewData()->highlightedPoints();
		for (auto ptIdx : hp)
		{
			int groupID = ptIdx / starGroupSize;
			if (ptIdx % starGroupSize == 0)
			{
				//LOG(lvlDebug, QString("Selected GROUP: %1").arg(groupID));
				hiGrp.insert(groupID);
			}
			else
			{
				int paramID = ((ptIdx % starGroupSize) - 1) / numOfSTARSteps;
				//LOG(lvlDebug, QString("Selected PARAM: %1, %2").arg(groupID).arg(paramID));
				hiGrpParam.insert(std::make_pair(groupID, paramID));
			}
			hiGrpAll.insert(groupID);
		}
		/*
		for (size_t i = 0; i < resultCount; ++i)
		{
			int groupID = static_cast<int>(i / starGroupSize);
			bool highlightGroup = hiGrp.find(groupID) != hiGrp.end();
			QColor c;
			if (i % starGroupSize == 0)
			{
				highlightGroup = highlightGroup ||
					// highlight center also if any of its param subgroups is highlighted:
					std::find_if(hiGrpParam.begin(), hiGrpParam.end(), [groupID](std::pair<int, int> a)
					{
						return a.first == groupID;
					}) != hiGrpParam.end();
				int colVal = highlightGroup ? 0 : 64;
				c = QColor(colVal, colVal, colVal, highlightGroup ? 255 : 128);
				//LOG(lvlDebug, QString("Point %1 (group=%2) : Color=%3, %4, %5, %6")
				//	.arg(i).arg(groupID).arg(c.red()).arg(c.green()).arg(c.blue()).arg(c.alpha()));
			}
			else
			{
				int paramID = ((i % starGroupSize) - 1) / numOfSTARSteps;
				bool highlightParam = hiGrpParam.find(std::make_pair(groupID, paramID)) != hiGrpParam.end();
				int colVal = (highlightParam || highlightGroup) ? 128 : 192;
				int blueVal = 192;
				c = QColor(colVal, colVal, blueVal, (highlightParam || highlightGroup) ? 192 : 128);
				//LOG(lvlDebug, QString("Point %1 (group=%2, paramID=%3) : Color=%4, %5, %6, %7")
				//	.arg(i).arg(groupID).arg(paramID).arg(c.red()).arg(c.green()).arg(c.blue()).arg(c.alpha()));
			}
			m_lut->setColor(i, c);
		}
		m_scatterPlot->setLookupTable(m_lut, m_mdsData->numParams() - 1);
		*/

		for (size_t curResultIdx = 0; curResultIdx < resultCount; ++curResultIdx)
		{
			size_t refResultIdx;
			if (m_paramInfluenceView->selectedResults().size() == 1)
			{  // color by difference to currently selected result
				refResultIdx = *m_paramInfluenceView->selectedResults().begin();
			}
			else
			{  // color by difference to STAR center
				refResultIdx = curResultIdx - (curResultIdx % starGroupSize);
			}
			m_mdsData->data()[m_mdsData->numParams() - SPDissimilarityOffset][curResultIdx] =
				resultDissimMatrix[static_cast<int>(curResultIdx)][static_cast<int>(refResultIdx)]
					.avgDissim[measureIdx];
		}
		m_mdsData->updateRanges();
		//auto rng = m_mdsData->paramRange(m_mdsData->numParams() - SPDissimilarityOffset);
		double rng[2];
		rng[0] = dissimRanges.size() > 0 ? dissimRanges[measureIdx].first : 0;
		rng[1] = dissimRanges.size() > 0 ? dissimRanges[measureIdx].second : 1;
		*m_lut.data() = iALUT::Build(rng, colorScaleName, 255, 0);
		m_scatterPlot->setLookupTable(m_lut, m_mdsData->numParams() - SPDissimilarityOffset);

		m_colorMapWidget->setColorMap(m_scatterPlot->lookupTable());
		m_colorMapWidget->update();

		m_scatterPlot->viewData()->clearLines();
		// we want to build a separate line for each parameter (i.e. in each branch "direction" of the STAR
		// easiest way is to collect all parameter values in a group (done in the vector of size_t/double pairs),
		// then sort this by the parameter values (since we don't know else how many are smaller or larger than
		// the center value), then take the point indices from this ordered vector to form the line.
		for (auto groupID : hiGrpAll)
		{
			auto groupStart = groupID * starGroupSize;
			for (int parIdx = 0; parIdx < numInputParams; ++parIdx)
			{
				using PtData = std::pair<size_t, double>;
				std::vector<PtData> linePtParVal;
				double centerValue = m_mdsData->paramData(parIdx)[groupStart];
				linePtParVal.push_back(std::make_pair(groupStart, centerValue));
				auto paraStart = groupStart + 1 + parIdx * numOfSTARSteps;
				for (int inParaIdx = 0; inParaIdx < numOfSTARSteps; ++inParaIdx)
				{
					size_t ptIdx = paraStart + inParaIdx;
					double paramValue = m_mdsData->paramData(parIdx)[ptIdx];
					linePtParVal.push_back(std::make_pair(ptIdx, paramValue));
				}
				std::sort(linePtParVal.begin(), linePtParVal.end(), [](PtData const& a, PtData const& b)
					{
						return a.second < b.second;
					});
				std::vector<size_t> linePoints(linePtParVal.size());
				for (size_t i = 0; i < linePoints.size(); ++i)
				{
					linePoints[i] = linePtParVal[i].first;
				}
				m_scatterPlot->viewData()->addLine(linePoints, QColor());
			}
		}
	}
};

using iADissimilarityMatrixDockContent = iAQTtoUIConnector<QWidget, Ui_DissimilarityMatrix>;

QWidget* iASensitivityInfo::setupMatrixView(QVector<int> const& measures)
{
	iADissimilarityMatrixDockContent* dissimDockContent = new iADissimilarityMatrixDockContent();
	auto measureNames = getAvailableDissimilarityMeasureNames();
	QStringList computedMeasureNames;
	for (int m = 0; m < measures.size(); ++m)
	{
		computedMeasureNames << measureNames[measures[m]];
	}
	dissimDockContent->cbMeasure->addItems(computedMeasureNames);
	dissimDockContent->cbParameter->addItems(m_paramNames);
	dissimDockContent->cbColorMap->addItems(iALUT::GetColorMapNames());
	connect(dissimDockContent->cbMeasure, QOverload<int>::of(&QComboBox::currentIndexChanged),
		this, &iASensitivityInfo::dissimMatrixMeasureChanged);
	connect(dissimDockContent->cbParameter, QOverload<int>::of(&QComboBox::currentIndexChanged),
		this, &iASensitivityInfo::dissimMatrixParameterChanged);
	connect(dissimDockContent->cbColorMap, QOverload<int>::of(&QComboBox::currentIndexChanged),
		this, &iASensitivityInfo::dissimMatrixColorMapChanged);                          // showAxes currently buggy - crashes!
	m_gui->m_matrixWidget = new iAMatrixWidget(m_resultDissimMatrix, m_paramValues, true, false);
	m_gui->m_matrixWidget->setSortParameter(0);
	m_gui->m_matrixWidget->setData(0);
	m_gui->m_matrixWidget->setLookupTable(iALUT::Build(m_gui->m_matrixWidget->range(), iALUT::GetColorMapNames()[0], 255, 255));
	dissimDockContent->matrix->layout()->addWidget(m_gui->m_matrixWidget);
	return dissimDockContent;
}

void iASensitivityInfo::dissimMatrixMeasureChanged(int idx)
{
	m_gui->m_matrixWidget->setData(idx);
	m_gui->m_matrixWidget->update();
	m_gui->m_parameterListView->dissimMatrixMeasureChanged(idx);
}

void iASensitivityInfo::dissimMatrixParameterChanged(int idx)
{
	m_gui->m_matrixWidget->setSortParameter(idx);
	m_gui->m_matrixWidget->update();
}

void iASensitivityInfo::dissimMatrixColorMapChanged(int idx)
{
	m_gui->m_matrixWidget->setLookupTable(iALUT::Build(m_gui->m_matrixWidget->range(), iALUT::GetColorMapNames()[idx], 255, 255));
	m_gui->m_matrixWidget->update();
	m_gui->m_parameterListView->dissimMatrixColorMapChanged(idx);
}

QString iASensitivityInfo::charactName(int selCharIdx) const
{
	return m_data->spmData->parameterName(m_charSelected[selCharIdx])
		.replace("[µm]", "")
		.replace("[µm²]", "")
		.replace("[µm³]", "")
		.replace("[°]", "");
}

namespace
{
	const QString ProjectParameterFile("ParameterSetsFile");
	const QString ProjectCharacteristics("Characteristics");
	const QString ProjectCharDiffMeasures("CharacteristicDifferenceMeasures");
	const QString ProjectSkipParameterCSVColumns("SkipParameterCSVColumns");
	const QString ProjectHistogramBins("DistributionHistogramBins"); // needs to match ProjectDistributionHistogramBins in iAFiAKErController.cpp
	//const QString ProjectResultDissimilarityMeasure("ResultDissimilarityMeasures");
}

void iASensitivityInfo::saveProject(QSettings& projectFile, QString  const& fileName)
{
	projectFile.setValue(ProjectParameterFile, MakeRelative(QFileInfo(fileName).absolutePath(), m_parameterFileName));
	projectFile.setValue(ProjectSkipParameterCSVColumns, m_skipColumns);
	projectFile.setValue(ProjectCharacteristics, joinNumbersAsString(m_charSelected, ","));
	projectFile.setValue(ProjectCharDiffMeasures, joinNumbersAsString(m_charDiffMeasure, ","));
	m_gui->m_settings->saveSettings(projectFile);

	// stored in cache file:
	//projectFile.setValue(ProjectResultDissimilarityMeasure, joinAsString(m_resultDissimMeasures, ",",
	//	[](std::pair<int, bool> const& a) {return QString::number(a.first)+":"+(a.second?"true":"false"); }));
}

bool iASensitivityInfo::hasData(iASettings const& settings)
{
	return settings.contains(ProjectParameterFile);
}


QSharedPointer<iASensitivityInfo> iASensitivityInfo::load(QMainWindow* child,
	QSharedPointer<iAFiberResultsCollection> data, QDockWidget* nextToDW, iASettings const& projectFile,
	QString const& projectFileName, std::vector<iAFiberCharUIData> const& resultUIs, iAVtkWidget* main3DWidget)
{
	QString parameterSetFileName = MakeAbsolute(QFileInfo(projectFileName).absolutePath(), projectFile.value(ProjectParameterFile).toString());
	QVector<int> charsSelected = stringToVector<QVector<int>, int>(projectFile.value(ProjectCharacteristics).toString());
	QVector<int> charDiffMeasure = stringToVector<QVector<int>, int>(projectFile.value(ProjectCharDiffMeasures).toString());
	int skipColumns = projectFile.value(ProjectSkipParameterCSVColumns, 1).toInt();
	int histogramBins = projectFile.value(ProjectHistogramBins, 20).toInt();
	return iASensitivityInfo::create(child, data, nextToDW, histogramBins, skipColumns, resultUIs, main3DWidget,
		parameterSetFileName, charsSelected, charDiffMeasure, projectFile);
}

class iASPParamPointInfo final: public iAScatterPlotPointInfo
{
public:
	iASPParamPointInfo(iASensitivityInfo const & data, iAFiberResultsCollection const & results) :
		m_data(data),
		m_results(results)
	{}
	QString text(const size_t paramIdx[2], size_t pointIdx) override
	{
		Q_UNUSED(paramIdx);
		QString result(QString("Fiber Count: %1<br/>").arg(m_results.result[pointIdx].fiberCount));
		for (int i = 0; i < m_data.m_variedParams.size(); ++i)
		{
			result +=
				m_data.m_paramNames[m_data.m_variedParams[i]] + ": " +
				QString::number(m_data.m_paramValues[m_data.m_variedParams[i]][pointIdx], 'f', 3) + "<br/>";
				/*
				m_data->parameterName(paramIdx[0]) + ": " +
				QString::number(m_data->paramData(paramIdx[0])[pointIdx]) + "<br>" +
				m_data->parameterName(paramIdx[1]) + ": " +
				QString::number(m_data->paramData(paramIdx[1])[pointIdx])
				*/
		}
		return result;
	}
private:
	iASensitivityInfo const & m_data;
	iAFiberResultsCollection const& m_results;
};

void iASensitivityInfo::createGUI()
{
	if (m_aborted)
	{
		emit aborted();
		return;
	}
	m_gui.reset(new iASensitivityGUI);

	m_gui->m_settings = new iASensitivitySettingsView(this);
	auto dwSettings = new iADockWidgetWrapper(m_gui->m_settings, "Sensitivity Settings", "foeSensitivitySettings");
	m_child->splitDockWidget(m_nextToDW, dwSettings, Qt::Horizontal);

	m_gui->m_paramInfluenceView = new iAParameterInfluenceView(this, ParamColor, OutputColor);
	m_gui->m_dwParamInfluence = new iADockWidgetWrapper(m_gui->m_paramInfluenceView, "Parameter Influence", "foeParamInfluence");
	connect(m_gui->m_paramInfluenceView, &iAParameterInfluenceView::barAdded, this, &iASensitivityInfo::outputBarAdded);
	connect(m_gui->m_paramInfluenceView, &iAParameterInfluenceView::barRemoved, this, &iASensitivityInfo::outputBarRemoved);
	connect(m_gui->m_paramInfluenceView, &iAParameterInfluenceView::resultSelected, this, &iASensitivityInfo::parResultSelected);
	m_child->splitDockWidget(dwSettings, m_gui->m_dwParamInfluence, Qt::Vertical);

	QStringList algoInNames;
	for (auto p : m_variedParams)
	{
		algoInNames.push_back(m_paramNames[p]);
	}
	QStringList algoOutNames;
	for (int charIdx = 0; charIdx < m_charSelected.size(); ++charIdx)
	{
		algoOutNames << charactName(charIdx);
	}
	m_gui->m_algoInfo = new iAAlgorithmInfo("Fiber Reconstruction", algoInNames, algoOutNames, ParamColor, OutputColor);
	m_gui->m_algoInfo->addShownOut(0); // equivalent to addStackedBar in iAParameterInfluenceView constructor!
	connect(m_gui->m_algoInfo, &iAAlgorithmInfo::inputClicked,
		m_gui->m_paramInfluenceView, &iAParameterInfluenceView::setSelectedParam);
	connect(m_gui->m_algoInfo, &iAAlgorithmInfo::outputClicked,
		m_gui->m_paramInfluenceView, &iAParameterInfluenceView::toggleCharacteristic);
	auto dwAlgoInfo = new iADockWidgetWrapper(m_gui->m_algoInfo, "Algorithm Details", "foeAlgorithmInfo");
	m_child->splitDockWidget(dwSettings, dwAlgoInfo, Qt::Horizontal);

	QVector<int> measures;
	for (auto d : m_resultDissimMeasures)
	{
		measures.push_back(d.first);
	}
	QWidget* dissimDockContent = setupMatrixView(measures);
	auto dwDissimMatrix = new iADockWidgetWrapper(dissimDockContent, "Dissimilarity Matrix", "foeMatrix");
	m_child->splitDockWidget(m_gui->m_dwParamInfluence, dwDissimMatrix, Qt::Vertical);

	m_gui->m_parameterListView = new iAParameterListView(m_paramNames, m_paramValues, m_variedParams, m_resultDissimMatrix);
	auto dwParamView = new iADockWidgetWrapper(m_gui->m_parameterListView, "Parameter View", "foeParameters");
	m_child->splitDockWidget(m_gui->m_dwParamInfluence, dwParamView, Qt::Vertical);

	m_gui->m_mdsData = QSharedPointer<iASPLOMData>(new iASPLOMData());
	std::vector<QString> spParamNames;
	for (auto p: m_variedParams)
	{
		spParamNames.push_back(m_paramNames[p]);
	}
	spParamNames.push_back("MDS X");
	spParamNames.push_back("MDS Y");
	spParamNames.push_back("ID");
	spParamNames.push_back("Dissimilarity"); // dissimilarity according to currently selected result
	size_t resultCount = m_data->result.size();
	m_gui->m_mdsData->setParameterNames(spParamNames, resultCount);
	for (int c = 0; c < spParamNames.size(); ++c)
	{
		m_gui->m_mdsData->data()[c].resize(resultCount);
	}
	for (int p=0; p<m_variedParams.size(); ++p)
	{	// set parameter values:
		int origParamIdx = m_variedParams[p];
		for (int r = 0; r < resultCount; ++r)
		{
			m_gui->m_mdsData->data()[p][r] = m_paramValues[origParamIdx][r];
		}
	}
	for (size_t i = 0; i < resultCount; ++i)
	{
		m_gui->m_mdsData->data()[spParamNames.size() - SPMDSXOffset][i] = 0.0;  // MDS X
		m_gui->m_mdsData->data()[spParamNames.size() - SPMDSYOffset][i] = 0.0;  // MDS Y
		m_gui->m_mdsData->data()[spParamNames.size() - SPIDOffset][i] = i;    // ID
		m_gui->m_mdsData->data()[spParamNames.size() - SPDissimilarityOffset][i] = 0.0;  // Dissimilarity
	}
	m_gui->m_mdsData->updateRanges();
	m_gui->m_scatterPlot = new iAScatterPlotWidget(m_gui->m_mdsData, true);
	m_gui->m_scatterPlot->setPointRadius(4);
	m_gui->m_scatterPlot->setPickedPointFactor(1.5);
	m_gui->m_scatterPlot->setFixPointsEnabled(true);
	//m_gui->m_scatterPlot->setHighlightColor(SelectedResultPlotColor);
	m_gui->m_scatterPlot->setHighlightColorTheme(
		iAColorThemeManager::instance().theme(m_gui->m_settings->cmbboxSPHighlightColorScale->currentText()));
	m_gui->m_scatterPlot->setHighlightDrawMode(iAScatterPlot::Outline | iAScatterPlot::Enlarged);
	m_gui->m_scatterPlot->setSelectionEnabled(false);
	m_gui->m_lut.reset(new iALookupTable());
	m_gui->m_lut->setRange(0, m_data->result.size());
	m_gui->m_lut->allocate(m_data->result.size());
	auto dwScatterPlot = new iADockWidgetWrapper(m_gui->m_scatterPlot, "Results Overview", "foeScatterPlot");
	connect(m_gui->m_scatterPlot, &iAScatterPlotWidget::pointHighlighted, this, &iASensitivityInfo::spPointHighlighted);
	connect(m_gui->m_scatterPlot, &iAScatterPlotWidget::highlightChanged, this, &iASensitivityInfo::spHighlightChanged);
	m_child->splitDockWidget(m_gui->m_dwParamInfluence, dwScatterPlot, Qt::Vertical);
	m_gui->m_colorMapWidget = new iAColorMapWidget();
	auto dwColorMap = new iADockWidgetWrapper(m_gui->m_colorMapWidget, "Dissimilarity Color Map", "foeColorMap");
	m_child->splitDockWidget(dwScatterPlot, dwColorMap, Qt::Horizontal);

	m_gui->updateScatterPlotLUT(m_starGroupSize, m_numOfSTARSteps, m_data->result.size(), m_variedParams.size(),
		m_resultDissimMatrix, m_resultDissimRanges, m_gui->m_settings->dissimMeasIdx(),
		m_gui->m_settings->spColorMap());
	m_gui->m_scatterPlot->setPointInfo(
		QSharedPointer<iAScatterPlotPointInfo>(new iASPParamPointInfo(*this, *m_data.data())));

	m_gui->m_diff3DWidget = new iAVtkWidget();
	auto renWin = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
#if VTK_VERSION_NUMBER < VTK_VERSION_CHECK(9, 0, 0)
	m_gui->m_diff3DWidget->SetRenderWindow(renWin);
#else
	m_gui->m_diff3DWidget->setRenderWindow(renWin);
#endif
	auto dwDiff3D = new iADockWidgetWrapper(m_gui->m_diff3DWidget, "Difference 3D", "foeDiff3D");
	m_child->splitDockWidget(dwSettings, dwDiff3D, Qt::Horizontal);
#if VTK_VERSION_NUMBER < VTK_VERSION_CHECK(9, 0, 0)
	m_gui->m_diff3DRenderManager.addToBundle(m_main3DWidget->GetRenderWindow()->GetRenderers()->GetFirstRenderer());
#else
	m_gui->m_diff3DRenderManager.addToBundle(m_main3DWidget->renderWindow()->GetRenderers()->GetFirstRenderer());
#endif
	renWin->AddRenderer(m_gui->m_diff3DEmptyRenderer);

	updateDissimilarity();
	changeAggregation(SelectedAggregationMeasureIdx);

	if (!m_projectToLoad.isEmpty())
	{
		m_gui->m_settings->loadSettings(m_projectToLoad);
	}
}

void iASensitivityInfo::changeMeasure(int newMeasure)
{
	m_gui->m_paramInfluenceView->setMeasure(newMeasure);
}

void iASensitivityInfo::changeAggregation(int newAggregation)
{
	m_gui->m_paramInfluenceView->setAggregation(newAggregation);
}

void iASensitivityInfo::outputBarAdded(int outType, int outIdx)
{
	if (outType == outCharacteristic)
	{
		m_gui->m_algoInfo->addShownOut(outIdx);
		m_gui->m_algoInfo->update();
	}
}
void iASensitivityInfo::outputBarRemoved(int outType, int outIdx)
{
	if (outType == outCharacteristic)
	{
		m_gui->m_algoInfo->removeShownOut(outIdx);
		m_gui->m_algoInfo->update();
	}
}

void iASensitivityInfo::fiberSelectionChanged(std::vector<std::vector<size_t>> const & selection)
{
	m_baseFiberSelection = selection;
	m_currentFiberSelection = selection;

	size_t selectedFibers = 0, resultCount = 0;
	for (size_t resultID = 0; resultID < selection.size(); ++resultID)
	{
		selectedFibers += selection[resultID].size();
		resultCount += (selection[resultID].size() > 0) ? 1 : 0;
	}
	LOG(lvlDebug, QString("New fiber selection: %1 selected fibers in %2 results").arg(selectedFibers).arg(resultCount));

	updateDifferenceView();
}

void iASensitivityInfo::updateDissimilarity()
{
	int dissimIdx = m_gui->m_settings->cmbboxDissimilarity->currentIndex();
	iAMatrixType distMatrix(m_data->result.size(), std::vector<double>(m_data->result.size()));
	//LOG(lvlDebug, "Distance Matrix:");
	for (int r1 = 0; r1 < distMatrix.size(); ++r1)
	{
		QString line;
		for (int r2 = 0; r2 < distMatrix.size(); ++r2)
		{
			distMatrix[r1][r2] = m_resultDissimMatrix[r1][r2].avgDissim[dissimIdx];
			line += " " + QString::number(distMatrix[r1][r2], 'f', 2).rightJustified(5);
		}
		//LOG(lvlDebug, QString("%1:%2").arg(r1).arg(line));
	}
	auto mds = computeMDS(distMatrix, 2, 100);
	//LOG(lvlDebug, "MDS:");
	for (int i = 0; i < mds.size(); ++i)
	{
		for (int c = 0; c < mds[0].size(); ++c)
		{
			m_gui->m_mdsData->data()[m_gui->m_mdsData->numParams() - SPMDSXOffset + c][i] = mds[i][c];
		}
		//LOG(lvlDebug, QString("%1: %2, %3").arg(i).arg(mds[i][0]).arg(mds[i][1]));
	}
	m_gui->m_mdsData->updateRanges();
}

void iASensitivityInfo::spPointHighlighted(size_t resultIdx, bool state)
{
	int paramID = -1;
	if (state && resultIdx % m_starGroupSize != 0)
	{	// specific parameter "branch" selected:
		paramID = ((resultIdx % m_starGroupSize) - 1) / m_numOfSTARSteps;
	}
	m_gui->m_paramInfluenceView->setResultSelected(resultIdx, state);
	m_gui->m_paramInfluenceView->setSelectedParam(paramID);
	emit resultSelected(resultIdx, state);
	if (m_currentFiberSelection.size() == 0)
	{	// before first selection is made...
		return;
	}

	// in any case, remove all eventually selected fibers in result from current selection:
	m_currentFiberSelection[resultIdx].clear();
	if (state)
	{	// add fibers matching the selection in m_baseFiberSelection to current selection:
		auto it = std::find(m_resultDissimMeasures.begin(), m_resultDissimMeasures.end(), std::make_pair(7, true));
		int measIdx = (it != m_resultDissimMeasures.end()) ? it - m_resultDissimMeasures.begin() : 0;
		for (int rSel = 0; rSel < m_baseFiberSelection.size(); ++rSel)
		{
			if (m_baseFiberSelection[rSel].size() == 0)
			{
				continue;
			}
			if (rSel == resultIdx)
			{
				m_currentFiberSelection[rSel] = m_baseFiberSelection[rSel];
				continue;
			}
			for (auto rSelFibID : m_baseFiberSelection[rSel])
			{
				auto& similarFibers = m_resultDissimMatrix[rSel][resultIdx].fiberDissim[rSelFibID][measIdx];
				for (auto similarFiber : similarFibers)
				{
					LOG(lvlDebug,
						QString("        Fiber %1, dissimilarity: %2%3")
							.arg(similarFiber.index)
							.arg(similarFiber.dissimilarity)
							.arg(similarFiber.dissimilarity >= 1 ? " (skipped)" : ""));
					if (similarFiber.dissimilarity < 1 &&
						std::find(m_currentFiberSelection[resultIdx].begin(), m_currentFiberSelection[resultIdx].end(),
							similarFiber.index) == m_currentFiberSelection[resultIdx].end())
					{
						m_currentFiberSelection[resultIdx].push_back(similarFiber.index);
					}
				}
			}
		}
		std::sort(m_currentFiberSelection[resultIdx].begin(), m_currentFiberSelection[resultIdx].end());
	}
	// TODO: change detection - only trigger fibersToSelect if selection has changed?
	emit fibersToSelect(m_currentFiberSelection);
}

void iASensitivityInfo::histoChartTypeToggled(bool checked)
{
	if (checked)
	{
		m_gui->m_paramInfluenceView->setHistogramChartType(qobject_cast<QRadioButton*>(QObject::sender())->text());
	}
}

void iASensitivityInfo::parResultSelected(size_t resultIdx, Qt::KeyboardModifiers modifiers)
{
	m_gui->m_scatterPlot->toggleHighlightedPoint(resultIdx, modifiers);
}

void iASensitivityInfo::updateSPDifferenceColors()
{
	m_gui->updateScatterPlotLUT(m_starGroupSize, m_numOfSTARSteps, m_data->result.size(), m_variedParams.size(),
		m_resultDissimMatrix, m_resultDissimRanges, m_gui->m_settings->dissimMeasIdx(),
		m_gui->m_settings->spColorMap());
}

void iASensitivityInfo::updateSPHighlightColors()
{
	m_gui->m_scatterPlot->setHighlightColorTheme(
		iAColorThemeManager::instance().theme(m_gui->m_settings->cmbboxSPHighlightColorScale->currentText()));
}

void iASensitivityInfo::spHighlightChanged()
{
	updateSPDifferenceColors();
	updateDifferenceView();
}

std::vector<size_t> iASensitivityInfo::selectedResults() const
{
	if (!m_gui || !m_gui->m_scatterPlot)
	{
		return std::vector<size_t>();
	}
	return m_gui->m_scatterPlot->viewData()->highlightedPoints();
}

namespace
{
	void logMeshSize(QString const& name, vtkSmartPointer<vtkPolyData> mesh)
	{
		const double* b1 = mesh->GetBounds();
		LOG(lvlDebug,
			QString(name + ": %1, %2, %3, %4 (mesh: %5 cells, %6 points)")
				.arg(b1[0])
				.arg(b1[1])
				.arg(b1[2])
				.arg(b1[3])
				.arg(mesh->GetNumberOfCells())
				.arg(mesh->GetNumberOfPoints()));
	}
}

void iASensitivityInfo::updateDifferenceView()
{
	//iATimeGuard timer("ShowDifference");
#if VTK_VERSION_NUMBER < VTK_VERSION_CHECK(9, 0, 0)
	auto renWin = m_gui->m_diff3DWidget->GetRenderWindow();
#else
	auto renWin = m_gui->m_diff3DWidget->renderWindow();
#endif
	auto const& hp = m_gui->m_scatterPlot->viewData()->highlightedPoints();

	// TODO: reuse actors... / store what was previously shown and only update if something has changed?
	//LOG(lvlDebug, QString("%1 Results!").arg(hp.size()));
	for (auto r: m_gui->m_diff3DRenderers)
	{
		m_gui->m_diff3DRenderManager.removeFromBundle(r->renderer);
		renWin->RemoveRenderer(r->renderer);
	}
	//renWin->GetRenderers()->RemoveAllItems();	// should also work instead of above RemoveRenderer call
	m_gui->m_diff3DRenderers.clear();
	// TODO: determine "central" resultID to compare to / fixed comparison point determined by user?
	for (size_t i=0; i<hp.size(); ++i)
	{
		auto rID = hp[i];
		if (!m_resultUIs[rID].main3DVis)
		{
			LOG(lvlDebug, QString("Result %1: 3D vis not initialized!").arg(rID));
			continue;
		}
		auto resultData = QSharedPointer<iAPolyDataRenderer>::create();
		resultData->data = m_resultUIs[rID].main3DVis->extractSelectedObjects();
		if (resultData->data.size() == 0)
		{
			LOG(lvlDebug, QString("Result %1: No selected fibers!").arg(rID));
			continue;
		}
		resultData->renderer = vtkSmartPointer<vtkRenderer>::New();
		auto bgColor(qApp->palette().color(QPalette::Window));
		resultData->renderer->SetBackground(bgColor.redF(), bgColor.greenF(), bgColor.blueF());
		resultData->renderer->SetViewport(
			static_cast<double>(i) / hp.size(), 0, static_cast<double>(i + 1) / hp.size(), 1);
		for (int f = 0; f < resultData->data.size(); ++f)
		{
			auto diffMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
			diffMapper->SetInputData(resultData->data[f]);
			resultData->actor.push_back(vtkSmartPointer<vtkActor>::New());
			resultData->actor[resultData->actor.size()-1]->SetMapper(diffMapper);
			//resultData->actor[resultData->actor.size() - 1]->GetProperty()->SetColor(128, 128, 128);
			resultData->actor[resultData->actor.size() - 1]->GetProperty()->SetOpacity(0.5);
			diffMapper->SetScalarModeToUsePointFieldData();
			diffMapper->ScalarVisibilityOn();
			diffMapper->Update();
			resultData->renderer->AddActor(resultData->actor[f]);
		}
		auto txt = QString("Result %1").arg(rID);
		resultData->text = vtkSmartPointer<vtkCornerAnnotation>::New();
		resultData->text->SetLinearFontScaleFactor(2);
		resultData->text->SetNonlinearFontScaleFactor(1);
		resultData->text->SetMaximumFontSize(18);
		resultData->text->SetText(2, txt.toStdString().c_str());
		// ToDo: add fiber id ;
		auto textColor = qApp->palette().color(QPalette::Text);
		resultData->text->GetTextProperty()->SetColor(textColor.redF(), textColor.greenF(), textColor.blueF());
		//cornerAnnotation->GetTextProperty()->BoldOn();
		resultData->renderer->AddViewProp(resultData->text);

		//m_diffActor->GetProperty()->SetPointSize(2);
		/*
	#if VTK_VERSION_NUMBER < VTK_VERSION_CHECK(9, 0, 0)
		m_main3DWidget->GetRenderWindow()->GetRenderers()->GetFirstRenderer()->AddActor(m_diffActor);
		m_main3DWidget->GetRenderWindow()->Render();
	#else
		m_main3DWidget->renderWindow()->GetRenderers()->GetFirstRenderer()->AddActor(m_diffActor);
		m_main3DWidget->renderWindow()->Render();
	#endif
		m_main3DWidget->update();
		*/

		// show differences - for now only for 1st fiber, to 1st fiber of 1st selected result:
		if (i > 0)
		{
			auto refResID = hp[0];
			int refFiberID = m_currentFiberSelection[refResID][0];
			auto& ref = m_data->result[refResID];
			auto const& refMapping = *ref.mapping.data();
			auto refIt = ref.curveInfo.find(refFiberID);
			iAFiberData refFiber(ref.table, refFiberID, refMapping,
				refIt != ref.curveInfo.end() ? refIt->second : std::vector<iAVec3f>());

			auto& d = m_data->result[rID];
			auto const& mapping = *d.mapping.data();
			std::vector<iAVec3f> sampledPoints;
			int fiber0ID = m_currentFiberSelection[rID][0];
			auto it = d.curveInfo.find(fiber0ID);
			iAFiberData sampleFiber(
				d.table, fiber0ID, mapping, it != d.curveInfo.end() ? it->second : std::vector<iAVec3f>());

			vtkNew<vtkPolyData> ptData;
			vtkNew<vtkPoints> points;
			auto colors = vtkSmartPointer<vtkUnsignedCharArray>::New();
			colors->SetNumberOfComponents(3);
			colors->SetName("Colors");

			// direction fiber -> refFiber
			// everything that's in the fiber but not in the reference -> color red
			samplePoints(sampleFiber, sampledPoints, 1000);
			size_t newPts = 0;
			for (size_t s = 0; s < sampledPoints.size(); ++s)
			{
				if (!pointContainedInFiber(sampledPoints[s], refFiber))
				{
					double pt[3];
					for (int c = 0; c < 3; ++c) pt[c] = sampledPoints[s][c];
					points->InsertNextPoint(pt);
					++newPts;
				}
			}
			unsigned char red[3] = {255, 0, 0};
			for (size_t s = 0; s < newPts; ++s)
			{
				colors->InsertNextTypedTuple(red);
			}

			// direction refFiber -> fiber
			// -> color blue
			samplePoints(refFiber, sampledPoints, 1000);
			newPts = 0;
			for (size_t s = 0; s < sampledPoints.size(); ++s)
			{
				if (!pointContainedInFiber(sampledPoints[s], sampleFiber))
				{
					double pt[3];
					for (int c = 0; c < 3; ++c) pt[c] = sampledPoints[s][c];
					points->InsertNextPoint(pt);
					++newPts;
				}
			}
			unsigned char blue[3] = {0, 0, 255};
			for (size_t s = 0; s < newPts; ++s)
			{
				colors->InsertNextTypedTuple(blue);
			}

			ptData->SetPoints(points);
			auto vertexFilter = vtkSmartPointer<vtkVertexGlyphFilter>::New();
			vertexFilter->SetInputData(ptData);
			vertexFilter->Update();

			resultData->diffPoints = vtkSmartPointer<vtkPolyData>::New();
			resultData->diffPoints->DeepCopy(vertexFilter->GetOutput());
			resultData->diffPoints->GetPointData()->SetScalars(colors);
			resultData->diffPtMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
			resultData->diffPtMapper->SetInputData(resultData->diffPoints);
			resultData->diffActor = vtkSmartPointer<vtkActor>::New();
			resultData->diffActor->SetMapper(resultData->diffPtMapper);
			resultData->diffPtMapper->Update();
			resultData->diffActor->GetProperty()->SetPointSize(6);
			resultData->renderer->AddActor(resultData->diffActor);
		}
		renWin->AddRenderer(resultData->renderer);
		m_gui->m_diff3DRenderManager.addToBundle(resultData->renderer);
		m_gui->m_diff3DRenderers.push_back(resultData);
	}
	if (m_gui->m_diff3DRenderers.size() == 0 && !renWin->GetRenderers()->IsItemPresent(m_gui->m_diff3DEmptyRenderer))
	{
		renWin->AddRenderer(m_gui->m_diff3DEmptyRenderer);
	}
	else if (m_gui->m_diff3DRenderers.size() > 0 && renWin->GetRenderers()->IsItemPresent(m_gui->m_diff3DEmptyRenderer))
	{
		renWin->RemoveRenderer(m_gui->m_diff3DEmptyRenderer);
	}
	renWin->Render();
	m_gui->m_diff3DWidget->update();
}

void iASensitivityInfo::styleChanged()
{
	auto textColor = qApp->palette().color(QPalette::Text);
	m_gui->m_diff3DEmptyText->GetTextProperty()->SetColor(textColor.redF(), textColor.greenF(), textColor.blueF());
	auto bgColor = qApp->palette().color(QPalette::Window);
	m_gui->m_diff3DEmptyRenderer->SetBackground(bgColor.redF(), bgColor.greenF(), bgColor.blueF());
	for (auto r : m_gui->m_diff3DRenderers)
	{
		r->renderer->SetBackground(bgColor.redF(), bgColor.greenF(), bgColor.blueF());
		r->text->GetTextProperty()->SetColor(textColor.redF(), textColor.greenF(), textColor.blueF());
	}
}