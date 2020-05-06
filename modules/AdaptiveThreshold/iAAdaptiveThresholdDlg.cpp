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
#include "iAAdaptiveThresholdDlg.h"

#include "iAChartVisHelper.h"
#include "iAThresholdDefinitions.h"
#include "iAIntersectionDefinition.h"

#include <iAConnector.h>
#include <iAConsole.h>

#include <QColor>
#include <QPoint>
#include <QFileDialog>
#include <QtCharts>
#include <QtCharts/QLineSeries>

#include <algorithm>
#include <limits>


iAAdaptiveThresholdDlg::iAAdaptiveThresholdDlg(QWidget * parent, Qt::WindowFlags f)
	: QDialog(parent, f),
	m_xMinRef(std::numeric_limits<double>::infinity()),
	m_xMaxRef(std::numeric_limits<double>::infinity()),
	m_yMinRef(std::numeric_limits<double>::infinity()),
	m_yMaxRef(std::numeric_limits<double>::infinity())
{
	setupUi(this);
	m_chart = new QtCharts::QChart;
	m_chart->setMargins(QMargins(0, 0, 0, 0));
	m_chart->layout()->setContentsMargins(0, 0, 0, 0);
	m_chart->setBackgroundRoundness(0);
	m_chartView = new QtCharts::QChartView(m_chart);
	m_chartView->setRubberBand(QChartView::RectangleRubberBand);
	axisX = new QValueAxis;
	axisY = new QValueAxis;
	m_refSeries = new QLineSeries();
	QValidator* validator = new QDoubleValidator(0, 8000, 2, this);
	this->ed_minSegmRange->setValidator(validator);
	this->ed_minSegmRange->setText(QString("%1").arg(0));
	this->ed_maxThresholdRange->setReadOnly(true);

	Qt::WindowFlags flags = this->windowFlags();
	flags |= Qt::Tool;
	this->setWindowFlags(flags);

	spin_xTicks->setValue(m_defaultTickCountsX);
	spin_yTicks->setValue(m_defaultTickCountsY);
	setupUIActions();
	this->mainLayout->addWidget(m_chartView);
}

void iAAdaptiveThresholdDlg::setupUIActions()
{
	connect(btn_update, &QPushButton::clicked, this, &iAAdaptiveThresholdDlg::updateChartClicked);
	connect(btn_resetGraph, &QPushButton::clicked, this, &iAAdaptiveThresholdDlg::resetGraphToDefault);
	connect(btn_movingAverage, &QPushButton::clicked, this, &iAAdaptiveThresholdDlg::calculateMovingAndVisualizeAverage);
	connect(btn_clearLog, &QPushButton::clicked, this, &iAAdaptiveThresholdDlg::clearLog);
	connect(btn_detectPeaksMinimum, &QPushButton::clicked, this, &iAAdaptiveThresholdDlg::computePeaksMinimum);
	connect(btn_redraw, &QPushButton::clicked, this, &iAAdaptiveThresholdDlg::redrawPlots);
	connect(btn_determineFinalThreshold, &QPushButton::clicked, this, &iAAdaptiveThresholdDlg::determineIntersectionAndFinalThreshold);
	connect(checkBox_excludeThreshold, &QCheckBox::clicked, this, &iAAdaptiveThresholdDlg::updateSegmentationRange);
}

void iAAdaptiveThresholdDlg::determineMinMax(const std::vector<double> &xVal, const std::vector<double> &yVal)
{
	if ((xVal.size() == 0) || (yVal.size() == 0))
	{
		DEBUG_LOG("could not determine min and maximum of input range");
		return;
	}

	if (xVal.size() != yVal.size())
	{
		DEBUG_LOG("min max of input range are not of equal size");
		return;
	}

	m_xMinRef = *std::min_element(std::begin(xVal), std::end(xVal));
	m_xMaxRef = *std::max_element(std::begin(xVal), std::end(xVal));
	m_yMinRef = *std::min_element(std::begin(yVal), std::end(yVal));
	m_yMaxRef = *std::max_element(std::begin(yVal), std::end(yVal));
}

