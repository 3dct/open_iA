#pragma once

#include "ui_AdaptiveThreshold.h"
#include <QSharedPointer>
#include <QDialog>
#include <QTCharts>
#include <vector>
#include <QtCharts/qlineseries.h>
using namespace QtCharts;

struct Threshold{
	
};

class  AdaptiveThreshold : public QDialog, Ui_AdaptiveThreshold

{
Q_OBJECT

public:
	//! Create a new dialog, all parameters are optional
	AdaptiveThreshold(QWidget * parent = 0, Qt::WindowFlags f = 0);
	~AdaptiveThreshold(); 
	void calculateMovingAvarage();

	void loadValues(const QString &filename);

	void initChart(double xmin, double xmax, double ymin, double ymax);

	private slots:
		void buttonUpdateClicked();

private: 
	const int maxSeriesNumbers = 10; 

	double xMinRef, xMaxRef, yMinRef, yMaxRef; 

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

