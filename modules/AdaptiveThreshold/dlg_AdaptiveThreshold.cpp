#include "dlg_AdaptiveThreshold.h"
#include <algorithm>
#include <qsharedpointer.h>
#include <QtCharts/qlineseries.h>
#include "iAConsole.h"
#include <QColor>
#include <QPoint>
#include <QFileDialog>
#include "ChartVisHelper.h"
#include "ThresholdDefinitions.h"



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
		flags |= Qt::Tool;
		this->setWindowFlags(flags);

		for (auto series: series_vec) {
			series = new QLineSeries(); 
		}

		setupUIActions();
		this->mainLayout->addWidget(m_chartView);
	}
	catch (std::bad_alloc &ba) {
		DEBUG_LOG("Not enough memory to create objects"); 
	}
}

void AdaptiveThreshold::setupUIActions()
{
	connect(this->btn_update, SIGNAL(clicked()), this, SLOT(UpdateChartClicked()));
	connect(this->btn_loadData, SIGNAL(clicked()), this, SLOT(buttonLoadDataClicked()));
	connect(this->btn_TestData, SIGNAL(clicked()), this, SLOT(createSampleSeries()));
	connect(this->btn_clearChart, SIGNAL(clicked()), this, SLOT(clear()));
	connect(this->btn_resetGraph, SIGNAL(clicked()), this, SLOT(resetGraphToDefault()));
	connect(this->btn_myAction, SIGNAL(clicked()), this, SLOT(myAction()));
	connect(this->btn_movingAverage, SIGNAL(clicked()), this, SLOT(calculateMovingAverage()));
	connect(this->btn_loadHistData, SIGNAL(clicked()), this, SLOT(buttonLoadHistDataClicked())); 
	connect(this->btn_clear, SIGNAL(clicked()), this, SLOT(clearEditField()));
	connect(this->btn_aTestAction, SIGNAL(clicked()), this, SLOT(aTestAction_2()));
	connect(this->btn_selectRange, SIGNAL(clicked()), this, SLOT(buttonSelectRangesClicked()));
	connect(this->btn_MinMax, SIGNAL(clicked()), this, SLOT(buttonMinMaxClicked())); 
	connect(this->btn_redraw, SIGNAL(clicked()), this, SLOT(redrawPlots())); 

}

AdaptiveThreshold::~AdaptiveThreshold()
{
			
}