void iAAdaptiveThresholdDlg::logText(const QString& Text)
{
	assert(!Text.isNull() && !Text.isEmpty());
	logEdit->append(Text + "\n");
}

void iAAdaptiveThresholdDlg::setInputData(const std::vector<double> &thres_binInX,const std::vector<double> &freqValsInY)
{
	m_greyThresholds = thres_binInX;
	m_frequencies = freqValsInY;
}

void iAAdaptiveThresholdDlg::setHistData (QSharedPointer<iAPlotData> &newData)
{
	if (!newData)
	{
		throw std::invalid_argument("data empty");
	}
	m_thresCalculator.setData(newData);
	logText("Loading histogram data");
	QString text = "Grey value histogram data";
	m_thresCalculator.retrieveHistData();
	this->setInputData(m_thresCalculator.getThresBins(), m_thresCalculator.getFreqValsY());
	this->prepareDataSeries(m_refSeries, m_greyThresholds,
		m_frequencies, &text, false, true);
}

double iAAdaptiveThresholdDlg::resultingThreshold() const
{
	return m_thresCalculator.GetResultingThreshold();
}

double iAAdaptiveThresholdDlg::segmentationStartValue() const
{
	return m_segmentationStartValue;
}

void iAAdaptiveThresholdDlg::resetGraphToDefault()
{
	this->initAxes(m_xMinRef, m_xMaxRef, m_yMinRef, m_yMaxRef, false);
	this->ed_XMin->setText(QString("%1").arg(m_xMinRef));
	this->ed_XMax->setText(QString("%1").arg(m_xMaxRef));
	this->ed_YMin->setText(QString("%1").arg(m_yMinRef));
	this->ed_YMax->setText(QString("%1").arg(m_yMaxRef));
	spin_xTicks->setValue(m_defaultTickCountsX);
	spin_yTicks->setValue(m_defaultTickCountsY);
	m_chart->update();
	m_chartView->update();
}

void iAAdaptiveThresholdDlg::calculateMovingAndVisualizeAverage()
{
	uint averageCount = this->spinBox_average->text().toUInt();
	if (averageCount <= 2)
	{
		return;
	}

	//reset the frequency
	if (m_movingFrequencies.size() > 0)
	{
		m_movingFrequencies.clear();
	}

	QString text = QString("Moving average %1").arg(averageCount);
	m_thresCalculator.calculateMovingAverage(m_frequencies, m_movingFrequencies, averageCount);

	/*prepare visualisation*/
	QLineSeries *newSeries = new QLineSeries;
	this->prepareDataSeries(newSeries, m_greyThresholds, m_movingFrequencies, &text, false,false);

}

void iAAdaptiveThresholdDlg::computePeaksMinimum()
{
	/*
	determine ranges for lokal peak
	determine range for maxPeak
	calculate fmax_lokal (air peak)
	calculate fmin_lokal (min peak between air and material)

	determine iso50
	 */
	threshold_defs::iAPeakRanges ranges;
	assignValuesFromField(ranges);

	try
	{
		//load values from checkbox
		computeNormalizeAndComputeLokalPeaks(ranges);
	}
	catch (std::invalid_argument& ia)
	{
		logText(ia.what());
	}
	this->chck_box_RecalcRange->setEnabled(true);
}

