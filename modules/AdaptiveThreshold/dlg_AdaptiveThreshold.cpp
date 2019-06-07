#include "dlg_AdaptiveThreshold.h"
#include <qsharedpointer.h>
#include <QtCharts/qlineseries.h>
#include "iAConsole.h"
#include "QColor"
#include "QPoint"
#include <QFileDialog>
//#include "QtCharts/L"
//#include <QTChart>

AdaptiveThreshold::AdaptiveThreshold(QWidget * parent/* = 0,*/, Qt::WindowFlags f/* f = 0*/):QDialog(parent, f){
	setupUi(this);
	try {
		m_chart = new QtCharts::QChart;
		m_chartView = new QtCharts::QChartView(m_chart);
		m_chartView->setRubberBand(QChartView::HorizontalRubberBand);
		axisX = new QValueAxis; 
		axisY = new QValueAxis; 
		m_refSeries = new QLineSeries();
		series_vec.reserve(maxSeriesNumbers); 

		for (auto series: series_vec) {
			series = new QLineSeries(); 
		}

		connect(this->btn_update, SIGNAL(clicked()), this, SLOT(buttonUpdateClicked()));
		connect(this->btn_loadData, SIGNAL(clicked()), this, SLOT(buttonLoadDataClicked())); 
		connect(this->btn_TestData, SIGNAL(clicked()), this, SLOT(createSampleSeries()));
		//connect()
	
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

void AdaptiveThreshold::calculateMovingAvarage()
{

}

void AdaptiveThreshold::initChart(/*double xmin, double xmax, double ymin, double ymax*/)
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
	m_refSeries->setName("TestSeries"); 
	

	QScatterSeries * series = new QScatterSeries(); 
	series->append(4, 8);
	series->append(2, 20); 
	series->setName("AName");
	series->setColor(QColor(0, 255, 0)); 
	//m_refSeries->set
	//m_refSeries->setLb
	m_chart->addSeries(m_refSeries);
	m_chart->addSeries(series);
	QLineSeries * s2 = new QLineSeries();
	s2->append(8, 0);
	s2->append(8, 8);

	//m_chart->legend()->markers(s2)->setVisible(false);
	
	//s2->set
	m_chart->addSeries(s2);
	m_chart->legend()->markers(s2)[0]->setVisible(false);
	//m_chart->createDefaultAxes(); 
	//QValueAxis *axisY = new QValueAxis;
	//m_refSeries->Get
	axisY->setMin(4); 
	axisY->setMax(20);
	axisY->setTickCount(7);
	//axisY->scal
	axisY->setTitleText("YFrequencies");

	//QValueAxis * axisX = new QValueAxis;
	//QValueAxis * axisY = new QValueAxis; 
	axisX->setMax(20);
	axisX->setTickCount(10);
	axisX->setTitleText("XGreyThreshold");
	
	m_chart->addAxis(axisX, Qt::AlignBottom);
	
	//axisY->setTitleText("Y");
	m_chart->addAxis(axisY, Qt::AlignLeft); 
	

	/*axisX->setMax(0); 
	axisX->setMin(10);*/
	//axisX->setTitleText("X"); 
	//series->attachAxis(axisX);
	m_refSeries->attachAxis(axisX);
	m_refSeries->attachAxis(axisY);

	s2->attachAxis(axisX);
	s2->attachAxis(axisY);
	series->attachAxis(axisX);
	series->attachAxis(axisY);

	//m_chart->addAxis(axisY, Qt::AlignLeft);
	/*m_chart->addAxis(axisY, Qt::AlignTop);
	m_chart->axisX(series)->setRange(0, 100);*/

	//m_chart->addAxis(axisY, Qt::AlignLeft); 
	//m_chart->setTitle("line chart example"); 
	
}


void AdaptiveThreshold::prepareDataSeries(QXYSeries *aSeries, const std::vector<double> &x_vals, const std::vector<double> &y_vals)
{
	if (!aSeries) return; 
	if (!(x_vals.size() == y_vals.size()))
		return;
	size_t lenght = x_vals.size();

	for (size_t i = 0; i < lenght; ++i) {
		double tmp_x = 0; double tmp_y = 0; 
		tmp_x = x_vals[i];
		tmp_y = y_vals[i];
		*aSeries << QPointF(tmp_x, tmp_y); 

	}

}


void AdaptiveThreshold::createSampleSeries()
{
	QScatterSeries *mySeries = new QScatterSeries;
	std::vector<double> vec_x = { 4,6,8 }; 
	std::vector<double> vec_y = { 7, 11, 15 };
	this->prepareDataSeries(mySeries, vec_x, vec_y);
	mySeries->setName("ATest");
	mySeries->setColor(QColor(0, 0, 255)); 
	m_chart->addSeries(mySeries);
	mySeries->attachAxis(axisX);
	mySeries->attachAxis(axisY);
	m_chart->update();
	m_chartView->update(); 
}

void AdaptiveThreshold::buttonUpdateClicked()
{
	DEBUG_LOG("Button update is clicked"); 
	this->m_chart->setTitle("Grey value distribution"); 
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

void AdaptiveThreshold::buttonLoadDataClicked()
{
	DEBUG_LOG("Button update is clicked");
	QString fName = QFileDialog::getOpenFileName(this, ("Open File"),
		"/home",
		("Files (*.csv *.txt)"));
	DEBUG_LOG(fName); 
	if (!m_seriesLoader.loadCSV(fName)) {
		return;
	}

	

}

