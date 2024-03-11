// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "ui_AdaptiveThreshold.h"
#include "iAThresholdCalculator.h"

#include <iAPlotData.h>

#include <QDialog>

#include <vector>

enum axisMode
{
	x,
	y
};

class QChart;
class QChartView;
class QLineSeries;
class QValueAxis;

class  iAAdaptiveThresholdDlg : public QDialog, Ui_AdaptiveThreshold

{
Q_OBJECT

public:
	//! Create a new dialog, all parameters are optional
	iAAdaptiveThresholdDlg(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
	void setupUIActions();
	void initAxes(double xmin, double xmax, double ymin, double yMax, bool setDefaultAxis);
	void prepareDataSeries(QXYSeries* aSeries, const std::vector<double>& x_vals,
		const std::vector<double>& y_vals, QString* grText, bool useDefaultValues, bool updateCoords);
	void addSeries(QXYSeries* aSeries, bool disableMarker);
	void setHistData(/*const*/ std::shared_ptr<iAPlotData> data);
	double resultingThreshold() const;
	double segmentationStartValue() const;

private slots:
	void updateChartClicked();
	void resetGraphToDefault();
	void calculateMovingAndVisualizeAverage();
	void computePeaksMinimum();
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

	threshold_defs::iAThresMinMax determineLocalPeaks(threshold_defs::iAPeakRanges& ranges);
	void visualizeFinalThreshold(double resThres);
	void OptionallyUpdateThrPeaks(bool selectedData, threshold_defs::iAThresMinMax& thrPeaks);

	void setGraphRangeFromInput();

	void prepareAxis(QValueAxis* axis, const QString& title, double min, double max, uint ticks, axisMode mode);
	void determineMinMax(const std::vector<double> &xVal, const std::vector<double> &yVal);
	void logText(const QString& Text);
	void setInputData(const std::vector<double> &thres_binInX, const std::vector<double> &freqValsInY);

	void assignValuesFromField(threshold_defs::iAPeakRanges &Ranges);

	void scaleGraphToMinMax(const threshold_defs::iAParametersRanges& ranges);

	iAThresholdCalculator m_thresCalculator;
	threshold_defs::iAParametersRanges m_NormalizedGlobalValueRangeXY;
	threshold_defs::iAParametersRanges m_paramRanges;
	threshold_defs::iAThresMinMax m_resultingthrPeaks;
	threshold_defs::iAParametersRanges m_maxPeakMaterialRanges;

	bool m_runOnFirstTime = true;

	double m_segmentationStartValue = 0;
	int m_colCounter = 0;

	const int m_defaultTickCountsX = 5;
	const int m_defaultTickCountsY = 5;

	double m_xMinRef, m_xMaxRef, m_yMinRef, m_yMaxRef;

	std::vector<double> m_greyThresholds;
	std::vector<double> m_frequencies;
	std::vector<double> m_movingFrequencies;

	QLineSeries* m_refSeries;
	QChartView* m_chartView;
	QChart* m_chart;
	QValueAxis* axisX;
	QValueAxis* axisY;
};