void iAAdaptiveThresholdDlg::computeNormalizeAndComputeLokalPeaks(threshold_defs::iAPeakRanges& ranges)
{
	if (m_movingFrequencies.empty())
	{
		logText("moving average not yet created, create average sequence before");
		return;
	}

	// first take moving current moving frequencies
	if (m_runOnFirstTime || this->chck_box_RecalcRange->isChecked())
	{
		m_thresCalculator.specifyRange(m_greyThresholds, m_movingFrequencies,
			m_NormalizedGlobalValueRangeXY, m_xMinRef, m_xMaxRef);

		m_resultingthrPeaks = determineLocalPeaks(ranges, m_resultingthrPeaks); //Peak Air (lokal Maxium) and Peak Min (lokal Min) are calculated
		//m_ maxPeakMaterialRanges;
		peakNormalization(m_maxPeakMaterialRanges, ranges, m_resultingthrPeaks);

		//here maybe update first maximum
		//mininum peak

		calculateIntermediateResults(m_resultingthrPeaks,m_maxPeakMaterialRanges, false);

		QColor cl_green = QColor(255, 0, 0);
		QString sr_text = "Max Peak Range";
		visualizeSeries(m_maxPeakMaterialRanges, cl_green, &sr_text);
		visualizeIntermediateResults(m_resultingthrPeaks);
		createVisualisation(m_paramRanges, m_resultingthrPeaks); //lines in histogram
		assignValuesToField(m_resultingthrPeaks);
		this->chckbx_LokalMinMax->setEnabled(true);
		logText("After Normalisation");
		logText(m_resultingthrPeaks.resultsToString(false));
		m_runOnFirstTime = false;
	}
	else
	{
		//if values are changes
		calculateIntermediateResults(m_resultingthrPeaks, m_maxPeakMaterialRanges, this->chckbx_LokalMinMax->isChecked());
		createVisualisation(m_paramRanges, m_resultingthrPeaks);
	}
}

void iAAdaptiveThresholdDlg::peakNormalization(threshold_defs::iAParametersRanges& maxPeakMaterialRanges, threshold_defs::iAPeakRanges& ranges, threshold_defs::iAThresMinMax& resultingthrPeaks)
{
	//determine global maximum HighPeakRanges in specified intervall
	m_thresCalculator.specifyRange(m_greyThresholds, m_movingFrequencies, maxPeakMaterialRanges, ranges.HighPeakXMin, ranges.HighPeakXMax);

	std::vector<double> tmp_Max = maxPeakMaterialRanges.getXRange();

	double globalMax = m_thresCalculator.getMaxPeakofRange(tmp_Max); //passt
	double lokalMax = resultingthrPeaks.getAirPeakThr();

	//min max normalization
	m_thresCalculator.performGreyThresholdNormalisation(m_NormalizedGlobalValueRangeXY, lokalMax, globalMax);
	m_thresCalculator.setNormalizedRange(m_NormalizedGlobalValueRangeXY);
	this->scaleGraphToMinMax(m_NormalizedGlobalValueRangeXY);

	this->ed_XMin->setText(QString("%1").arg(m_NormalizedGlobalValueRangeXY.getXMin()));
	this->ed_XMax->setText(QString("%1").arg(m_NormalizedGlobalValueRangeXY.getXMax()));
}

void iAAdaptiveThresholdDlg::visualizeIntermediateResults(threshold_defs::iAThresMinMax& resultingthrPeaks)
{
	auto* GlobalRangeGraph = createLineSeries(m_NormalizedGlobalValueRangeXY);
	GlobalRangeGraph->setColor(QColor(255, 0, 0));
	GlobalRangeGraph->setName("Normalized Series");

	this->addSeries(GlobalRangeGraph, false);

	QPointF f1 = QPointF(resultingthrPeaks.Iso50ValueThr(), m_yMaxRef);
	QPointF f2 = QPointF(resultingthrPeaks.Iso50ValueThr(), 0);

	QLineSeries* iso50 = nullptr;

	iso50 = createLineSeries(f2, f1, LineVisOption::horizontally);

	QColor cl_blue = QColor(0, 0, 255);
	iso50->setColor(cl_blue);
	iso50->setName("iso 50");

	this->addSeries(iso50, false);
}

