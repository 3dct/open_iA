#include "dlg_AdaptiveThreshold.h"
#include <qsharedpointer.h>
#include <QtCharts/qlineseries.h>
#include "iAConsole.h"
#include "QColor"
#include "QPoint"
//#include <QTChart>

AdaptiveThreshold::AdaptiveThreshold(QWidget * parent/* = 0,*/, Qt::WindowFlags f/* f = 0*/):QDialog(parent, f){
	setupUi(this);
	try {
		m_chart = new QtCharts::QChart;
		m_chartView = new QtCharts::QChartView(m_chart);
		axisX = new QValueAxis; 
		axisY = new QValueAxis; 
		m_refSeries = new QLineSeries();
		series_vec.reserve(maxSeriesNumbers); 

		for (auto series: series_vec) {
			series = new QLineSeries(); 
		}

		connect(this->btn_update, SIGNAL(clicked()), this, SLOT(buttonUpdateClicked()));
	
		this->mainLayout->addWidget(m_chartView);
	}
	catch (std::bad_alloc &ba) {
		throw; 
	}
}

AdaptiveThreshold::~AdaptiveThreshold()
{
	delete m_chart;
	delete m_chartView; 

	for (auto p:series_vec) {
		if (p)
			delete p;
	}
	//delete m_refSeries; 
}

void AdaptiveThreshold::initChart()
{

	//QLineSeries *series = new QLineSeries();
	m_refSeries->append(0, 6);
	m_refSeries->append(2, 4);
	m_refSeries->append(3, 8);
	m_refSeries->append(7, 4);
	m_refSeries->append(10, 5);
	*m_refSeries << QPointF(2, 2) << QPointF(3, 3) << QPointF(20, 0); 
	QColor color = QColor(255,0,0); 
	m_refSeries->setColor(color);
	//m_refSeries->setLb
	m_chart->addSeries(m_refSeries);

	//m_chart->createDefaultAxes(); 
	//QValueAxis *axisY = new QValueAxis;

	axisY->setMin(4); 
	axisY->setMax(20);
	axisY->setTitleText("Y");

	//QValueAxis * axisX = new QValueAxis;
	//QValueAxis * axisY = new QValueAxis; 
	axisX->setTickCount(10);
	axisX->setTitleText("X");
	m_chart->addAxis(axisX, Qt::AlignBottom);
	
	axisY->setTitleText("Y");
	m_chart->addAxis(axisY, Qt::AlignLeft); 
	/*axisX->setMax(0); 
	axisX->setMin(10);*/
	//axisX->setTitleText("X"); 
	//series->attachAxis(axisX);
	m_refSeries->attachAxis(axisX);
	m_refSeries->attachAxis(axisY);
	//m_chart->addAxis(axisY, Qt::AlignLeft);
	/*m_chart->addAxis(axisY, Qt::AlignTop);
	m_chart->axisX(series)->setRange(0, 100);*/

	//m_chart->addAxis(axisY, Qt::AlignLeft); 
	//m_chart->setTitle("line chart example"); 
	
}

void AdaptiveThreshold::buttonUpdateClicked()
{
	DEBUG_LOG("Button update is clicked"); 
	this->m_chart->setTitle("Hallo"); 
	this->m_chart->update(); 
	this->m_chartView->update(); 
	//DEBUG_LOG("Button update is clicked"); 
	QString xmin, xMax, yMIn, yMax; 
	double xmin_val, xmax_val, ymin_val, ymax_val;

	xmin = this->ed_xMIn->text();
	xMax = this->ed_xMax->text();
	yMIn = this->ed_Ymin->text();
	yMax = this->ed_YMax->text(); 

	if (xmin.isEmpty() && xMax.isEmpty() && yMIn.isEmpty() && yMax.isEmpty()) {
		return;
	}

	xmin_val = xmin.toDouble();
	xmax_val = xMax.toDouble();
	ymin_val = yMIn.toDouble();
	ymax_val = yMIn.toDouble();

	axisX->setRange(xmin_val, xmax_val);
	m_chart->update(); 
}

