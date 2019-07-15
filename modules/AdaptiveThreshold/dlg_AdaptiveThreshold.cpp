#include "dlg_AdaptiveThreshold.h"

#include <algorithm>
#include <qsharedpointer.h>
#include <QtCharts/qlineseries.h>
#include "iAConsole.h"
#include <QColor>
#include <QPoint>
#include <QFileDialog>
//#include "QtCharts/L"
//#include <QTChart>



struct axisParams {
	double xmin; 
	double xmax; 
	double ymin;
	double ymax; 
	axisMode mode; 

};


AdaptiveThreshold::AdaptiveThreshold(QWidget * parent/* = 0,*/, Qt::WindowFlags f/* f = 0*/):QDialog(parent, f){
	setupUi(this);
	try {
		 
		m_chart = new QtCharts::QChart;
		m_chartView = new QtCharts::QChartView(m_chart);
		m_chartView->setRubberBand(QChartView::RectangleRubberBand);
		axisX = new QValueAxis; 
		axisY = new QValueAxis; 
		m_refSeries = new QLineSeries();
		series_vec.reserve(maxSeriesNumbers); 
		this->ed_xMIn->setText("");
		this->ed_xMax->setText("");
		this->ed_YMax->setText("");
		this->ed_Ymin->setText(""); 

		Qt::WindowFlags flags = this->windowFlags();
		this->setWindowFlags(flags | Qt::Tool); 

		for (auto series: series_vec) {
			series = new QLineSeries(); 
		}

		setupUIActions();

		this->mainLayout->addWidget(m_chartView);
	}
	catch (std::bad_alloc &ba) {
		throw; 
	}
}

void AdaptiveThreshold::setupUIActions()
{
	connect(this->btn_update, SIGNAL(clicked()), this, SLOT(buttonUpdateClicked()));
	connect(this->btn_loadData, SIGNAL(clicked()), this, SLOT(buttonLoadDataClicked()));
	connect(this->btn_TestData, SIGNAL(clicked()), this, SLOT(createSampleSeries()));
	connect(this->btn_clearChart, SIGNAL(clicked()), this, SLOT(clear()));
	connect(this->btn_resetGraph, SIGNAL(clicked()), this, SLOT(resetGraphToDefault()));
	connect(this->btn_myAction, SIGNAL(clicked()), this, SLOT(myAction()));
	connect(this->btn_movingAverage, SIGNAL(clicked()), this, SLOT(visualizeMovingAverage()));
	connect(this->btn_loadHistData, SIGNAL(clicked()), this, SLOT(buttonLoadHistDataClicked())); 
	connect(this->btn_clear, SIGNAL(clicked()), this, SLOT(clearEditField()));

}

AdaptiveThreshold::~AdaptiveThreshold()
{
	delete m_chart;
	delete m_chartView; 

	for (auto p:series_vec) {
		if (p)
			delete p;
	}
	
}



void AdaptiveThreshold::initChart()
{
	
	/*m_chart->addSeries(s2);
	m_chart->legend()->markers(s2)[0]->setVisible(false);*/
	initAxes(0, 20, 2, 20, true); 
}




void AdaptiveThreshold::generateSampleData(bool addserries)
{
	m_refSeries->append(0, 6);
	m_refSeries->append(2, 4);
	m_refSeries->append(3, 8);
	m_refSeries->append(7, 4);
	m_refSeries->append(10, 5);
	*m_refSeries << QPointF(2, 2) << QPointF(3, 3) << QPointF(20, 0);
	QColor color = QColor(255, 0, 0);
	m_refSeries->setColor(color);
	m_refSeries->setName("TestSeries");
	
	QScatterSeries * series = new QScatterSeries();
	series->append(4, 8);
	series->append(2, 20);
	series->setName("AName");
	series->setColor(QColor(0, 255, 0));
	
	if (addserries)
	{
		m_chart->addSeries(m_refSeries);
		m_chart->addSeries(series);
	}
	
	QLineSeries * s2 = new QLineSeries();
	s2->append(8, 0);
	s2->append(8, 8);
}

void AdaptiveThreshold::determineMinMax(const std::vector<double> &xVal, const std::vector<double> &yVal)
{
	if ((xVal.size() == 0) ||(yVal.size() == 0))
		return; 

	m_xMinRef = *std::min_element(std::begin(xVal), std::end(xVal));
	m_xMaxRef = *std::max_element(std::begin(xVal), std::end(xVal));
	m_yMinRef = *std::min_element(std::begin(yVal), std::end(yVal));
	m_yMaxRef = *std::min_element(std::begin(yVal), std::end(yVal));
}

void AdaptiveThreshold::setOutputText(const QString& Text)
{
	this->textEdit->append(Text + "\n"); 
}

void AdaptiveThreshold::setHistData (QSharedPointer<iAPlotData> &data)
{
	if (!data) 
		throw std::invalid_argument("data empty");

	m_thresCalculator.setData(data); 
}

void AdaptiveThreshold::resetGraphToDefault()
{
	DEBUG_LOG("reset to default"); 
	//DEBUG_LOG(QString("Default %1 %2 %3 %4").arg(minXDefault).arg(maxXDefault).arg(minYDefault).arg(maxYDefault)); 
	//m_xMinRef /*= xMIn;*/
	//m_xMaxRef /*= xMax;*/
	//m_yMinRef /*= yMin;*/
	//m_yMaxRef /*= yMax;*/
	this->initAxes(m_xMinRef/*minXDefault*/, m_xMaxRef, m_yMinRef, m_yMaxRef, false);
	this->ed_xMIn->setText(QString("%1").arg(m_xMinRef));
	this->ed_xMax->setText(QString("%1").arg(m_xMaxRef));
	this->ed_Ymin->setText(QString("%1").arg(m_yMinRef));
	this->ed_YMax->setText(QString("%1").arg(m_yMaxRef));

	m_chart->update();
	m_chartView->update(); 
}