void iAAdaptiveThresholdDlg::calculateIntermediateResults(threshold_defs::iAThresMinMax& resultingthrPeaks, threshold_defs::iAParametersRanges maxPeakMaterialRanges, bool updatePeaks)
{
	OptionallyUpdateThrPeaks(updatePeaks, resultingthrPeaks);
	resultingthrPeaks.fAirPeakHalf(resultingthrPeaks.FreqPeakLokalMaxY() / 2.0f); //f_air/2.0;

	//iso 50 as grey threshold
	m_thresCalculator.determinIso50(maxPeakMaterialRanges, resultingthrPeaks);
	//here calculations are finished
	double xminThr = resultingthrPeaks.getAirPeakThr();
	double xmaxThr = resultingthrPeaks.getMaterialsThreshold();

	resultingthrPeaks.setPeaksMinMax(xminThr, xmaxThr);

	//then minmaxnormalize x values for later intersection calculation;
	logText("Before Normalisation");
	logText(resultingthrPeaks.resultsToString(false));

	resultingthrPeaks.normalizeXValues(xminThr, xmaxThr);
	m_thresCalculator.setCalculatedResults(resultingthrPeaks);
}

threshold_defs::iAThresMinMax iAAdaptiveThresholdDlg::determineLocalPeaks(threshold_defs::iAPeakRanges& ranges, threshold_defs::iAThresMinMax resultingthrPeaks)
{
	//input grauwerte und moving freqs, output is paramRanges
	m_thresCalculator.specifyRange(m_greyThresholds, m_movingFrequencies, m_paramRanges, ranges.XRangeMin, ranges.XRangeMax/*x_min, x_max*/);

	//calculate lokal peaks
	resultingthrPeaks = m_thresCalculator.calcMinMax(m_paramRanges);
	return resultingthrPeaks;
}

void iAAdaptiveThresholdDlg::OptionallyUpdateThrPeaks(bool selectedData, threshold_defs::iAThresMinMax& thrPeaks)
{
	if (!selectedData)
	{
		return;
	}

	double lokalMax_X = ed_PeakThrMaxX->text().toDouble();
	double lokalMax_Y = ed_PeakFreqMaxY->text().toDouble();
	double lokalMin_X = ed_minPeakThrX->text().toDouble();
	double lokalMin_Y = ed_minPeakFreqY->text().toDouble();
	thrPeaks.updateMinMaxPeaks(lokalMin_X, lokalMin_Y, lokalMax_X, lokalMax_Y);
}

void iAAdaptiveThresholdDlg::assignValuesToField(threshold_defs::iAThresMinMax& thrPeaks)
{
	this->ed_PeakThrMaxX->setText(QString("%1").arg(thrPeaks.LokalMaxPeakThreshold_X()));
	this->ed_PeakFreqMaxY->setText(QString("%1").arg(thrPeaks.FreqPeakLokalMaxY()));
	this->ed_minPeakThrX->setText(QString("%1").arg(thrPeaks.PeakMinXThreshold()));
	this->ed_minPeakFreqY->setText(QString("%1").arg(thrPeaks.FreqPeakMinY()));
}


void iAAdaptiveThresholdDlg::assignValuesToField(double min, double max, double y1, double y2)
{
	this->ed_PeakThrMaxX->setText(QString("%1").arg(min));
	this->ed_PeakFreqMaxY->setText(QString("%1").arg(max));
	this->ed_minPeakThrX->setText(QString("%1").arg(y1));
	this->ed_minPeakFreqY->setText(QString("%1").arg(y2));
}