void AdaptiveThreshold::initChart()
{
	
	/*m_chart->addSeries(s2);
	m_chart->legend()->markers(s2)[0]->setVisible(false);*/
	initAxes(0, 20, 2, 20, false); 
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
	if ((xVal.size() == 0) || (yVal.size() == 0)) {
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

void AdaptiveThreshold::setOutputText(const QString& Text)
{
	this->textEdit->append(Text + "\n"); 
}

void AdaptiveThreshold::setInputData(const std::vector<double> &thres_binInX,const std::vector<double> &freqValsInY)
{
	m_greyThresholds = thres_binInX; 
	m_frequencies = freqValsInY; 
}

void AdaptiveThreshold::addAllSeries(std::vector<QXYSeries*> allSeries, bool disableMarker)
{
	for (QXYSeries* el : allSeries) {
		this->addSeries(el, disableMarker);
	}
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
	this->initAxes(m_xMinRef/*minXDefault*/, m_xMaxRef, m_yMinRef, m_yMaxRef, false);
	this->ed_xMIn->setText(QString("%1").arg(m_xMinRef));
	this->ed_xMax->setText(QString("%1").arg(m_xMaxRef));
	this->ed_Ymin->setText(QString("%1").arg(m_yMinRef));
	this->ed_YMax->setText(QString("%1").arg(m_yMaxRef));

	m_chart->update();
	m_chartView->update(); 
}

void AdaptiveThreshold::calculateMovingAverage()
{
	DEBUG_LOG("Moving Average");
	uint averageCount = this->spinBox_average->text().toUInt();
	if (averageCount <= 2) return; 	

	DEBUG_LOG(QString("Freq size%1").arg(m_frequencies.size())); 

	//reset the frequency
	if (m_movingFrequencies.size() > 0)
		m_movingFrequencies.clear();

	QString text = QString("Moving average %1").arg(averageCount);
	m_thresCalculator.calculateMovingAverage(m_frequencies, m_movingFrequencies, averageCount);	
	this->cmb_BoxMovFreq->addItem(text); 
	
	QLineSeries *newSeries = new QLineSeries;
	this->prepareDataSeries(newSeries, m_greyThresholds, m_movingFrequencies, &text, false,false);
	
}

void AdaptiveThreshold::buttonSelectRangesClicked()
{
	double x_min = 0;
	double x_max = 0;
	assignValuesFromField(x_min, x_max);
		
	try
	{
		threshold_defs::ParametersRanges paramRanges;

		if (m_movingFrequencies.empty())
		{
			this->textEdit->append("moving average not yet created, create average sequence before");
			return;
		}

		//input grauwerte und moving freqs, output is paramRanges
		m_thresCalculator.specifyRange(m_greyThresholds, m_movingFrequencies/*m_frequencies*/, paramRanges, x_min, x_max);
		auto thrPeaks = m_thresCalculator.calcMinMax(paramRanges);
		this->ed_PeakThrMaxX->setText(QString("%1").arg(thrPeaks.maxX));
		this->ed_PeakFregMaxY->setText(QString("%1").arg(thrPeaks.maxThresholdY));
		this->ed_minPeakThrX->setText(QString("%1").arg(thrPeaks.minX));
		this->ed_MinPeakFreqrY->setText(QString("%1").arg(thrPeaks.minThresholdY));
		createVisualisation(paramRanges, thrPeaks);
		return;

	}catch (std::bad_alloc& ba){
			this->textEdit->append("not enough memory available"); 

	}catch (std::invalid_argument& ia)
	{
		QString outputMessg = ia.what();
		this->textEdit->append(outputMessg);
	}
}

void AdaptiveThreshold::createVisualisation(threshold_defs::ParametersRanges paramRanges, threshold_defs::ThresMinMax thrPeaks)
{
	try {
		QLineSeries* rangedSeries = nullptr;
		rangedSeries = ChartVisHelper::createLineSeries(paramRanges);
		QPointF p1 = ChartVisHelper::createPoint(thrPeaks.maxX, thrPeaks.maxThresholdY);
		QPointF p2 = ChartVisHelper::createPoint(thrPeaks.minX, thrPeaks.minThresholdY);
		
		if (!rangedSeries)
		{
			DEBUG_LOG("Range series not created");
			return;
		}

		m_colCounter += 20; 
		if (m_colCounter > 255) {
			m_colCounter = 0; 
		}

		QColor basis = QColor(0, 0, m_colCounter);		
		std::vector<QXYSeries*> data; 
		auto SeriesTwoPoints = ChartVisHelper::createLineSeries(p1, p1, LineVisOption::vertically);
		auto SeriesTwoPointsb = ChartVisHelper::createLineSeries(p1, p1, LineVisOption::horizontally);
		SeriesTwoPointsb->setColor(basis);
		SeriesTwoPoints->setColor(basis);
		
		data.push_back(SeriesTwoPoints);
		data.push_back(SeriesTwoPointsb);

		auto series_p2 = ChartVisHelper::createLineSeries(p2, p2, LineVisOption::horizontally);
		auto series_p2_b = ChartVisHelper::createLineSeries(p2, p2, LineVisOption::vertically);

		series_p2_b->setColor(QColor(basis));
		series_p2->setColor(QColor(basis));
		QColor color = QColor(255, 0, 0);
		rangedSeries->setColor(color);
		data.push_back(rangedSeries);
		data.push_back(series_p2);
		data.push_back(series_p2); 	

		this->addSeries(series_p2, true);
		this->addAllSeries(data, true);
		m_chartView->update();
				
		this->writeText(thrPeaks.toString());
	}
	catch (std::invalid_argument iae) {
		throw; 
	
	}
	catch (std::bad_alloc& ba) {
		throw; 
	}
}

void AdaptiveThreshold::assignValuesFromField(double& x_min, double& x_max)
{
	bool* x_OK = new bool; bool* x2_OK = new bool;

	x_min = this->ed_minRange->text().toDouble(x_OK);
	x_max = this->ed_maxRange->text().toDouble(x2_OK);

	if ((!x_OK) || (!x2_OK)) {
		this->textEdit->setText("invalid input");
		return;

	}
	else if ((x_min < 0) || (x_max <= 0)) {
		this->textEdit->setText("please again check parameters");
		return;
	}

	delete x_OK;
	delete x2_OK;
}

void AdaptiveThreshold::redrawPlots()
{
	double xmin = this->ed_xMIn->text().toDouble();
	double xmax = this->ed_xMax->text().toDouble(); 
	double ymin = this->ed_Ymin->text().toDouble(); 
	double ymax = this->ed_YMax->text().toDouble(); 

	//redraw histogram
	//redraw moving averages

	if (m_chart) {
		if (m_chartView) {
			DEBUG_LOG("Delete viewer");
			auto sList = m_chart->series();

			QListIterator<QAbstractSeries*> iter(sList);
			while (iter.hasNext()) {
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

	if (this->m_refSeries) {
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

void AdaptiveThreshold::buttonMinMaxClicked()
{

}

void AdaptiveThreshold::myAction()
{
	m_thresCalculator.doubleTestSum();
}

void AdaptiveThreshold::aTestAction_2()
{
	DEBUG_LOG("TestAction_2 is fired"); 
	//double val = 
	double val = ed_testText->text().toDouble(); 
	//	std::vector<double> data{ 6.1, 8.0, 9.0, 14.1, 10.0,14.3, 12.1, 14.4 };
	auto res = 	m_thresCalculator.testFindIndex(val);
	QString tmp = QString("ind %1 val %2").arg(res.thrIndx).arg(res.value);

	this->textEdit->append(tmp); 
}

void AdaptiveThreshold::aTestAction()
{
	DEBUG_LOG("ATestAction is controlled");

	std::vector<double> v_inRange = { 2.005, 1,0.1,2.0001, 0, 0, 8 , 4, 10, 7, 12 };
	//								1	2	3		4	5	6	7	8
	std::vector<double> v_elements = { 0, 100, 200, 300, 400, 500, 600,700,800, 900, 1000 };
	threshold_defs::ParametersRanges outputRanges;
	m_thresCalculator.testSpecifyRange(v_inRange, v_elements,outputRanges);
	QScatterSeries *testSeries = ChartVisHelper::createScatterSeries(outputRanges); 
	this->addSeries(testSeries, false); 

}

void AdaptiveThreshold::AnotherAction()
{
	DEBUG_LOG("Another Action fired"); 

	std::vector<double> vec_x = { 1, 2, 3, 4,8};
	std::vector<double> vec_y = { 100, 117, 120, 110, 120 };
	QLineSeries* mySeries = ChartVisHelper::createLineSeries(vec_x, vec_y); 
	this->initAxes(0, 10, 90, 100, false);
	this->addSeries(mySeries, false); 
	QColor color = QColor(0, 255, 0); 
	mySeries->setColor(color); 
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
	
	//TODO welchen sinn hat das 
	if (setDefaultAxis) {
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

void AdaptiveThreshold::prepareDataSeries(QXYSeries *aSeries,
	const std::vector<double> &x_vals, const std::vector<double> &y_vals, 
	QString *grText, bool useDefaultValues, bool updateCoords)
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
		DEBUG_LOG("x, y size different");
		return;
	}
	if (!useDefaultValues)
		determineMinMax(x_vals, y_vals);
	else{
		this->DetermineRange(); 
	}
	
	DEBUG_LOG(QString("xmin xmax ymin ymax %1 %2 %3 %4").arg(m_xMinRef).arg(m_yMaxRef).
		arg(m_yMinRef).arg(m_yMaxRef)); 
		
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

	
	
	this->addSeries(aSeries, false); 
	if (grText) {
		QString stext = *grText;
			aSeries->setName(stext);
	}
	this->m_chart->update();
	this->m_chartView->update(); 

}


void AdaptiveThreshold::addSeries(QXYSeries* aSeries, bool disableMarker)
{
	if (!aSeries) 
		return; 
	
	this->m_chart->addSeries(aSeries);

	if (disableMarker){
		m_chart->legend()->markers(aSeries)[0]->setVisible(false);
	}

	aSeries->attachAxis(axisX);
	aSeries->attachAxis(axisY); 
	m_chart->update();
	m_chartView->update(); 

}


//rescale 
void AdaptiveThreshold::UpdateChartClicked()
{
	DEBUG_LOG("rescale graph"); 
	this->m_chart->setTitle("Grey value distribution"); 

	DetermineRange();
	
	QString xTicks, yTicks;
	xTicks = this->spin_xTicks->text();
	yTicks = this->spin_yTicks->text();

	uint xTickcount = xTicks.toUInt();
	uint yTickcount = yTicks.toUInt(); 

	if ((!yTicks.isEmpty()) && (!xTicks.isEmpty())) {
		this->setTicks(xTickcount, yTickcount, false);
	}
		
	m_chart->update(); 
	this->m_chartView->update();
}

void AdaptiveThreshold::DetermineRange()
{
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
}

void AdaptiveThreshold::buttonLoadDataClicked()
{
	QString fName = QFileDialog::getOpenFileName(this, ("Open File"),
		"/home",
		("Files (*.csv *.txt)"));
	DEBUG_LOG(fName); 
	if (!m_seriesLoader.loadCSV(fName)) {
		return;
	}

	this->m_greyThresholds = m_seriesLoader.getGreyValues(); 
	this->m_frequencies = m_seriesLoader.getFrequencies(); 
	prepareDataSeries(m_refSeries,m_greyThresholds, m_frequencies,nullptr, true, true); 
}

void AdaptiveThreshold::buttonLoadHistDataClicked()
{   //load histogram 
	//retrieve thresholds and frequencies
	//visualize it
	this->textEdit->append("Loading histogram data\n");
	QString text = "Histogram data";
	m_thresCalculator.retrieveHistData(); 
	this->setInputData(m_thresCalculator.getThresBins(), m_thresCalculator.getFreqValsY()); 
	this->prepareDataSeries(m_refSeries, m_greyThresholds, 
		m_frequencies, &text, false, true);
}