void AdaptiveThreshold::visualizeMovingAverage()
{
	uint averageCount = this->spinBox_average->text().toUInt();
	DEBUG_LOG("Moving Average"); 
	DEBUG_LOG(QString("Freq size%1").arg(m_frequencies.size())); 
	if (m_movingFrequencies.size() > 0)
		m_movingFrequencies.clear();

	QString text = QString("Moving average %1").arg(averageCount);
	m_thresCalculator.calculateAverage(m_frequencies, m_movingFrequencies, averageCount);	
	m_thresCalculator.testPeakDetect();
	QLineSeries *newSeries = new QLineSeries;
	this->prepareDataSeries(newSeries, m_greyThresholds, m_movingFrequencies, false);
	
}

void AdaptiveThreshold::myAction()
{
	m_thresCalculator.doubleTestSum();
}

void AdaptiveThreshold::setDefaultMinMax(double xMIn, double xMax, double yMin, double yMax)
{
	m_xMinRef = xMIn;
	m_xMaxRef = xMax;
	m_yMinRef = yMin;
	m_yMaxRef = yMax; 
}

void AdaptiveThreshold::initAxes(double xmin, double xmax, double ymin, double yMax, bool setDefaultAxis)
{
	QString titleX = "xGreyThreshold";
	QString titleY = "YFrequencies"; 

	if (setDefaultAxis) {
		this->resetGraphToDefault();
		return;
	}

	prepareAxis(axisX, titleX, xmin, xmax, m_defautTickCountsX, axisMode::x);
	prepareAxis(axisY, titleY, ymin, yMax, m_defaultTickCountsY, axisMode::y); 
}

void AdaptiveThreshold::prepareAxis(QValueAxis *axis, const QString &title, double min, double max, uint ticks, axisMode mode)
{
	axis->setMin(min);
	axis->setMax(max);
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

void AdaptiveThreshold::clear()
{
	m_chart->removeAllSeries(); 
	m_chart->update(); 
	m_chartView->update(); 
}

void AdaptiveThreshold::prepareDataSeries(QXYSeries *aSeries, const std::vector<double> &x_vals, const std::vector<double> &y_vals, bool updateCoords)
{
	if (!aSeries) {
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
		DEBUG_LOG("size different");
		return;
	}

	determineMinMax(x_vals, y_vals); 
	//setDefaultMinMax(m_xMinRef, 20000, 0, 25000); 


	DEBUG_LOG(QString("xmin xmax ymin ymax %! %2 %3 %4").arg(m_xMinRef).arg(m_yMaxRef).
		arg(m_yMinRef).arg(m_yMaxRef)); 

	
	//this->initAxes(m_xMinRef, m_xMaxRef, m_yMinRef, m_yMinRef, false);
	if (updateCoords)
	{
		this->resetGraphToDefault(); 
	}
	

	size_t lenght = x_vals.size();

	for (size_t i = 0; i < lenght; ++i) {
		double tmp_x = 0; double tmp_y = 0; 
		tmp_x = x_vals[i];
		tmp_y = y_vals[i];
		*aSeries << QPointF(tmp_x, tmp_y); 
	}

	QString text = QString("Moving average %1").arg(5);
	
	//aSeries->setPointLabelsVisible(true);
	this->addSeries(aSeries); 
	aSeries->setObjectName(text);
	this->m_chart->update();
	this->m_chartView->update(); 

}


void AdaptiveThreshold::addSeries(QXYSeries *aSeries)
{
	if (!aSeries) 
		return; 
	if (!axisX | !axisY)
		return; 

	this->m_chart->addSeries(aSeries);
	aSeries->attachAxis(axisX);
	aSeries->attachAxis(axisY); 

	m_chart->update();
	m_chartView->update(); 

}

QT_CHARTS_NAMESPACE::QXYSeries * AdaptiveThreshold::createDataSeries(const std::vector<double> &xvals, const std::vector<double> &y_vals, plotMode mode)
{
	QXYSeries *series = nullptr; 
	switch (mode)	
	{
	case plotMode::lines:
			series = new QScatterSeries;
			break;

	case plotMode::scatter:
		series = new QLineSeries; 
		break; 
	default:
		break;
	}

	return series; 
}

void AdaptiveThreshold::createSampleSeries()
{
	QScatterSeries *mySeries = new QScatterSeries;
	std::vector<double> vec_x = { 4,6,8 }; 
	std::vector<double> vec_y = { 7, 11, 15 };
	this->prepareDataSeries(mySeries, vec_x, vec_y, false);
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
	ymax_val = yMax.toDouble();

	axisX->setRange(xmin_val, xmax_val);
	axisY->setRange(ymin_val, ymax_val);
	m_chart->update(); 
	this->m_chartView->update();
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

	this->m_greyThresholds = m_seriesLoader.getGreyValues(); 
	this->m_frequencies = m_seriesLoader.getFrequencies(); 

	prepareDataSeries(m_refSeries,m_greyThresholds, m_frequencies, true); 

	

}

void AdaptiveThreshold::buttonLoadHistDataClicked()
{
	this->textEdit->append("Loading histogram data\n");
	m_thresCalculator.retrieveHistData(); 
	this->prepareDataSeries(m_refSeries, m_thresCalculator.getThresBins(), 
		m_thresCalculator.getFreqValsY(), true); 
}

