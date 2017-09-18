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
#include "iAPerformanceHelper.h"
#include "iAToolsVTK.h"
#include "iAScatterPlot.h"
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

/*
// only relevant for heatmap:
QCPColorGradient GetGradientFromIdx(int index)
{
	const int PredefinedCount = QCPColorGradient::gpHues + 1;
	if (index < PredefinedCount)
	{
		return QCPColorGradient(static_cast<QCPColorGradient::GradientPreset>(index));
	}
	else
	{
		switch (index - PredefinedCount)
		{
			case 0:
			{
				QCPColorGradient myGrayScale;
				QMap<double, QColor> myGrayScaleMap;
				myGrayScaleMap.insert(0.0, QColor(255, 255, 255));
				myGrayScaleMap.insert(1.0, QColor(0, 0, 0));
				myGrayScale.setColorStops(myGrayScaleMap);
				return myGrayScale;
			}
			default:
			case 1:
			{
				QCPColorGradient whiteToBlue;
				QMap<double, QColor> whiteToBlueMap;
				whiteToBlueMap.insert(0.0, QColor(255, 255, 255));
				whiteToBlueMap.insert(1.0, QColor(0, 0, 255));
				whiteToBlue.setColorStops(whiteToBlueMap);
				return whiteToBlue;
			}
		}
	}
}
*/

iAScatterPlotView::iAScatterPlotView()
// only relevant for heatmap:
// m_gradient(QCPColorGradient::gpGrayscale),
/*
	:
	m_plot(new QCustomPlot()),
	m_curve(nullptr)
*/
{
	setLayout(new QVBoxLayout());
	layout()->setSpacing(0);

	/*
	//m_plot->setOpenGl(true);
	m_plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables | QCP::iMultiSelect);
	m_plot->setMultiSelectModifier(Qt::ShiftModifier);
	connect(m_plot, SIGNAL(mousePress(QMouseEvent *)), this, SLOT(ChartMousePress(QMouseEvent *)));
	layout()->addWidget(m_plot);
	*/

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
	/*
	// only relevant for heatmap
	QComboBox * colorThemeChooser = new QComboBox();
	QStringList options;
	options
		<< "gpGrayscale"
		<< "gpHot"
		<< "gpCold"
		<< "gpNight"
		<< "gpCandy"
		<< "gpGeography"
		<< "gpIon"
		<< "gpThermal"
		<< "gpPolar"
		<< "gpSpectrum"
		<< "gpJet"
		<< "gpHues"
		<< "WhiteToBlack"
		<< "WhiteToBlue";
	colorThemeChooser->addItems(options);
	connect(colorThemeChooser, SIGNAL(currentIndexChanged(int)), this, SLOT(ColorThemeChanged(int)));
	QWidget* colorThemeContainer = new QWidget();
	colorThemeContainer->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
	colorThemeContainer->setLayout(new QHBoxLayout());
	colorThemeContainer->layout()->addWidget(new QLabel("Color Theme:"));
	colorThemeContainer->layout()->addWidget(colorThemeChooser);
	layout()->addWidget(colorThemeContainer);
	*/
}


class ScatterPlotWidget : public QGLWidget
{
public:
	ScatterPlotWidget(iAScatterPlot* scatterplot) :
		m_scatterplot(scatterplot)
	{
		qglClearColor(QColor(255, 255, 255));
	}
	virtual void paintEvent(QPaintEvent * event)
	{
		QPainter painter(this);
		painter.setRenderHint(QPainter::Antialiasing);
		painter.setRenderHint(QPainter::HighQualityAntialiasing);
		painter.beginNativePainting();
		glClear(GL_COLOR_BUFFER_BIT);
		painter.endNativePainting();
		m_scatterplot->paintOnParent(painter);
	}
	virtual void resizeEvent(QResizeEvent* event)
	{
		m_scatterplot->setRect(geometry());
		m_scatterplot->setPSize(width(), height());
		update();
	}
	virtual void wheelEvent(QWheelEvent * event)
	{
		m_scatterplot->SPLOMWheelEvent(event);
		update();
	}
	virtual void mousePressEvent(QMouseEvent * event)
	{
		m_scatterplot->SPLOMMousePressEvent(event);
		update();
	}

	virtual void mouseReleaseEvent(QMouseEvent * event)
	{
		m_scatterplot->SPLOMMouseReleaseEvent(event);
		update();
	}

	virtual void mouseMoveEvent(QMouseEvent * event)
	{
		m_scatterplot->SPLOMMouseMoveEvent(event);
		update();
	}
	virtual void keyPressEvent(QKeyEvent * event)
	{
		switch (event->key())
		{
		case Qt::Key_R: //if R is pressed, reset all the applied transformation as offset and scaling
			m_scatterplot->setTransform(1.0, QPointF(0.0f, 0.0f));
			break;
		}
		update();
	}

private:
	iAScatterPlot* m_scatterplot;
};

