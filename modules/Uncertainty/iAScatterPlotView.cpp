/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#include "iAScatterPlotView.h"

#include "iAColors.h"
#include "iAConsole.h"
#include "iALookupTable.h"
#include "iAPerformanceHelper.h"
#include "iAToolsVTK.h"
#include "iAScatterPlot.h"
#include "iAScatterPlotSelectionHandler.h"
#include "iASPLOMData.h"

#include <QGLWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QToolButton>
#include <QVariant>
#include <QVBoxLayout>

#include <vtkImageData.h>
#include <vtkLookupTable.h>

class iAScatterPlotStandaloneHandler : public iAScatterPlotSelectionHandler
{
public:
	virtual QVector<unsigned int> & getSelection() {
		return m_selection;
	}
	virtual const QList<int> & getHighlightedPoints() const {
		return m_highlight;
	}
	virtual int getVisibleParametersCount() const {
		return 2;
	}
	virtual double getAnimIn() const {
		return 1.0;
	}
	virtual double getAnimOut() const {
		return 0.0;
	}
private:
	QList<int> m_highlight;
	QVector<unsigned int> m_selection;
};

iAScatterPlotView::iAScatterPlotView():
	m_scatterPlotHandler(new iAScatterPlotStandaloneHandler())
{
	setLayout(new QVBoxLayout());
	layout()->setSpacing(0);


	m_settings = new QWidget();
	m_settings->setLayout(new QVBoxLayout);
	m_settings->layout()->setSpacing(0);
	m_settings->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);

	auto datasetChoiceContainer = new QWidget();
	datasetChoiceContainer->setLayout(new QVBoxLayout());
	m_xAxisChooser = new QWidget();
	m_xAxisChooser->setLayout(new QHBoxLayout());
	m_xAxisChooser->layout()->setSpacing(0);
	m_xAxisChooser->layout()->addWidget(new QLabel("X Axis"));
	m_yAxisChooser = new QWidget();
	m_yAxisChooser->setLayout(new QHBoxLayout());
	m_yAxisChooser->layout()->setSpacing(0);
	m_yAxisChooser->layout()->addWidget(new QLabel("Y Axis"));
	datasetChoiceContainer->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
	datasetChoiceContainer->layout()->addWidget(m_xAxisChooser);
	datasetChoiceContainer->layout()->addWidget(m_yAxisChooser);
	m_settings->layout()->addWidget(datasetChoiceContainer);
	layout()->addWidget(m_settings);
}


class ScatterPlotWidget : public QGLWidget
{
public:
	const int PaddingLeft   = 45;
	const int PaddingTop    = 5;
	const int PaddingRight  = 5;
	const int PaddingBottom = 45;
	ScatterPlotWidget():
		m_scatterplot(nullptr)
	{
		setMouseTracking(true);
		setFocusPolicy(Qt::StrongFocus);
	}
	void setScatterPlot(iAScatterPlot* scatterplot)
	{
		m_scatterplot = scatterplot;
	}
	virtual void paintEvent(QPaintEvent * event)
	{
		QPainter painter(this);
		painter.setRenderHint(QPainter::Antialiasing);
		painter.setRenderHint(QPainter::HighQualityAntialiasing);
		painter.beginNativePainting();
		glClearColor(1.0, 1.0, 1.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT);
		painter.endNativePainting();
		m_scatterplot->paintOnParent(painter);

		// print axes labels:
		painter.save();
		QList<double> ticksX, ticksY; QList<QString> textX, textY;
		m_scatterplot->printTicksInfo(&ticksX, &ticksY, &textX, &textY);
		painter.setPen(m_scatterplot->settings.tickLabelColor);
		QPoint tOfs(45,45);
		long tSpc = 5;
		for (long i = 0; i < ticksY.size(); ++i)
		{
			double t = ticksY[i]; QString text = textY[i];
			painter.drawText(QRectF(0, t - tOfs.y(), tOfs.x() - tSpc, 2 * tOfs.y()), Qt::AlignRight | Qt::AlignVCenter, text);
		}
		painter.rotate(-90);
		for (long i = 0; i < ticksX.size(); ++i)
		{
			double t = ticksX[i]; QString text = textX[i];
			painter.drawText(QRectF(-tOfs.y() + tSpc, t - tOfs.x(), tOfs.y() - tSpc, 2 * tOfs.x()), Qt::AlignLeft | Qt::AlignVCenter, text);
		}
		painter.restore();
	}
	virtual void resizeEvent(QResizeEvent* event)
	{
		QRect size(geometry());
		size.moveTop(PaddingTop);
		size.moveLeft(0);
		size.adjust(PaddingLeft, 0, -PaddingRight, -PaddingBottom);
		m_scatterplot->setRect(size);
	}
	virtual void wheelEvent(QWheelEvent * event)
	{
		m_scatterplot->SPLOMWheelEvent(event);
	}
	virtual void mousePressEvent(QMouseEvent * event)
	{
		m_scatterplot->SPLOMMousePressEvent(event);
	}

	virtual void mouseReleaseEvent(QMouseEvent * event)
	{
		m_scatterplot->SPLOMMouseReleaseEvent(event);
	}

	virtual void mouseMoveEvent(QMouseEvent * event)
	{
		m_scatterplot->SPLOMMouseMoveEvent(event);
	}
	virtual void keyPressEvent(QKeyEvent * event)
	{
		switch (event->key())
		{
		case Qt::Key_R: //if R is pressed, reset all the applied transformation as offset and scaling
			m_scatterplot->setTransform(1.0, QPointF(0.0f, 0.0f));
			break;
		}
	}
	virtual void initializeGL()
	{
		//qglClearColor(QColor(255, 255, 255));
	}

private:
	iAScatterPlot* m_scatterplot;
};

