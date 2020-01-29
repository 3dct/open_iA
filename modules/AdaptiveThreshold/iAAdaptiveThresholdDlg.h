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
#pragma once

#include "ui_AdaptiveThreshold.h"
#include "iAThresholdCalculator.h"

#include <charts/iAPlotData.h>

#include <QSharedPointer>
#include <QDialog>

#include <vector>

enum axisMode
{
	x,
	y
};

namespace QtCharts
{
	class QChart;
	class QChartView;
	class QLineSeries;
	class QValueAxis;
}

class  iAAdaptiveThresholdDlg : public QDialog, Ui_AdaptiveThreshold

{
Q_OBJECT

public:
	//! Create a new dialog, all parameters are optional
	iAAdaptiveThresholdDlg(QWidget * parent = 0, Qt::WindowFlags f = 0);

	void setupUIActions();
	void initChart();

	//init Axis in x and y, also with ticks
	void initAxes(double xmin, double xmax, double ymin, double yMax, bool setDefaultAxis);

	void prepareDataSeries(QtCharts::QXYSeries* aSeries, const std::vector<double>& x_vals, const std::vector<double>& y_vals, QString* grText, bool useDefaultValues, bool updateCoords);
	void addSeries(QtCharts::QXYSeries* aSeries, bool disableMarker);

	void setHistData(/*const*/ QSharedPointer<iAPlotData>& data);

	double resultingThreshold() const;
	double segmentationStartValue() const;

private slots:
	void updateChartClicked();
	void resetGraphToDefault();
	void calculateMovingAndVisualizeAverage();
	void buttonSelectRangesClickedAndComputePeaks();
	void determineIntersectionAndFinalThreshold();
	void redrawPlots();
	void rescaleToMinMax();
	void updateSegmentationRange(bool updateRange);
	void clearLog();

private:
	void computeNormalizeAndComputeLokalPeaks(threshold_defs::iAPeakRanges& ranges);
	void peakNormalization(threshold_defs::iAParametersRanges& maxPeakMaterialRanges, threshold_defs::iAPeakRanges& ranges, threshold_defs::iAThresMinMax& resultingthrPeaks);
	void visualizeIntermediateResults(threshold_defs::iAThresMinMax& resultingthrPeaks);
	void calculateIntermediateResults(threshold_defs::iAThresMinMax& resultingthrPeaks, threshold_defs::iAParametersRanges maxPeakMaterialRanges, bool updatePeaks);
	void assignValuesToField(threshold_defs::iAThresMinMax& thrPeaks);
	void assignValuesToField(double min, double max, double y1, double y2);
	void createVisualisation(threshold_defs::iAParametersRanges paramRanges, threshold_defs::iAThresMinMax thrPeaks);
	void visualizeSeries(threshold_defs::iAParametersRanges ParamRanges, QColor color, QString *seriesName);

	threshold_defs::iAThresMinMax determineLocalPeaks(threshold_defs::iAPeakRanges& ranges, threshold_defs::iAThresMinMax resultingthrPeaks);
	void visualizeFinalThreshold(double resThres);
	void OptionallyUpdateThrPeaks(bool selectedData, threshold_defs::iAThresMinMax& thrPeaks);

	void DetermineGraphRange();
	void prepareAxis(QValueAxis *axis, const QString &title, double min, double max, uint ticks, axisMode mode);
	void generateSampleData(bool addserries);
	void determineMinMax(const std::vector<double> &xVal, const std::vector<double> &yVal);
	void logText(const QString& Text);
	void setInputData(const std::vector<double> &thres_binInX, const std::vector<double> &freqValsInY);
	//TODO Refactoring
	void addAllSeries(std::vector<QXYSeries*> allSeries, bool disableMarker);

	void assignValuesFromField(threshold_defs::iAPeakRanges &Ranges);

	void writeDebugText(const QString& Text);

	void setTicks(uint xTicks, uint yTicks, bool update);

	void scaleGraphToMinMax(const threshold_defs::iAParametersRanges& ranges);

	threshold_defs::iAMovingFreqs allMovingfreqs;
	iAThresholdCalculator m_thresCalculator;
	threshold_defs::iAGraphRange m_graphValuesScope;

	threshold_defs::iAParametersRanges m_NormalizedGlobalValueRangeXY;
	threshold_defs::iAParametersRanges m_paramRanges;
	threshold_defs::iAThresMinMax m_resultingthrPeaks;
	threshold_defs::iAParametersRanges m_maxPeakMaterialRanges;

	bool updateValuesChecked = false;
	bool runOnFirstTime = true;

	double m_segmentationStartValue = 0;

	const double minXDefault = 0; const double maxXDefault = 65535;
	const double minYDefault = 0; const double maxYDefault = 40000;
	int m_average = 0;
	int m_colCounter = 0;

	const int m_defautTickCountsX = 10;
	const int m_defaultTickCountsY = 10;

	const int maxSeriesNumbers = 10;
	double m_xMinRef, m_xMaxRef, m_yMinRef, m_yMaxRef;

	std::vector<double> m_greyThresholds;
	std::vector<double> m_frequencies;
	std::vector<double> m_movingFrequencies;
	QtCharts::QLineSeries *m_refSeries;
	std::vector<QtCharts::QLineSeries*> series_vec;
	double resThreshold;

	QtCharts::QChartView* m_chartView;
	QtCharts::QChart* m_chart;
	QtCharts::QValueAxis* axisX;
	QtCharts::QValueAxis* axisY;
};

