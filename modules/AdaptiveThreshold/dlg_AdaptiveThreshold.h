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

	void setupUIActions();

	~AdaptiveThreshold();

	//void calculateMovingAvarage();	

	void initChart();

	void initAxes(double xmin, double xmax, double ymin, double yMax, bool setDefaultAxis); 	
	
	void prepareDataSeries(QXYSeries *aSeries, const std::vector<double> &x_vals, const std::vector<double> &y_vals, bool updateCoords);

    void addSeries(QXYSeries *aSeries); 
	
	//TBA those will be empty
	QXYSeries *createDataSeries(const std::vector<double> &xvals, const std::vector<double> &y_vals,plotMode mode); 

	inline void clearSeries(QXYSeries *series) {
		series->clear();

	}

	void setHistData(/*const*/ QSharedPointer<iAPlotData>& data);
	
	

private slots:
		void buttonUpdateClicked();
		void buttonLoadDataClicked(); 
		void buttonLoadHistDataClicked(); 
		void createSampleSeries();
		void clear(); 
		void resetGraphToDefault(); 
		void calculateMovingAverage(); 
		void myAction();
		void aTestAction(); 

		inline void clearEditField() {
			this->textEdit->clear();
		}

private:
	void prepareAxis(QValueAxis *axis, const QString &title, double min, double max, uint ticks, axisMode mode);
	void generateSampleData(bool addserries);
	void determineMinMax(const std::vector<double> &xVal, const std::vector<double> &yVal); 
	void setOutputText(const QString& Text); 
	void setInputData(const std::vector<double> &thres_binInX, const std::vector<double> &freqValsInY);
	//TODO Refactoring


	void setDefaultMinMax(double xMIn, double xMax, double yMin, double yMax); 
	//void setMinMaxToEdit(double xMin, double xMax, double yMin, double yMax); 

	const double minXDefault = 0; const double maxXDefault = 65535; 
	const double minYDefault = 0; const double maxYDefault = 40000; 
	int m_average = 0; 

	const int m_defautTickCountsX = 8;
	const int m_defaultTickCountsY = 10; 

	const int maxSeriesNumbers = 10; 
	double m_xMinRef, m_xMaxRef, m_yMinRef, m_yMaxRef; 
	ThesholdCalculator m_thresCalculator; 
	Loader m_seriesLoader; 

	std::vector<double> m_greyThresholds; 
	std::vector<double> m_frequencies; 
	std::vector<double> m_movingFrequencies; 	
	QLineSeries *m_refSeries; 
	std::vector<QLineSeries*> series_vec;
	double resThreshold; 

	
	QChartView *m_chartView;
	QChart *m_chart; 
	QValueAxis *axisX;
	QValueAxis *axisY; 
};