void iAScatterPlotView::AddPlot(vtkImagePointer imgX, vtkImagePointer imgY, QString const & captionX, QString const & captionY)
{
	// setup data object:
	int * dim = imgX->GetDimensions();
	m_voxelCount = static_cast<size_t>(dim[0]) * dim[1] * dim[2];
	double* bufX = static_cast<double*>(imgX->GetScalarPointer());
	double* bufY = static_cast<double*>(imgY->GetScalarPointer());
	splomData = QSharedPointer<iASPLOMData>(new iASPLOMData());
	splomData->paramNames().push_back(captionX);
	splomData->paramNames().push_back(captionY);
	QList<double> values0;
	splomData->data().push_back(values0);
	QList<double> values1;
	splomData->data().push_back(values1);
	for (size_t i = 0; i < m_voxelCount; ++i)
	{
		splomData->data()[0].push_back(bufX[i]);
		splomData->data()[1].push_back(bufY[i]);
	}

	// setup scatterplot:
	ScatterPlotWidget *scatterPlotWidget = new ScatterPlotWidget();
	scatterplot = new iAScatterPlot(m_scatterPlotHandler.data(), scatterPlotWidget);
	scatterPlotWidget->setScatterPlot(scatterplot);
	auto lut = vtkSmartPointer<vtkLookupTable>::New();
	double lutRange[2] = { 0, 1 };
	lut->SetRange(lutRange);
	lut->Build();
	vtkIdType lutColCnt = lut->GetNumberOfTableValues();
	double alpha = 0.5;
	for (vtkIdType i = 0; i < lutColCnt; i++)
	{
		double rgba[4]; lut->GetTableValue(i, rgba);
		rgba[3] = alpha;
		lut->SetTableValue(i, rgba);
	}
	lut->Build();
	QSharedPointer<iALookupTable> lookupTable(new iALookupTable(lut.GetPointer()));
	scatterplot->setData(0, 1, splomData);
	scatterplot->setLookupTable(lookupTable, captionX);
	layout()->addWidget(scatterPlotWidget);

	connect(scatterplot, SIGNAL(selectionModified()), this, SLOT(selectionUpdated()));
}


void iAScatterPlotView::SetDatasets(QSharedPointer<iAUncertaintyImages> imgs)
{
	for (auto widget : m_xAxisChooser->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly))
	{
		delete widget;
	}
	for (auto widget : m_yAxisChooser->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly))
	{
		delete widget;
	}
	m_imgs = imgs;
	m_xAxisChoice = iAUncertaintyImages::LabelDistributionEntropy;
	m_yAxisChoice = iAUncertaintyImages::AvgAlgorithmEntropyProbSum;
	for (int i = 0; i < iAUncertaintyImages::SourceCount; ++i)
	{
		QToolButton* xButton = new QToolButton(); xButton->setText(imgs->GetSourceName(i));
		QToolButton* yButton = new QToolButton(); yButton->setText(imgs->GetSourceName(i));
		xButton->setProperty("imgId", i);
		yButton->setProperty("imgId", i);
		xButton->setAutoExclusive(true);
		xButton->setCheckable(true);
		yButton->setAutoExclusive(true);
		yButton->setCheckable(true);
		if (i == m_xAxisChoice)
		{
			xButton->setChecked(true);
		}
		if (i == m_yAxisChoice)
		{
			yButton->setChecked(true);
		}
		connect(xButton, SIGNAL(clicked()), this, SLOT(XAxisChoice()));
		connect(yButton, SIGNAL(clicked()), this, SLOT(YAxisChoice()));
		m_xAxisChooser->layout()->addWidget(xButton);
		m_yAxisChooser->layout()->addWidget(yButton);
	}
	m_selectionImg = AllocateImage(imgs->GetEntropy(m_xAxisChoice));
	AddPlot(imgs->GetEntropy(m_xAxisChoice), imgs->GetEntropy(m_yAxisChoice),
		imgs->GetSourceName(m_xAxisChoice), imgs->GetSourceName(m_yAxisChoice));
}


void iAScatterPlotView::XAxisChoice()
{
	int imgId = qobject_cast<QToolButton*>(sender())->property("imgId").toInt();
	if (imgId == m_xAxisChoice)
		return;
	m_xAxisChoice = imgId;
	AddPlot(m_imgs->GetEntropy(m_xAxisChoice), m_imgs->GetEntropy(m_yAxisChoice),
		m_imgs->GetSourceName(m_xAxisChoice), m_imgs->GetSourceName(m_yAxisChoice));
}


void iAScatterPlotView::YAxisChoice()
{
	int imgId = qobject_cast<QToolButton*>(sender())->property("imgId").toInt();
	if (imgId == m_yAxisChoice)
		return;
	m_yAxisChoice = imgId;
	AddPlot(m_imgs->GetEntropy(m_xAxisChoice), m_imgs->GetEntropy(m_yAxisChoice),
		m_imgs->GetSourceName(m_xAxisChoice), m_imgs->GetSourceName(m_yAxisChoice));
}

void iAScatterPlotView::SelectionUpdated()
{
	QVector<unsigned int> m_points = m_scatterPlotHandler->getSelection();
}

vtkImagePointer iAScatterPlotView::GetSelectionImage()
{
	return m_selectionImg;
}

void iAScatterPlotView::ToggleSettings()
{
	m_settings->setVisible(!m_settings->isVisible());
}