void iAAdaptiveThresholdDlg::createVisualisation(threshold_defs::iAParametersRanges paramRanges, threshold_defs::iAThresMinMax thrPeaks)
{
	QLineSeries* rangedSeries = nullptr;
	rangedSeries = createLineSeries(paramRanges);
	QPointF lokalPeakMax(thrPeaks.LokalMaxPeakThreshold_X(), thrPeaks.FreqPeakLokalMaxY());
	QPointF p2(thrPeaks.PeakMinXThreshold(), thrPeaks.FreqPeakMinY());

	if (!rangedSeries)
	{
		DEBUG_LOG("Range series not created");
		return;
	}

	m_colCounter += 20;
	if (m_colCounter > 255)
	{
		m_colCounter = 0;
	}

	QColor basis = QColor(0, 0, m_colCounter);
	std::vector<QXYSeries*> newData;
	auto SeriesTwoPoints = createLineSeries(lokalPeakMax, lokalPeakMax, LineVisOption::vertically);
	auto SeriesTwoPointsb = createLineSeries(lokalPeakMax, lokalPeakMax, LineVisOption::horizontally);
	SeriesTwoPointsb->setColor(basis);
	SeriesTwoPoints->setColor(basis);

	newData.push_back(SeriesTwoPoints);
	newData.push_back(SeriesTwoPointsb);

	QPointF lokalFMax_2(thrPeaks.LokalMaxPeakThreshold_X(), thrPeaks.fAirPeakHalf());
	auto seriesFMaxHalf = createLineSeries(lokalFMax_2, lokalFMax_2, LineVisOption::vertically);
	auto seriesFMaxHalf_2 = createLineSeries(lokalFMax_2, lokalFMax_2, LineVisOption::horizontally);

	QPointF lokalFmaxEnd = QPointF(65000.0f, lokalFMax_2.y());
	auto seriesFMaxHal_3 = createLineSeries(lokalFMax_2, lokalFmaxEnd, LineVisOption::horizontal_xy);

	this->addSeries(seriesFMaxHalf, true);
	this->addSeries(seriesFMaxHalf_2, true);
	this->addSeries(seriesFMaxHal_3, true);

	auto series_p2 = createLineSeries(p2, p2, LineVisOption::horizontally);
	auto series_p2_b = createLineSeries(p2, p2, LineVisOption::vertically);

	series_p2_b->setColor(QColor(basis));
	series_p2->setColor(QColor(basis));
	QColor color = QColor(255, 0, 0);
	rangedSeries->setColor(color);
	newData.push_back(rangedSeries);
	newData.push_back(series_p2);
	newData.push_back(series_p2);

	this->addSeries(series_p2, true);
	for (QXYSeries* el : newData)
	{
		this->addSeries(el, /*disableMarker*/true);
	}
	m_chartView->update();
	logText(thrPeaks.MinMaxToString());
}

void iAAdaptiveThresholdDlg::visualizeSeries(threshold_defs::iAParametersRanges ParamRanges, QColor color, QString *seriesName)
{
	QLineSeries* rangedSeries = nullptr;
	rangedSeries = createLineSeries(ParamRanges);
	rangedSeries->setColor(color);
	if (seriesName)
	{
		rangedSeries->setName(*seriesName);
	}

	this->addSeries(rangedSeries, true);

	m_chart->update();
	m_chartView->update();
}

