#pragma once

#include "ui_AdaptiveThreshold.h"
#include "Loader.h"
#include "ThesholdCalculator.h"

#include <charts/iAPlotData.h>

#include <QSharedPointer>
#include <QDialog>

#include <vector>

enum axisMode {
	x,
	y
};

enum plotMode
{
	scatter,
	lines
};

namespace QtCharts
{
	class QChart;
	class QChartView;
	class QLineSeries;
	class QValueAxis;
}

class  AdaptiveThreshold : public QDialog, Ui_AdaptiveThreshold

{
Q_OBJECT

public:
	//! Create a new dialog, all parameters are optional
	AdaptiveThreshold(QWidget * parent = 0, Qt::WindowFlags f = 0);

	void enableComponents(bool setCompsVisible);

	void setupUIActions();
	void initChart();


	//init Axis in x and y, also with ticks
	void initAxes(double xmin, double xmax, double ymin, double yMax, bool setDefaultAxis); 	
	
	
	void prepareDataSeries(QXYSeries* aSeries, const std::vector<double>& x_vals, const std::vector<double>& y_vals, QString* grText, bool useDefaultValues, bool updateCoords);
	void addSeries(QXYSeries* aSeries, bool disableMarker);
	
	inline void clearSeries(QXYSeries * /*series*/) {
		//series->clear();

	}

	void setHistData(/*const*/ QSharedPointer<iAPlotData>& data);


	inline double getResultingThreshold() {
		return m_thresCalculator.GetResultingThreshold();
	}
	
	double SegmentationStartValue() const { return segmentationStartValue; }
	
	
	void SegmentationStartValue(double val) { segmentationStartValue = val; }
public slots:
	void buttonLoadHistDataClicked();

private slots:
		void UpdateChartClicked();
		void buttonLoadDataClicked();
		
		
		void clear(); 
		void resetGraphToDefault(); 
		void calculateMovingAndVisualizeAverage();
		void buttonSelectRangesClickedAndComputePeaks(); 

		void computeNormalizeAndComputeLokalPeaks(threshold_defs::PeakRanges& ranges);

		void peakNormalization(threshold_defs::ParametersRanges& maxPeakMaterialRanges, threshold_defs::PeakRanges& ranges, threshold_defs::ThresMinMax& resultingthrPeaks);

		void visualizeIntermediateResults(threshold_defs::ThresMinMax& resultingthrPeaks);

		void calculateIntermediateResults(threshold_defs::ThresMinMax& resultingthrPeaks, threshold_defs::ParametersRanges maxPeakMaterialRanges, bool updatePeaks);

		threshold_defs::ThresMinMax determineLocalPeaks(threshold_defs::PeakRanges& ranges, threshold_defs::ThresMinMax resultingthrPeaks);

		void enableCheckBox(); 

		void assignValuesToField(threshold_defs::ThresMinMax& thrPeaks);

		void assignValuesToField(double min, double max, double y1, double y2);
		void createVisualisation(threshold_defs::ParametersRanges paramRanges, threshold_defs::ThresMinMax thrPeaks);
		void visualizeSeries(threshold_defs::ParametersRanges ParamRanges, QColor color, QString *seriesName); 

		void determineIntersectionAndFinalThreshold(); 

	

		void buttonNormalizedClicked(); 
				
		void redrawPlots();
		void rescaleToMinMax(); 
		void updateSegmentationRange(bool updateRange);

		inline void clearEditField() {
			this->textEdit->clear();
		}

		

private:
	void visualizeFinalThreshold(double resThres);
	void OptionallyUpdateThrPeaks(bool selectedData, threshold_defs::ThresMinMax& thrPeaks);
	
	void DetermineGraphRange();
	void prepareAxis(QValueAxis *axis, const QString &title, double min, double max, uint ticks, axisMode mode);
	void generateSampleData(bool addserries);
	void determineMinMax(const std::vector<double> &xVal, const std::vector<double> &yVal); 
	void setOutputText(const QString& Text); 
	void setInputData(const std::vector<double> &thres_binInX, const std::vector<double> &freqValsInY);
	//TODO Refactoring
	void addAllSeries(std::vector<QXYSeries*> allSeries, bool disableMarker);

	void setDefaultMinMax(double xMIn, double xMax, double yMin, double yMax); 

	void assignValuesFromField(threshold_defs::PeakRanges &Ranges);

	inline void writeDebugText(const QString& Text) {
		if (Text.isNull() || Text.isEmpty()) return; 
		
		this->textEdit->append(Text);
		
	}

	inline void setTicks(uint xTicks, uint yTicks, bool update) {
		if ((xTicks > 0) && (yTicks > 0)) {
			axisX->setTickCount(xTicks);
			axisY->setTickCount(yTicks); 
		}
		else {
			writeDebugText(QString("Please set a tick count greater 0")); 
		}

		if (update) {
			m_chart->update();
			m_chartView->update(); 
		}
	}

	void scaleGraphToMinMax(const threshold_defs::ParametersRanges& ranges);

	
private:
	
	threshold_defs::MovingFreqs allMovingfreqs; 
	ThesholdCalculator m_thresCalculator; 
	threshold_defs::GraphRange m_graphValuesScope; 

	threshold_defs::ParametersRanges m_NormalizedGlobalValueRangeXY;
	threshold_defs::ParametersRanges m_paramRanges;
	threshold_defs::ThresMinMax m_resultingthrPeaks;
	threshold_defs::ParametersRanges m_maxPeakMaterialRanges;

	bool updateValuesChecked = false; 
	bool runOnFirstTime = true; 

	Loader m_seriesLoader; 

	double segmentationStartValue = 0; 
	
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

