#include "iACustomChartView.h"
#include "iAConsole.h"

#include <QtCharts>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QValueAxis>

iACustomChartView::iACustomChartView(QWidget *parent)
{
	m_chart = new QChart;
	QChartView(m_chart, parent);
	m_axisX = new QValueAxis;
	m_axisY = new QValueAxis;
	m_chart->createDefaultAxes();
}

iACustomChartView::~iACustomChartView()
{
	if (m_axisX) delete m_axisX; 
	if (m_axisY) delete m_axisY; 
	if (m_chart) delete m_chart; 
}

void iACustomChartView::prepareAxisX(const QString &title, double min,double max, uint tick_count)
{
	this->prepareAxis(m_axisX, title, min, max, tick_count, chartV::axisMode::x);
}

void iACustomChartView::prepareAxisY(const QString &title, double min, double max, uint tick_count)
{
	this->prepareAxis(m_axisY, title, min, max, tick_count, chartV::axisMode::y);
}

void iACustomChartView::addSeries(QAbstractSeries *aSerie, const QString *title, bool disableSymbol)
{
	if (!aSerie)
	{
		return;
	}
	m_chart->addSeries(aSerie);
	if (title && (!title->isEmpty() || !title->isNull()))
	{
		aSerie->setObjectName(*title);
	}

	aSerie->attachAxis(m_axisX);
	aSerie->attachAxis(m_axisX);

	if (disableSymbol)
	{
		m_chart->legend()->markers(aSerie)[0]->setVisible(false);
	}

	m_chart->update();
	this->update(); 

}

void iACustomChartView::setAxisLimits(double xmin, double xMax, double yMin, double yMax)
{
	setAxisMinMax(m_axisX, xmin, xMax, false);
	setAxisMinMax(m_axisY, yMin, yMax, false); 
	m_chart->update();
	this->update(); 
}

void iACustomChartView::setAxisMinMax(QValueAxis *axis, double min, double max, bool updateChart)
{
	if (!axis)
	{
		return;
	}
	axis->setMin(min); 
	axis->setMax(max); 
		
	if (updateChart)
	{
		DEBUG_LOG("update chart"); 
		m_chart->update();
		this->update();
	}
}

void iACustomChartView::prepareAxis(QValueAxis *axis, const QString &title, double min, double max, uint ticks, chartV::axisMode mode)
{
	axis->setMin(min);
	axis->setMax(max);
	axis->setTickCount(ticks);
	axis->setTitleText(title);

	switch (mode)
	{
	case chartV::axisMode::x:
		m_chart->addAxis(axis, Qt::AlignBottom);
		break;
	case chartV::axisMode::y:
		m_chart->addAxis(axis, Qt::AlignLeft);
		break;
	default:
		break;
	}
}

void iACustomChartView::setDiagramTitle(const QString& title)
{
	m_chart->setTitle(title);
}

void iACustomChartView::setXTite(const QString& title)
{
	this->m_axisX->setTitleText(title);
}

void iACustomChartView::setYTitle(const QString& title)
{
	m_axisY->setTitleText(title);
}
