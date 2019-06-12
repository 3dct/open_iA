#include "iACutstomChartView.h"
#include "iAConsole.h"



iACutstomChartView::iACutstomChartView(QWidget *parent)
{
	try
	{
		m_chart = new QChart;
		QChartView(m_chart, parent);
		m_axisX = new QValueAxis;
		m_axisY = new QValueAxis;
		m_chart->createDefaultAxes();
	}
	catch (std::bad_alloc) {
		throw; 
	}
}


iACutstomChartView::~iACutstomChartView()
{
	if (m_axisX) delete m_axisX; 
	if (m_axisY) delete m_axisY; 
	if (m_chart) delete m_chart; 
}

void iACutstomChartView::prepareAxisX(const QString &title, double min,double max, uint tick_count)
{
	this->prepareAxis(m_axisX, title, min, max, tick_count, axisMode::x); 
}

void iACutstomChartView::prepareAxisY(const QString &title, double min, double max, uint tick_count)
{
	this->prepareAxis(m_axisY, title, min, max, tick_count, axisMode::y); 
}

void iACutstomChartView::addSeries(QAbstractSeries *aSerie, const QString *title, bool disableSymbol)
{
	if (!aSerie) return; 
	m_chart->addSeries(aSerie);
	if (title && (!title->isEmpty() || !title->isNull()));
	aSerie->setObjectName(*title); 
	aSerie->attachAxis(m_axisX);
	aSerie->attachAxis(m_axisX);

	if (disableSymbol)
		m_chart->legend()->markers(aSerie)[0]->setVisible(false); 

	m_chart->update();
	this->update(); 

}

void iACutstomChartView::setAxisLimits(double xmin, double xMax, double yMin, double yMax)
{
	setAxisMinMax(m_axisX, xmin, xMax, false);
	setAxisMinMax(m_axisY, yMin, yMax, false); 
	m_chart->update();
	this->update(); 
}

void iACutstomChartView::setAxisMinMax(QValueAxis *axis, double min, double max, bool updateChart)
{
	if (axis)
	{
		axis->setMin(min); 
		axis->setMax(max); 
		
		if (updateChart)
		{
			DEBUG_LOG("update chart"); 
			m_chart->update();
			this->update();
		}
	}
}

void iACutstomChartView::prepareAxis(QValueAxis *axis, const QString &title, double min, double max, uint ticks, chartV::axisMode mode)
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
