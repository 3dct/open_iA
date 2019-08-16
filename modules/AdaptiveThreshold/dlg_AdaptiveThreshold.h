#pragma once

#include "ui_AdaptiveThreshold.h"
#include "Loader.h"
#include "ThesholdCalculator.h"
#include "charts/iAPlotData.h"

#include <QSharedPointer>
#include <QDialog>
#include <QtCharts>
#include <vector>
#include <QtCharts/qlineseries.h>


using namespace QtCharts;

class QtCharts::QXYSeries; 

enum axisMode {
	x,
	y
};

enum plotMode
{
	scatter,
	lines
};

class  AdaptiveThreshold : public QDialog, Ui_AdaptiveThreshold

{
Q_OBJECT

public:
	//! Create a new dialog, all parameters are optional
	AdaptiveThreshold(QWidget * parent = 0, Qt::WindowFlags f = 0);
	~AdaptiveThreshold();

	void setupUIActions();
	void initChart();


	//init Axis in x and y, also with ticks
	void initAxes(double xmin, double xmax, double ymin, double yMax, bool setDefaultAxis); 	
	
	//void prepareDataSeries(QXYSeries *aSeries, const std::vector<double> &x_vals, const std::vector<double> &y_vals, QString *grText, bool updateCoords);
	void prepareDataSeries(QXYSeries* aSeries, const std::vector<double>& x_vals, const std::vector<double>& y_vals, QString* grText, bool useDefaultValues, bool updateCoords);
	void addSeries(QXYSeries* aSeries, bool disableMarker);
	//TBA

	inline void clearSeries(QXYSeries *series) {
		//series->clear();

	}

	void setHistData(/*const*/ QSharedPointer<iAPlotData>& data);
	
private slots:
		void UpdateChartClicked();
		void buttonLoadDataClicked();
		void buttonLoadHistDataClicked(); 
		
		void clear(); 
		void resetGraphToDefault(); 
		void calculateMovingAverage();
		void buttonSelectRangesClicked(); 

		void assignValuesToField(threshold_defs::ThresMinMax& thrPeaks);

		void createVisualisation(threshold_defs::ParametersRanges paramRanges, threshold_defs::ThresMinMax thrPeaks);
		void visualizeSeries(threshold_defs::ParametersRanges ParamRanges, QColor color, QString *seriesName); 

		void buttonVisualizePointsClicked(); 
		void buttonMinMaxClicked();
		void redrawPlots();
		void rescaleToMinMax(); 


		//TBa remove test actions below
		void myAction();
		void aTestAction_2(); 
		void sortTestAction(); 

		void aTestAction(); 

		void AnotherAction(); 
		//end tba
		inline void clearEditField() {
			this->textEdit->clear();
		}

private:
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

	inline void writeText(const QString& Text) {
		if (Text.isNull() || Text.isEmpty()) return; 
		
		this->textEdit->append(Text);
		
	}
	inline void setTicks(uint xTicks, uint yTicks, bool update) {
		if ((xTicks > 0) && (yTicks > 0)) {
			axisX->setTickCount(xTicks);
			axisY->setTickCount(yTicks); 
		}
		else {
			writeText(QString("Please set a tick count greater 0")); 
		}

		if (update) {
			m_chart->update();
			m_chartView->update(); 
		}
	}
		
private: 
	
	threshold_defs::MovingFreqs allMovingfreqs; 
	ThesholdCalculator m_thresCalculator; 
	threshold_defs::GraphRange m_graphRange; 

	Loader m_seriesLoader; 


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
	QLineSeries *m_refSeries; 
	std::vector<QLineSeries*> series_vec;
	double resThreshold; 

	QChartView* m_chartView;
	QChart* m_chart;
	QValueAxis* axisX;
	QValueAxis* axisY;
};