void iAAdaptiveThresholdDlg::determineIntersectionAndFinalThreshold()
{
	//workflow select range for choosing points
	//calculate first intersection within the range
	//visualiseIntersectionPoint

	double xmin, xmax;
	xmin = this->ed_ptXmin->text().toDouble();
	xmax = this->ed_pt_xMax->text().toDouble();

	if ((xmin < 0) || (xmax < 0) || (xmax < xmin))
	{
		return;
	}
	if (m_greyThresholds.size() == 0)
	{
		logText("Workflow: \n 1 Please load histogram data first\n 2 create Moving average\n 3 Specify Ranges of Peaks \n4  Calculate Intersection");
		return;
	}
	if (m_movingFrequencies.size() == 0)
	{
		logText("Moving Frequencies not created");
		return;
	}
	try
	{
		//calculate fair/2
		QPointF lokalMaxHalf = m_thresCalculator.getPointAirPeakHalf();
		QString peakHalf = QString("fmin/2 %1 %2").arg(lokalMaxHalf.x()).arg(lokalMaxHalf.y());

		//TODO REPLACE BY MAX limits
		//fair_half end point for visualisation only
		QPointF LokalMaxHalfEnd(m_xMaxRef, lokalMaxHalf.y());

		//prepare line for intersection with fpeak half
		QLineF LinePeakHalf(lokalMaxHalf, LokalMaxHalfEnd);

		//points for intersection x,y - Werte of normalized Hist for Line Intersections
		threshold_defs::iAParametersRanges Intersectranges;

		logText(QString("ranges for crossing of fair_half with Hist (xmin) (xmax) %1 %2").arg(xmin).arg(xmax)) ;

		//outvalues intersectranges:
		m_thresCalculator.rangeFromParamRanges(m_thresCalculator.getNormalizedRangedValues(), Intersectranges, xmin, xmax);
		//determine line intersection
		//create points for intersection
		auto intersectionPoints = intersectLineWithRange(LinePeakHalf, Intersectranges);

		QPointF ptIntersect(std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity());

		//find first point ptIntersect in the intersection intervall xmin, xmax
		m_thresCalculator.getFirstElemInRange(intersectionPoints, xmin, xmax, &ptIntersect);

		m_thresCalculator.setIntersectionPoint(ptIntersect);

		logText("Determined threshold");

		// visualize intersections
		QColor col = QColor(0, 255, 0);
		QPointF p1 = QPointF(ptIntersect.x(), 0);
		QPointF p2 = QPointF(ptIntersect.x(), 1000000);

		auto* IntersectSeries = createLineSeries(p1, p2, LineVisOption::horizontally);
		logText(QString("intersection point 1% %2").arg(ptIntersect.x()).arg(ptIntersect.y()));

		// Intersection is created;
		logText("Applying decision rule");
		QPointF calcThresPoint = m_thresCalculator.determineResultingThresholdBasedOnDecisionRule(m_thresCalculator.getResults(), logEdit);

		if (ptIntersect.x() < 0 || (ptIntersect.x() > std::numeric_limits<float>::max()))
		{
			logText("no intersection negative or inf, try again parametrisation");
			return;
		}

		m_thresCalculator.SetResultingThreshold(calcThresPoint.x());
		logText(QString("P(x,y) %1 %2").arg(calcThresPoint.x()).arg(calcThresPoint.y()));
		//todo check for inf values

		// convert threshold back to min max
		double resThres = m_thresCalculator.GetResultingThreshold();
		auto peaks = m_thresCalculator.getThrPeaksVals();
		double convertedThr = mapNormTo(peaks.getLocalMax(), peaks.getGlobalMax(),resThres);

		this->ed_maxThresholdRange->setText(QString("%1").arg(convertedThr));
		m_thresCalculator.SetResultingThreshold(convertedThr);

		logText(QString("Result final segmentation:\n grey value %1, normalised %2").arg(convertedThr).arg(resThres)) ;

		IntersectSeries->setColor(col);
		IntersectSeries->setName("Intersection Material with fmin/2");

		this->addSeries(IntersectSeries, false);

		visualizeFinalThreshold(resThres);

		if (resThres < 0)
		{
			logText(QString("Resulting segmentation threshold either negative or inf, try again with different parametrisation."));
			return;
		}
		m_chart->update();
		m_chartView->update();
		this->ed_minSegmRange->setEnabled(true);
		btn_performSegmentation->setEnabled(true);
	}
	catch (std::invalid_argument& iae)
	{
		logText(iae.what());
	}
}

void iAAdaptiveThresholdDlg::visualizeFinalThreshold(double resThres)
{
	auto* finalThresSeries = createLineSeries(QPointF(resThres, 0), QPointF(resThres, 10000000), horizontal_xy);
	finalThresSeries->setColor(QColor(100, 0, 170));
	finalThresSeries->setName("Determined segmentation threshold");

	this->addSeries(finalThresSeries, false);

	auto pen = finalThresSeries->pen();
	pen.setWidth(4);
	pen.setStyle(Qt::DashDotDotLine);
	finalThresSeries->setPen(pen);
}