void iAScatterPlotView::AddPlot(vtkImagePointer imgX, vtkImagePointer imgY, QString const & captionX, QString const & captionY)
{
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
	scatterplot = new iAScatterPlot();
	scatterplot->setData(0, 1, splomData);
	ScatterPlotWidget *scatterPlotWidget = new ScatterPlotWidget(scatterplot);
	/*
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
	splom->setLookupTable(lut, captionX);
	*/
	layout()->addWidget(scatterPlotWidget);
	scatterplot->setRect(scatterPlotWidget->geometry());
	scatterplot->setPSize(scatterPlotWidget->width(), scatterPlotWidget->height());

	//iAScatterPlot* scatterPlot = new iAScatterPlot(); // apparently we can't have a single scatterplot but need a matrix
	/*
	// QCUSTOMPLOT {
	QCPDataSelection selection;
	if (m_curve)
	{
		selection = m_curve->selection();
	}
	m_plot->clearPlottables();
	*/

/*
	// HEATMAP {
	const int BinCountX = 250;
	const int BinCountY = 250;
	iAPerformanceHelper heatmapCalcMeasure;
	heatmapCalcMeasure.start("Heatmap calculation");
	colorMap = new QCPColorMap(m_plot->xAxis, m_plot->yAxis);
	colorMap->data()->setSize(BinCountX, BinCountY);
	colorMap->data()->setRange(QCPRange(0, 1), QCPRange(0, 1));
	colorMap->data()->fill(0);

	double* bufX = static_cast<double*>(imgX->GetScalarPointer());
	double* bufY = static_cast<double*>(imgY->GetScalarPointer());
	FOR_VTKIMG_PIXELS_IDX(imgX, idx)
	{
		int x, y;
		colorMap->data()->coordToCell(bufX[idx], bufY[idx], &x, &y);
		colorMap->data()->setCell(x, y, colorMap->data()->cell(x, y)+1 );
	}

	colorScale = new QCPColorScale(m_plot);
	m_plot->plotLayout()->addElement(0, 1, colorScale); // add it to the right of the main axis rect
	colorScale->setType(QCPAxis::atRight); // scale shall be vertical bar with tick/axis labels right (actually atRight is already the default)
	colorScale->setRangeDrag(false);
	colorScale->setRangeZoom(false);
	colorMap->setColorScale(colorScale); // associate the color map with the color scale
	colorScale->axis()->setLabel("Entropy");

	colorMap->setGradient(GetGradientFromIdx(m_gradient));
	colorMap->rescaleDataRange();
	QCPMarginGroup *marginGroup = new QCPMarginGroup(m_plot);
	m_plot->axisRect()->setMarginGroup(QCP::msBottom | QCP::msTop, marginGroup);
	colorScale->setMarginGroup(QCP::msBottom | QCP::msTop, marginGroup);

	m_plot->rescaleAxes();
	heatmapCalcMeasure.stop();

	// create mapping from bins  to pixels that are contained 
	//     or still explicit uncertainty, to be able to select not only bins but sub-bins?

	// subdivide bins -> zoomable?
	// } HEATMAP
*/
	/*
	iAPerformanceHelper scatterPlotCreationTimer;
	scatterPlotCreationTimer.start("Scatterplot creation");
	QSharedPointer<QCPCurveDataContainer> data(new QCPCurveDataContainer);
	int * dim = imgX->GetDimensions();
	m_voxelCount = static_cast<size_t>(dim[0]) * dim[1] * dim[2];
	double* bufX = static_cast<double*>(imgX->GetScalarPointer());
	double* bufY = static_cast<double*>(imgY->GetScalarPointer());
	for (size_t i = 0; i < m_voxelCount; ++i)
	{
		data->add(QCPCurveData(i, bufX[i], bufY[i]));
	}
	m_curve = new QCPCurve(m_plot->xAxis, m_plot->yAxis);
	m_curve->setData(data);
	m_curve->setLineStyle(QCPCurve::lsNone);
	m_curve->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, Uncertainty::ChartColor, 2));
	m_curve->setSelectable(QCP::stMultipleDataRanges);
	m_curve->selectionDecorator()->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, Uncertainty::SelectionColor, 2));
	connect(m_curve, SIGNAL(selectionChanged(QCPDataSelection const &)), this, SLOT(SelectionChanged(QCPDataSelection const &)));

	m_plot->xAxis->setLabel(captionX);
	m_plot->yAxis->setLabel(captionY);
	m_plot->xAxis->setRange(0, 1);
	m_plot->yAxis->setRange(0, 1);

	m_curve->setSelection(selection);

	scatterPlotCreationTimer.stop();

	m_plot->replot();
	*/
	// QCUSTOMPLOT }
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

/*
void iAScatterPlotView::SelectionChanged(QCPDataSelection const & selection)
{
	double* buf = static_cast<double*>(m_selectionImg->GetScalarPointer());
	for (int v=0; v<m_voxelCount; ++v)
	{
		*buf = 0;
		buf++;
	}
	buf = static_cast<double*>(m_selectionImg->GetScalarPointer());
	for (int r = 0; r < selection.dataRangeCount(); ++r)
	{
		std::fill(buf + selection.dataRange(r).begin(), buf + selection.dataRange(r).end(), 1);
	}
	m_selectionImg->Modified();

	//StoreImage(m_selectionImg, "C:/Users/p41143/selection.mhd", true);
	emit SelectionChanged();
}
*/

vtkImagePointer iAScatterPlotView::GetSelectionImage()
{
	return m_selectionImg;
}

/*
void iAScatterPlotView::ChartMousePress(QMouseEvent *)
{
	if (QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier))
	{	// allow selection with Ctrl key
		m_plot->setSelectionRectMode(QCP::srmSelect);
	}
	else
	{	// enable dragging otherwise
		m_plot->setSelectionRectMode(QCP::srmNone);
	}
}
*/


void iAScatterPlotView::ColorThemeChanged(int index)
{
	/*
	// only relevant for heatmap
	m_gradient = index;
	colorMap->setGradient(GetGradientFromIdx(m_gradient));
	colorScale->setGradient(GetGradientFromIdx(m_gradient));
	m_plot->replot();
	*/
}


void iAScatterPlotView::ToggleSettings()
{
	m_settings->setVisible(!m_settings->isVisible());
}