void iAAdaptiveThresholdDlg::assignValuesFromField(threshold_defs::iAPeakRanges &Ranges)
{
	bool x1_ok, x2_ok, x3_ok, x4_ok;

	Ranges.XRangeMin = this->ed_minRange->text().toDouble(&x1_ok);
	Ranges.XRangeMax = this->ed_maxRange->text().toDouble(&x2_ok);
	Ranges.HighPeakXMax = this->ed_MaxPeakXRangeMax->text().toDouble(&x3_ok);
	Ranges.HighPeakXMin = this->ed_MaxPeakXRangeMin->text().toDouble(&x4_ok);


	if ((!x1_ok) || (!x2_ok) || (!x3_ok) || (!x4_ok) )
	{
		logEdit->append("Invalid input!");
		return;
	}
	else if ((Ranges.XRangeMin < 0) || (Ranges.XRangeMax <= 0))
	{
		logEdit->append("Please check input parameters - ");
		return;
	}
}

void iAAdaptiveThresholdDlg::scaleGraphToMinMax(const threshold_defs::iAParametersRanges& ranges)
{
	if (ranges.isXEmpty() || ranges.isYEmpty())
	{
		throw std::invalid_argument("greyvalues or frequencies not set");
	}
	axisX->setRange(ranges.getXMin(), ranges.getXMax());
	m_chart->update();
	m_chartView->update();
}

void iAAdaptiveThresholdDlg::redrawPlots()
{
	double xmin = this->ed_XMin->text().toDouble();
	double xmax = this->ed_XMax->text().toDouble();
	double ymin = this->ed_YMin->text().toDouble();
	double ymax = this->ed_YMax->text().toDouble();

	//redraw histogram
	//redraw moving averages

	if (m_chart)
	{
		if (m_chartView)
		{
			DEBUG_LOG("Delete viewer");
			auto sList = m_chart->series();

			QListIterator<QAbstractSeries*> iter(sList);
			while (iter.hasNext())
			{
				auto* oldseries = iter.next();
				oldseries->detachAxis(axisX);
				oldseries->detachAxis(axisY);
			}

			m_chart->removeAxis(axisX);
			m_chart->removeAxis(axisY);

			disconnect(m_chart);
			disconnect(m_chartView);

			delete m_chart;
			delete m_chartView;
		}
	}

	m_chart = new QChart();
	m_chartView = new QtCharts::QChartView(m_chart);

	this->initAxes(xmin, xmax, ymin, ymax, true);

	//loading original data

	if (this->m_refSeries)
	{
		delete m_refSeries;
	}
	m_refSeries = new QLineSeries;
	QString hist = "Histogram Data";

	this->prepareDataSeries(m_refSeries, m_greyThresholds,
		m_frequencies, &hist, true, true);

	//series have to be redone
	this->addSeries(m_refSeries, false);
	m_chart->update();
	m_chartView->update();
	this->mainLayout->addWidget(m_chartView);
}

void iAAdaptiveThresholdDlg::rescaleToMinMax()
{
	double xmin_val = m_xMinRef;
	double xmax_val = m_xMaxRef;
	double ymin_val = m_yMinRef;
	double ymax_val = m_yMaxRef;

	if ((xmin_val < xmax_val) && (ymin_val < ymax_val))
	{
		axisX->setRange(xmin_val, xmax_val);
		axisY->setRange(ymin_val, ymax_val);

		this->ed_XMin->setText(QString("%1").arg(xmin_val));
		this->ed_XMax->setText(QString("%1").arg(xmax_val));
		this->ed_YMin->setText(QString("%1").arg(ymin_val));
		this->ed_YMax->setText(QString("%1").arg(ymax_val));

		m_chart->update();
		m_chartView->update();
	}
}

void iAAdaptiveThresholdDlg::updateSegmentationRange(bool updateRange)
{
	if (!updateRange)
	{
		m_segmentationStartValue = 0;
		return;
	}
	m_segmentationStartValue = this->ed_minSegmRange->text().toDouble();
}

void iAAdaptiveThresholdDlg::clearLog()
{
	logEdit->clear();
}

void iAAdaptiveThresholdDlg::initAxes(double xmin, double xmax, double ymin, double yMax, bool setDefaultAxis)
{
	QString titleX = "Grey values";
	QString titleY = "Frequencies";

	//TODO welchen sinn hat das
	if (setDefaultAxis)
	{
		return;
	}

	prepareAxis(axisX, titleX, xmin, xmax, m_defaultTickCountsX, axisMode::x);
	prepareAxis(axisY, titleY, ymin, yMax, m_defaultTickCountsY, axisMode::y);
}

void iAAdaptiveThresholdDlg::prepareAxis(QValueAxis *axis, const QString &title, double min, double max, uint ticks, axisMode mode)
{
	axis->setRange(min, max);
	axis->setTickCount(ticks);
	axis->setTitleText(title);

	switch (mode)
	{
	case axisMode::x:
		m_chart->addAxis(axis, Qt::AlignBottom);
		break;
	case axisMode::y:
		m_chart->addAxis(axis, Qt::AlignLeft);
		break;
	default:
		break;
	}
}

void iAAdaptiveThresholdDlg::prepareDataSeries(QXYSeries *aSeries,
	const std::vector<double> &x_vals, const std::vector<double> &y_vals,
	QString *grText, bool useDefaultValues, bool updateCoords)
{
	if (!aSeries)
	{
		DEBUG_LOG("series is null");
		return;
	}
	if ((x_vals.size() == 0) || (y_vals.size() == 0))
	{
		DEBUG_LOG("entries are empty");
		return;
	}

	if (!(x_vals.size() == y_vals.size()))
	{
		DEBUG_LOG("x, y size different");
		return;
	}
	if (!useDefaultValues)
	{
		determineMinMax(x_vals, y_vals);
	}
	else
	{
		setGraphRangeFromInput();
	}

	if (updateCoords)
	{
		this->resetGraphToDefault();
	}

	size_t lenght = x_vals.size();

	for (size_t i = 0; i < lenght; ++i)
	{
		double tmp_x = 0; double tmp_y = 0;
		tmp_x = x_vals[i];
		tmp_y = y_vals[i];
		*aSeries << QPointF(tmp_x, tmp_y);
	}

	this->addSeries(aSeries, false);
	if (grText)
	{
		QString stext = *grText;
		aSeries->setName(stext);
	}
	this->m_chart->update();
	this->m_chartView->update();
}

void iAAdaptiveThresholdDlg::addSeries(QXYSeries* aSeries, bool disableMarker)
{
	if (!aSeries)
	{
		return;
	}

	this->m_chart->addSeries(aSeries);

	if (disableMarker)
	{
		m_chart->legend()->markers(aSeries)[0]->setVisible(false);
	}
	aSeries->attachAxis(axisX);
	aSeries->attachAxis(axisY);
	m_chart->update();
	m_chartView->update();
}

void iAAdaptiveThresholdDlg::updateChartClicked()
{
	this->m_chart->setTitle("Grey value distribution");

	setGraphRangeFromInput();

	uint xTicks = spin_xTicks->value();
	uint yTicks = spin_yTicks->value();
	axisX->setTickCount(xTicks);
	axisY->setTickCount(yTicks);

	m_chart->update();
	m_chartView->update();
}

void iAAdaptiveThresholdDlg::setGraphRangeFromInput()
{
	QString xMin = this->ed_XMin->text();
	QString xMax = this->ed_XMax->text();
	QString yMin = this->ed_YMin->text();
	QString yMax = this->ed_YMax->text();

	if (xMin.isEmpty() || xMax.isEmpty() || yMin.isEmpty() || yMax.isEmpty())
	{
		return;
	}

	double xmin_val = xMin.toDouble();
	double xmax_val = xMax.toDouble();
	double ymin_val = yMin.toDouble();
	double ymax_val = yMax.toDouble();

	axisX->setRange(xmin_val, xmax_val);
	axisY->setRange(ymin_val, ymax_val);
}
