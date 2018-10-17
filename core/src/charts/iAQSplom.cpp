/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
#include "iAQSplom.h"

#include "iAChartWidget.h"
#include "iAColorTheme.h"
#include "iAConsole.h"
#include "iAHistogramData.h"
#include "iALookupTable.h"
#include "iAMathUtility.h"
#include "iALUT.h"
#include "iAPlotTypes.h"
#include "iAScatterPlot.h"
#include "iASPLOMData.h"
#include "iASPMSettings.h"

#include <vtkLookupTable.h>

#include <QAbstractTextDocumentLayout>
#include <QColorDialog>
#include <QListWidgetItem>
#include <QPropertyAnimation>
#include <QTableWidget>
#include <QWheelEvent>
#include <QtMath>
#include <QMenu>
#include <QMessageBox>
#if (VTK_MAJOR_VERSION >= 8 && defined(VTK_OPENGL2_BACKEND) )
#include <QSurfaceFormat>
#include <QPainter>
#endif

namespace
{ // apparently QFontMetric width is not returning the full width of the string - correction constant:
	const int TextPadding = 7;
	const double PointRadiusFractions = 10.0;
}

iAQSplom::Settings::Settings()
	:plotsSpacing( 7 ),
	tickLabelsOffset( 5 ),
	maxRectExtraOffset( 20 ),
	tickOffsets( 45, 45 ),
	backgroundColor( Qt::white ),
	maximizedLinked( false ),
	popupBorderColor( QColor( 180, 180, 180, 220 )),
	popupFillColor(QColor( 250, 250, 250, 200 )),
	popupTextColor( QColor( 50, 50, 50 ) ),
	selectionColor(QColor(255, 40, 0, 1)),
	isAnimated( true ),
	animDuration( 100.0 ),
	animStart( 0.0 ),
	separationMargin( 10 ),
	histogramBins(10),
	popupWidth(180),
	pointRadius(1.0),
	selectionMode(iAScatterPlot::Polygon),
	selectionEnabled(true),
	histogramVisible(true),
	quadraticPlots(false),
	showPCC(false),
	enableColorSettings(false),
	colorScheme(AllPointsSame),
	colorThemeName("Diverging blue-gray-red"),
	pointColor(QColor(128, 128, 128))
{
	popupTipDim[0] = 5; popupTipDim[1] = 10;
}

void iAQSplom::setAnimIn( double anim )
{
	m_animIn = anim;
	update();
}

void iAQSplom::setAnimOut( double anim )
{
	m_animOut = anim;
	update();
}

iAQSplom::SelectionType const & iAQSplom::getHighlightedPoints() const
{
	return m_highlightedPoints;
}

void iAQSplom::setSeparation(int idx)
{
	m_separationIdx = idx;
	updatePlotGridParams();
	updateSPLOMLayout();
	update();
}

void iAQSplom::setBackgroundColorTheme(iAColorTheme const * theme)
{
	m_bgColorTheme = theme;
	update();
}

iAColorTheme const * iAQSplom::getBackgroundColorTheme()
{
	return m_bgColorTheme;
}

void iAQSplom::clearSelection()
{
	m_selInds.clear();
}

void iAQSplom::showAllPlots(const bool enableAllPlotsVisible)
{
	if (enableAllPlotsVisible)
		m_mode = ALL_PLOTS;
	else
		m_mode = UPPER_HALF;
	updateVisiblePlots();
}

void iAQSplom::setSelectionColor(QColor color)
{
	settings.selectionColor = color;
	for (auto & row : m_matrix)
		for (auto s : row)
			if (s)
				s->setSelectionColor(color);

	if (m_maximizedPlot)
		m_maximizedPlot->setSelectionColor(color);
}

void iAQSplom::setSelectionMode(int mode)
{
	if (m_maximizedPlot)
		m_maximizedPlot->settings.selectionMode = static_cast<iAScatterPlot::SelectionMode>(mode);
	QSignalBlocker sb(m_settingsDlg->cbSelectionMode);
	m_settingsDlg->cbSelectionMode->setCurrentIndex(mode);
	settings.selectionMode = mode;
}

void iAQSplom::enableSelection(bool enabled)
{
	if (m_maximizedPlot)
		m_maximizedPlot->settings.selectionEnabled = enabled;
	settings.selectionEnabled = enabled;

}

void iAQSplom::selectionModePolygon()
{
	setSelectionMode(iAScatterPlot::Polygon);
}

void iAQSplom::selectionModeRectangle()
{
	setSelectionMode(iAScatterPlot::Rectangle);
}

#if (VTK_MAJOR_VERSION >= 8 && defined(VTK_OPENGL2_BACKEND) )
iAQSplom::iAQSplom(QWidget * parent , Qt::WindowFlags f)
	:QOpenGLWidget(parent, f),
#else
iAQSplom::iAQSplom(QWidget * parent /*= 0*/, const QGLWidget * shareWidget /*= 0*/, Qt::WindowFlags f /*= 0 */)
	:QGLWidget(parent, shareWidget, f),
#endif
	settings(),
	m_lut(new iALookupTable()),
	m_colorLookupParam(0),
	m_activePlot(nullptr),
	m_mode(ALL_PLOTS),
	m_splomData(new iASPLOMData()),
	m_previewPlot(nullptr),
	m_maximizedPlot(nullptr),
	m_animIn(1.0),
	m_animOut(0.0),
	m_animationOut(new QPropertyAnimation(this, "m_animOut")),
	m_animationIn(new QPropertyAnimation(this, "m_animIn")),
	m_popupHeight(0),
	m_separationIdx(-1),
	m_contextMenu(new QMenu(this)),
	m_bgColorTheme(iAColorThemeManager::GetInstance().GetTheme("White")),
	m_settingsDlg(new iASPMSettings(this, f))
{
	setMouseTracking( true );
	setFocusPolicy( Qt::StrongFocus );
	setBackgroundRole(QPalette::Base);
	setAutoFillBackground(true);
#if (VTK_MAJOR_VERSION >= 8 && defined(VTK_OPENGL2_BACKEND) )
	QSurfaceFormat format = QSurfaceFormat();
	format.setSamples(4);
	setFormat(format);
#endif
	m_animationIn->setDuration( settings.animDuration );
	m_animationOut->setDuration( settings.animDuration );

	// set up context menu:
	showHistogramAction = new QAction(tr("Show Histograms"), this);
	showHistogramAction->setCheckable(true);
	showHistogramAction->setChecked(settings.histogramVisible);
	quadraticPlotsAction = new QAction(tr("Quadratic Plots"), this);
	quadraticPlotsAction->setCheckable(true);
	quadraticPlotsAction->setChecked(settings.quadraticPlots);
	showPCCAction = new QAction(tr("Show Pearsons's Correlation Coefficient"), this);
	showPCCAction->setCheckable(true);
	showPCCAction->setChecked(settings.quadraticPlots);
	selectionModePolygonAction = new QAction(tr("Polygon Selection Mode"), this);
	QActionGroup * selectionModeGroup = new QActionGroup(m_contextMenu);
	selectionModeGroup->setExclusive(true);
	selectionModePolygonAction->setCheckable(true);
	selectionModePolygonAction->setChecked(true);
	selectionModePolygonAction->setActionGroup(selectionModeGroup);
	selectionModeRectangleAction = new QAction(tr("Rectangle Selection Mode"), this);
	selectionModeRectangleAction->setCheckable(true);
	selectionModeRectangleAction->setActionGroup(selectionModeGroup);
	QAction* showSettingsAction = new QAction(tr("Settings..."), this);
	addContextMenuAction(showHistogramAction);
	addContextMenuAction(quadraticPlotsAction);
	addContextMenuAction(showPCCAction);
	addContextMenuAction(selectionModeRectangleAction);
	addContextMenuAction(selectionModePolygonAction);
	addContextMenuAction(showSettingsAction);
	connect(showHistogramAction, &QAction::toggled, this, &iAQSplom::setHistogramVisible);
	connect(quadraticPlotsAction, &QAction::toggled, this, &iAQSplom::setQuadraticPlots);
	connect(showPCCAction, &QAction::toggled, this, &iAQSplom::setShowPCC);
	connect(selectionModePolygonAction, SIGNAL(toggled(bool)), this, SLOT(selectionModePolygon()));
	connect(selectionModeRectangleAction, SIGNAL(toggled(bool)), this, SLOT(selectionModeRectangle()));
	connect(showSettingsAction, SIGNAL(triggered()), this, SLOT(showSettings()));
	connect(m_settingsDlg->parametersList, SIGNAL(itemChanged(QListWidgetItem *)), this, SLOT(changeParamVisibility(QListWidgetItem *)));
	connect(m_settingsDlg->cbColorParameter, SIGNAL(currentIndexChanged(int)), this, SLOT(setParameterToColorCode(int)));
	connect(m_settingsDlg->cbColorScheme, SIGNAL(currentIndexChanged(int)), this, SLOT(colorSchemeChanged(int)) );
	connect(m_settingsDlg->sbMin, SIGNAL(valueChanged(double)), this, SLOT(updateLookupTable()));
	connect(m_settingsDlg->sbMax, SIGNAL(valueChanged(double)), this, SLOT(updateLookupTable()));
	connect(m_settingsDlg->slPointOpacity, SIGNAL(valueChanged(int)), this, SLOT(pointOpacityChanged(int)));
	connect(m_settingsDlg->slPointSize, SIGNAL(valueChanged(int)), this, SLOT(pointRadiusChanged(int)));
	connect(m_settingsDlg->pbPointColor, SIGNAL(clicked()), this, SLOT(changePointColor()));
	connect(m_settingsDlg->pbRangeFromParameter, SIGNAL(clicked()), this, SLOT(rangeFromParameter()));
	connect(m_settingsDlg->cbSelectionMode, SIGNAL(currentIndexChanged(int)), this, SLOT(setSelectionMode(int)));
	connect(m_settingsDlg->cbQuadraticPlots, &QCheckBox::toggled, this, &iAQSplom::setQuadraticPlots);
	connect(m_settingsDlg->cbShowCorrelationCoefficient, &QCheckBox::toggled, this, &iAQSplom::setShowPCC);
	connect(m_settingsDlg->cbShowHistograms, &QCheckBox::toggled, this, &iAQSplom::setHistogramVisible);
	connect(m_settingsDlg->sbHistogramBins, SIGNAL(valueChanged(int)), this, SLOT(setHistogramBins(int)));
	connect(m_settingsDlg->cbColorTheme, SIGNAL(currentIndexChanged(QString const &)), this, SLOT(setColorTheme(QString const &)));
	m_settingsDlg->cbColorTheme->addItems(iALUT::GetColorMapNames());
	m_columnPickMenu = m_contextMenu->addMenu("Columns");
}

void iAQSplom::addContextMenuAction(QAction* action)
{
	m_contextMenu->addAction(action);
}

void iAQSplom::updateHistogram(size_t paramIndex)
{
	std::vector<double> hist_InputValues;
	for (size_t i = 0; i < m_splomData->numPoints(); ++i)
	{
		if (m_splomData->matchesFilter(i))
			hist_InputValues.push_back(m_splomData->paramData(paramIndex)[i]);
	}
	if (m_histograms[paramIndex]->plots().size() > 0)
		m_histograms[paramIndex]->removePlot(m_histograms[paramIndex]->plots()[0]);

	auto histogramData = iAHistogramData::Create(hist_InputValues, settings.histogramBins);
	auto histogramPlot = QSharedPointer<iABarGraphPlot>(new iABarGraphPlot(histogramData, QColor(70, 70, 70, 255)));
	m_histograms[paramIndex]->addPlot(histogramPlot);
	m_histograms[paramIndex]->update();
}

void iAQSplom::updateHistograms()
{
	if (!settings.histogramVisible)
		return;
	for (size_t y = 0; y < m_splomData->numParams(); ++y)
	{
		if (m_paramVisibility[y])
		{
			updateHistogram(y);
		}
	}
}

void iAQSplom::addFilter(int paramIndex, double value)
{
	m_splomData->addFilter(paramIndex, value);
	updateFilter();
}

void iAQSplom::removeFilter(int paramIndex, double value)
{
	m_splomData->removeFilter(paramIndex, value);
	updateFilter();
}

void iAQSplom::resetFilter()
{
	m_splomData->clearFilter();
	updateFilter();
}

void iAQSplom::updateFilter()
{
	for(auto & row: m_visiblePlots)
		for(iAScatterPlot * s: row)
			if (s)
				s->updatePoints();
	if (m_maximizedPlot)
		m_maximizedPlot->updatePoints();

	updateHistograms();
	update();
}

void iAQSplom::initializeGL()
{
}

iAQSplom::~iAQSplom()
{
	delete m_maximizedPlot;
	delete m_animationIn;
	delete m_animationOut;
	delete m_contextMenu;
}

void iAQSplom::setData( const QTableWidget * data )
{
	m_splomData->import( data );
	std::vector<char> allVisible(m_splomData->numParams(), true);
	dataChanged(allVisible);
}

void iAQSplom::setData( QSharedPointer<iASPLOMData> data, std::vector<char> const & visibility )
{
	m_splomData = data;
	dataChanged(visibility);
}

QSharedPointer<iASPLOMData> iAQSplom::data()
{
	return m_splomData;
}

void iAQSplom::createScatterPlot(size_t y, size_t x, bool initial)
{
	if (!m_paramVisibility[y] || !m_paramVisibility[x] || (m_mode == UPPER_HALF && x >= y)
		|| (m_matrix.size() > y && m_matrix[y][x]))
		return;
	iAScatterPlot * s = new iAScatterPlot(this, this);
	connect(s, &iAScatterPlot::transformModified, this, &iAQSplom::transformUpdated);
	connect(s, &iAScatterPlot::currentPointModified, this, &iAQSplom::currentPointUpdated);
	s->setData(x, y, m_splomData);
	s->setSelectionColor(settings.selectionColor);
	s->setPointRadius(settings.pointRadius);
	s->settings.showPCC = settings.showPCC;
	if (!initial)
	{
		s->setLookupTable(m_lut, m_colorLookupParam);
		QPointF offset;
		double scale = 1.0;
		size_t curPoint = iAScatterPlot::NoPointIndex;
		for (size_t p = 0; p < m_splomData->numParams(); ++p)
			if (m_matrix[p][x])
			{
				offset.setX(m_matrix[p][x]->getOffset().x());
				scale = m_matrix[p][x]->getScale();
				curPoint = m_matrix[p][x]->getCurrentPoint();
				break;
			}
		for (size_t p = 0; p < m_splomData->numParams(); ++p)
			if (m_matrix[y][p])
			{
				offset.setY(m_matrix[y][p]->getOffset().y());
				scale = m_matrix[y][p]->getScale();
				curPoint = m_matrix[y][p]->getCurrentPoint();
				break;
			}
		s->setTransform(scale, offset);
		s->setCurrentPoint(curPoint);
	}
	m_matrix[y][x] = s;
}

void iAQSplom::dataChanged(std::vector<char> visibleParams)
{
	clear();
	m_paramVisibility = visibleParams;
	m_columnPickMenu->clear();
	unsigned long numParams = m_splomData->numParams();

	// cleanup old histograms (if any)
	for (auto histo : m_histograms)
		delete histo;
	m_histograms.clear();

	QSignalBlocker blockListSignals(m_settingsDlg->parametersList);
	QSignalBlocker colorSignals(m_settingsDlg->cbColorParameter);
	m_settingsDlg->parametersList->clear();
	m_settingsDlg->cbColorParameter->clear();
	m_matrix.resize(numParams);
	for( unsigned long y = 0; y < numParams; ++y )
	{
		m_matrix[y].resize(numParams, nullptr);
		for (unsigned long x = 0; x < numParams; ++x)
			createScatterPlot(y, x, true);

		m_histograms.push_back(new iAChartWidget(this, m_splomData->parameterName(y), ""));

		QAction * a = new QAction(m_splomData->parameterName(y), nullptr);
		a->setCheckable(true);
		m_columnPickMenu->addAction(a);
		connect(a, &QAction::toggled, this, &iAQSplom::parameterVisibilityToggled);

		QListWidgetItem * item = new QListWidgetItem(m_splomData->parameterName(y), m_settingsDlg->parametersList);
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable); // set checkable flag
		item->setCheckState(Qt::Checked); // AND initialize check state

		m_settingsDlg->cbColorParameter->addItem(m_splomData->parameterName(y));
	}
	updateLookupTable();
	updateVisiblePlots();
	updateHistograms();
}

void iAQSplom::setLookupTable( vtkLookupTable * lut, const QString & paramName )
{
	size_t colorLookupCol = m_splomData->paramIndex(paramName);
	if (colorLookupCol == std::numeric_limits<size_t>::max())
		return;
	m_lut->copyFromVTK( lut );
	m_colorLookupParam = colorLookupCol;
	setColorScheme(Custom);
}

void iAQSplom::setLookupTable( iALookupTable &lut, size_t columnIndex)
{
	if (columnIndex >= m_splomData->numParams())
		return;
	*m_lut = lut;
	m_colorLookupParam = columnIndex;
	setColorScheme(Custom);
}

void iAQSplom::parameterVisibilityToggled(bool isVisible)
{
	QAction* sender = qobject_cast<QAction*>(QObject::sender());
	size_t paramIndex = m_splomData->paramIndex(sender->text());
	setParameterVisibility( paramIndex, isVisible );
	emit parameterVisibilityChanged( paramIndex, isVisible );
}

void iAQSplom::setParameterVisibility( const QString & paramName, bool isVisible )
{
	setParameterVisibility(m_splomData->paramIndex(paramName), isVisible);
}

void iAQSplom::setParameterVisibility( size_t paramIndex, bool isVisible )
{
	if( paramIndex < 0 || paramIndex >= m_paramVisibility.size() || static_cast<bool>(m_paramVisibility[paramIndex]) == isVisible )
		return;
	m_paramVisibility[paramIndex] = isVisible;
	if (settings.histogramVisible)
		updateHistogram(paramIndex);
	for (size_t p = 0; p < m_splomData->numParams(); ++p)
	{
		createScatterPlot(p, paramIndex, false);
		createScatterPlot(paramIndex, p, false);
	}
	updateVisiblePlots();
	update();
}

void iAQSplom::setParameterVisibility(std::vector<char> const & visibility)
{
	if (visibility.size() != m_paramVisibility.size())
	{
		DEBUG_LOG("Call to setParameterVisibility with vector of invalid size!");
		return;
	}
	m_paramVisibility = visibility;
	for (size_t y = 0; y < m_splomData->numParams(); ++y)
		for (size_t x = 0; x < m_splomData->numParams(); ++x)
			createScatterPlot(y, x, false);

	updateVisiblePlots();
	updateHistograms();
	update();
}

void iAQSplom::setParameterInverted( size_t paramIndex, bool isInverted )
{
	m_splomData->setInverted(paramIndex, isInverted);
	unsigned long numParams = m_splomData->numParams();
	for (unsigned long row = 0; row < numParams; ++row)
	{
		auto s = m_matrix[row][paramIndex];
		if (s)
			s->updatePoints();
	}
	for (unsigned long col = 0; col < numParams; ++col)
	{  // avoid double updated of row==col plot
		auto s = m_matrix[paramIndex][col];
		if (s && col != paramIndex)
			s->updatePoints();
	}
	update();
}

iAQSplom::SelectionType const & iAQSplom::getSelection() const
{
	return m_selInds;
}

iAQSplom::SelectionType & iAQSplom::getSelection()
{
	return m_selInds;
}

iAQSplom::SelectionType const & iAQSplom::getFilteredSelection() const
{
	if (!m_splomData->filterDefined() || m_selInds.size() == 0)
	{
		m_filteredSelInds = m_selInds;
		return m_filteredSelInds;
	}
	m_filteredSelInds.clear();
	size_t curFilteredIdx = 0;
	size_t curSelIdx = 0;
	const double Epsilon = 1e-10;
	for (size_t curIdx = 0; curIdx < m_splomData->numPoints(); ++curIdx)
	{
		if (!m_splomData->matchesFilter(curIdx))
			continue;
		if (curSelIdx >= m_selInds.size())
			break;
		if (curIdx == m_selInds[curSelIdx])
		{
			m_filteredSelInds.push_back(curFilteredIdx);
			++curSelIdx;
		}
		++curFilteredIdx;
	}
	return m_filteredSelInds;
}

void iAQSplom::setSelection( iAQSplom::SelectionType const & selInds )
{
	m_selInds = selInds;
	std::sort(m_selInds.begin(), m_selInds.end());
	update();
}

void iAQSplom::setFilteredSelection(iAQSplom::SelectionType const & filteredSelInds)
{
	if (!m_splomData->filterDefined())
	{
		setSelection(filteredSelInds);
		return;
	}
	std::vector<size_t> sortedFilteredSelInds = filteredSelInds;
	std::sort(sortedFilteredSelInds.begin(), sortedFilteredSelInds.end());
	size_t curFilteredIdx = 0,
		curSelIdx = 0;
	m_selInds.clear();
	for (size_t curIdx = 0; curIdx < m_splomData->numPoints(); ++curIdx)
	{
		if (!m_splomData->matchesFilter(curIdx))
			continue;
		if (curSelIdx >= sortedFilteredSelInds.size())
			break;
		if (curFilteredIdx == sortedFilteredSelInds[curSelIdx])
		{
			m_selInds.push_back(curIdx);
			++curSelIdx;
		}
		++curFilteredIdx;
	}
	update();
}

void iAQSplom::getActivePlotIndices( int * inds_out )
{
	if( !m_activePlot )
	{
		inds_out[0] = inds_out[1] = -1;
	}
	else
	{
		inds_out[0] = m_activePlot->getIndices()[0];
		inds_out[1] = m_activePlot->getIndices()[1];
	}
}

int iAQSplom::getVisibleParametersCount() const
{
	return m_visiblePlots.size();
}

void iAQSplom::clear()
{
	removeMaximizedPlot();
	m_activePlot = 0;
	for (auto & row: m_matrix)
	{
		for (auto s: row)
		{
			if (s)
			{
				s->disconnect();
				delete s;
			}
		}
		row.clear();
	}
	m_matrix.clear();
	m_histograms.clear();
	m_paramVisibility.clear();
}

void iAQSplom::selectionUpdated()
{
	update();
	emit selectionModified( m_selInds );
}

void iAQSplom::transformUpdated( double scale, QPointF deltaOffset )
{
	iAScatterPlot * sender = dynamic_cast<iAScatterPlot*>( QObject::sender() );
	if (!sender)
		return;

	if (m_maximizedPlot && settings.maximizedLinked)
	{
		m_maximizedPlot->setTransform(sender->getScale(),
									  QPointF(sender->getOffset().x() / sender->getRect().width() * m_maximizedPlot->getRect().width(),
											  sender->getOffset().y() / sender->getRect().height() * m_maximizedPlot->getRect().height()));
		if (sender == m_maximizedPlot)
			deltaOffset = QPointF(deltaOffset.x() / m_maximizedPlot->getRect().width() * m_previewPlot->getRect().width(),
								  deltaOffset.y() / m_maximizedPlot->getRect().height() * m_previewPlot->getRect().height());
	}

	const int * ind = sender->getIndices();
	for ( auto & row: m_matrix )
	{
		for ( auto s: row )
		{
			if( s && s != sender )
			{
				if( s->getIndices()[0] == ind[0] )
					s->setTransformDelta( scale, QPointF( deltaOffset.x(), 0.0f ) );
				else if( s->getIndices()[1] == ind[1] )
					s->setTransformDelta( scale, QPointF( 0.0f, deltaOffset.y() ) );
				else
					s->setTransformDelta( scale, QPointF( 0.0f, 0.0f ) );
			}
		}
	}

	update();
}

void iAQSplom::currentPointUpdated( size_t index )
{
	for (auto & row : m_matrix)
	{
		for (auto s : row)
		{
			if( s && s != QObject::sender() )
			{
				s->setCurrentPoint( index );
			}
		}
	}
	if( m_maximizedPlot && ( m_maximizedPlot != QObject::sender() ) )
		m_maximizedPlot->setCurrentPoint( index );

	//animation
	if( settings.isAnimated && m_activePlot )
	{
		size_t curPt = m_activePlot->getCurrentPoint();
		size_t prePt = m_activePlot->getPreviousIndex();
		if( prePt != iAScatterPlot::NoPointIndex && curPt != prePt )
		{
			//Out
			m_animationOut->setStartValue( m_animIn );
			m_animationOut->setEndValue( 0.0 );
			m_animationOut->start();
		}
		if( curPt != iAScatterPlot::NoPointIndex )
		{
			//In
			m_animationIn->setStartValue( settings.animStart );
			m_animationIn->setEndValue( 1.0 );
			m_animationIn->start();
		}
	}
	repaint(); // should be update, but that does not work if called from base class at the moment (no idea why)
	if( index != iAScatterPlot::NoPointIndex )
		emit currentPointModified( index );
}

void iAQSplom::addHighlightedPoint( size_t index )
{
	if ( std::find(m_highlightedPoints.begin(), m_highlightedPoints.end(), index) == m_highlightedPoints.end() )
	{
		m_highlightedPoints.push_back( index );
		update();
	}
}

void iAQSplom::removeHighlightedPoint( size_t index )
{
	auto it = std::find(m_highlightedPoints.begin(), m_highlightedPoints.end(), index );
	if (it != m_highlightedPoints.end())
	{
		m_highlightedPoints.erase(it);
		update();
	}
}

void iAQSplom::showDefaultMaxizimedPlot()
{
	if (m_visiblePlots.empty())
		return;
	// maximize plot in upper left corner:
	this->maximizeSelectedPlot(m_visiblePlots.at(getVisibleParametersCount() - 1).at(0));
}

void iAQSplom::setHistogramVisible(bool visible)
{
	settings.histogramVisible = visible;
	QSignalBlocker sb(m_settingsDlg->cbShowHistograms);
	m_settingsDlg->cbShowHistograms->setChecked(visible);
	updateVisiblePlots();
	updateHistograms();
}

void iAQSplom::setHistogramBins(int bins)
{
	settings.histogramBins = bins;
	if (!settings.histogramVisible)
		return;
	updateHistograms();
}

void iAQSplom::setQuadraticPlots(bool quadratic)
{
	settings.quadraticPlots = quadratic;
	QSignalBlocker sb(m_settingsDlg->cbQuadraticPlots);
	m_settingsDlg->cbQuadraticPlots->setChecked(quadratic);
	updatePlotGridParams();
	updateSPLOMLayout();
	update();
}

void iAQSplom::setShowPCC(bool showPCC)
{
	settings.showPCC = showPCC;
	QSignalBlocker sb(m_settingsDlg->cbShowCorrelationCoefficient);
	m_settingsDlg->cbShowCorrelationCoefficient->setChecked(showPCC);
	for (auto row : m_matrix)
		for (auto s : row)
			if (s)
				s->settings.showPCC = showPCC;
	if (m_maximizedPlot)
		m_maximizedPlot->settings.showPCC = showPCC;
	update();
}

void iAQSplom::contextMenuEvent(QContextMenuEvent * event)
{
	showHistogramAction->setChecked(settings.histogramVisible);
	quadraticPlotsAction->setChecked(settings.quadraticPlots);
	{
		QSignalBlocker sb1(selectionModeRectangleAction), sb2(selectionModePolygonAction);
		selectionModeRectangleAction->setChecked(settings.selectionMode == iAScatterPlot::Rectangle);
		selectionModePolygonAction->setChecked(settings.selectionMode == iAScatterPlot::Polygon);
	}
	showPCCAction->setChecked(settings.showPCC);
	for (auto col : m_columnPickMenu->actions())
	{
		size_t paramIdx = m_splomData->paramIndex(col->text());
		if (paramIdx >= m_splomData->numParams())
		{
			DEBUG_LOG(QString("Invalid menu entry %1 in column pick submenu - there is currently no such column!").arg(col->text()));
			continue;
		}
		QSignalBlocker toggleBlock(col);
		col->setChecked( m_paramVisibility[paramIdx] );
	}
	m_contextMenu->exec(event->globalPos());
}

void iAQSplom::maximizeSelectedPlot(iAScatterPlot *selectedPlot) 
{
	if (!selectedPlot) 
		return;

	if (m_previewPlot)
		m_previewPlot->setPreviewState(false);

	selectedPlot->setPreviewState(true);
	m_previewPlot = selectedPlot;
	if (m_mode == ALL_PLOTS)
	{	// hide lower triangle
		m_mode = UPPER_HALF;
		for (int y = 0; y < getVisibleParametersCount(); ++y)
			for (int x = 0; x < getVisibleParametersCount(); ++x)
				if (x >= y)
					m_visiblePlots[y][x] = nullptr;
	}

	delete m_maximizedPlot;
	m_maximizedPlot = new iAScatterPlot(this, this, 11, true);
	connect(m_maximizedPlot, &iAScatterPlot::selectionModified, this, &iAQSplom::selectionUpdated);
	connect(m_maximizedPlot, &iAScatterPlot::currentPointModified, this, &iAQSplom::currentPointUpdated);

	if (settings.maximizedLinked)
		connect(m_maximizedPlot, &iAScatterPlot::transformModified, this, &iAQSplom::transformUpdated);

	const int * plotInds = selectedPlot->getIndices();
	m_maximizedPlot->setData(plotInds[0], plotInds[1], m_splomData); //we want first plot in lower left corner of the SPLOM
	m_maximizedPlot->setLookupTable(m_lut, m_colorLookupParam);
	m_maximizedPlot->setSelectionColor(settings.selectionColor);
	m_maximizedPlot->setPointRadius(settings.pointRadius);
	m_maximizedPlot->settings.selectionMode = static_cast<iAScatterPlot::SelectionMode>(settings.selectionMode);
	m_maximizedPlot->settings.selectionEnabled = settings.selectionEnabled;
	m_maximizedPlot->settings.showPCC = settings.showPCC;
	updateMaxPlotRect();
	if (selectedPlot->getRect().height() > 0)
	{
		QPointF ofst = selectedPlot->getOffset();
		double scl[2] = { ((double)m_maximizedPlot->getRect().width()) / selectedPlot->getRect().width(),
			((double)m_maximizedPlot->getRect().height()) / selectedPlot->getRect().height() };
		m_maximizedPlot->setTransform(selectedPlot->getScale(), QPointF(ofst.x() * scl[0], ofst.y() * scl[1]));
	}
	update();
}

void iAQSplom::removeMaximizedPlot()
{
	delete m_maximizedPlot;
	m_maximizedPlot = 0;
	m_activePlot = 0;
	if( m_previewPlot )
	{
		m_previewPlot->setPreviewState( false );
		m_previewPlot = 0;
	}
}

int iAQSplom::invert( int val ) const
{
	return ( getVisibleParametersCount() - val - 1 );
}

void iAQSplom::paintEvent( QPaintEvent * event )
{
	QPainter painter( this );
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setRenderHint(QPainter::HighQualityAntialiasing);
	painter.beginNativePainting();
	glClearColor(settings.backgroundColor.redF(), settings.backgroundColor.greenF(), settings.backgroundColor.blueF(), settings.backgroundColor.alphaF());
	glClear(GL_COLOR_BUFFER_BIT);
	painter.endNativePainting();
	if (m_visiblePlots.size() < 2)
	{
		painter.drawText(geometry(), Qt::AlignCenter | Qt::AlignVCenter, "Too few parameters selected!");
		return;
	}
	QFontMetrics fm = painter.fontMetrics();
	
	// collect tick labels text and positions:
	QList<double> ticksX, ticksY; QList<QString> textX, textY;
	for (int i = 0; i < m_visiblePlots.size() -1; ++i)
	{                //y  //x
		m_visiblePlots[i+1][i]->printTicksInfo(&ticksX, &ticksY, &textX, &textY);
	}
	int maxTickLabelWidth = getMaxTickLabelWidth(textX, fm);
	if (settings.tickOffsets.x() != maxTickLabelWidth || settings.tickOffsets.y() != maxTickLabelWidth)
	{
		settings.tickOffsets.setX(maxTickLabelWidth);
		settings.tickOffsets.setY(maxTickLabelWidth);
		updateSPLOMLayout();
	}

	if (m_separationIdx != -1)
	{
		QRect upperLeft = getPlotRectByIndex(0, getVisibleParametersCount()-1);
		QRect lowerRight = getPlotRectByIndex(getVisibleParametersCount()-1, 0);
		QRect separation = getPlotRectByIndex(m_separationIdx+1, m_separationIdx+1);
		QRect r1(
			QPoint(upperLeft.left(), upperLeft.top()), QPoint(separation.left() - settings.separationMargin - settings.plotsSpacing, separation.bottom())
		);
		QColor c1(m_bgColorTheme->GetColor(0)); c1.setAlpha(64);
		painter.fillRect(r1, QBrush(c1));
		if (!m_maximizedPlot)
		{
			QColor c2(m_bgColorTheme->GetColor(1)); c2.setAlpha(64);
			QColor c3(m_bgColorTheme->GetColor(3)); c3.setAlpha(64);
			QRect r2(
				QPoint(separation.left(), separation.bottom() + settings.separationMargin + settings.plotsSpacing), QPoint(lowerRight.right(), lowerRight.bottom())
			), r3(
				QPoint(upperLeft.left(), separation.bottom() + settings.separationMargin + settings.plotsSpacing), QPoint(separation.left() - settings.separationMargin - settings.plotsSpacing, lowerRight.bottom())
			), r4(
				QPoint(separation.left(), upperLeft.top()), QPoint(lowerRight.right(), separation.bottom())
			);
			painter.fillRect(r2, QBrush(c1));
			painter.fillRect(r3, QBrush(c2));
			painter.fillRect(r4, QBrush(c3));
		}
	}
	if( !getVisibleParametersCount() )
		return;

	//draw elements
	drawVisibleParameters(painter);

	drawTicks( painter, ticksX, ticksY, textX, textY );
	for( auto & row: m_visiblePlots )
		for( iAScatterPlot * s: row )
			if( s )
				s->paintOnParent( painter );

	if( m_maximizedPlot )
		m_maximizedPlot->paintOnParent( painter );
	drawPopup( painter );
}

bool iAQSplom::drawPopup( QPainter& painter )
{
	if( !m_activePlot )
		return false;
	size_t curInd = m_activePlot->getCurrentPoint();
	double anim = 1.0;
	if( curInd == iAScatterPlot::NoPointIndex )
	{
		if( m_animOut > 0.0 && m_animIn >= 1.0)
		{
			anim = m_animOut;
			curInd = m_activePlot->getPreviousPoint();
		}
		return false;
	}
	else if ( m_activePlot->getPreviousIndex() == iAScatterPlot::NoPointIndex )
		anim = m_animIn;
	const int * pInds = m_activePlot->getIndices();

	painter.save();
	QPointF popupPos = m_activePlot->getPointPosition( curInd );
	double pPM = m_activePlot->settings.pickedPointMagnification;
	double ptRad = m_activePlot->getPointRadius();
	popupPos.setY( popupPos.y() -  pPM * ptRad ); //popupPos.setY( popupPos.y() - ( 1 + ( pPM - 1 )*m_anim ) * ptRad );
	QColor col = settings.popupFillColor; col.setAlpha( col.alpha()* anim ); painter.setBrush( col );
	col = settings.popupBorderColor; col.setAlpha( col.alpha()* anim ); painter.setPen( col );
	//painter.setBrush( settings.popupFillColor );
	//painter.setPen( settings.popupBorderColor );
	painter.translate( popupPos );

	QString text = "<center><b>#" + QString::number(curInd) + "</b><br> " + \
		m_splomData->parameterName(pInds[0]) + ": " + \
		QString::number(m_splomData->paramData(pInds[0])[curInd]) + "<br>" + \
		m_splomData->parameterName(pInds[1]) + ": " + \
		QString::number(m_splomData->paramData(pInds[1])[curInd]) + "</center>";
	QTextDocument doc;
	doc.setHtml(text);
	doc.setTextWidth(settings.popupWidth);

	double * tipDim = settings.popupTipDim;
	double popupWidthHalf = settings.popupWidth / 2;
	m_popupHeight = doc.size().height();
	QPointF points[7] = {
		QPointF( 0, 0 ),
		QPointF( -tipDim[0], -tipDim[1] ),
		QPointF( -popupWidthHalf, -tipDim[1] ),
		QPointF( -popupWidthHalf, -m_popupHeight - tipDim[1] ),
		QPointF( popupWidthHalf, -m_popupHeight - tipDim[1] ),
		QPointF( popupWidthHalf, -tipDim[1] ),
		QPointF( tipDim[0], -tipDim[1] ),
	};
	painter.drawPolygon( points, 7 );

	painter.translate( -popupWidthHalf, -m_popupHeight - tipDim[1] );
	QAbstractTextDocumentLayout::PaintContext ctx;
	col = settings.popupTextColor; col.setAlpha( col.alpha()* anim );
	ctx.palette.setColor( QPalette::Text, col );
	ctx.palette.setColor( QPalette::Text, settings.popupTextColor );
	doc.documentLayout()->draw( &painter, ctx ); //doc.drawContents( &painter );

	painter.restore();
	return true;
}

iAScatterPlot * iAQSplom::getScatterplotAt( QPoint pos )
{
	if( m_visiblePlots.empty() )
		return nullptr;
	QPoint offsetPos = pos - settings.tickOffsets;
	QPoint grid( m_scatPlotSize.x() + settings.plotsSpacing, m_scatPlotSize.y() + settings.plotsSpacing );
	if (grid.x() == 0 || grid.y() == 0)
		return nullptr;	// to avoid division by 0
	int ind[2] = { offsetPos.x() / grid.x(), offsetPos.y() / grid.y() };
	//boundary checks
	for( int i = 0; i < 2; ++i )
	{
		ind[i] = clamp(0, getVisibleParametersCount() - 1, ind[i]);
	}
	if (ind[0] > m_separationIdx)
		offsetPos.setX(offsetPos.x() - settings.separationMargin);
	if (ind[1] > m_separationIdx)
		offsetPos.setY(offsetPos.y() - settings.separationMargin);
	//are we between plots due to the spacing?
	bool isBetween = false;
	if( offsetPos.x() - ind[0] * grid.x() >= m_scatPlotSize.x() ) isBetween = true;
	if( offsetPos.y() - ind[1] * grid.y() >= m_scatPlotSize.y() ) isBetween = true;
	ind[1] = invert( ind[1] );	// indexing is bottom to top -> invert index Y
	//get the resulting plot
	iAScatterPlot * s = isBetween ? 0 : m_visiblePlots[ind[1]][ind[0]];
	//check if we hit the maximized plot if necessary
	if( ( UPPER_HALF == m_mode ) && !s )
	{
		if( m_maximizedPlot )
		{
			QRect r = m_maximizedPlot->getRect();
			if( r.contains( pos ) )
				s = m_maximizedPlot;
		}
	}
	return s;
}

void iAQSplom::resizeEvent( QResizeEvent * event )
{
	updateSPLOMLayout();
#if (VTK_MAJOR_VERSION >= 8 && defined(VTK_OPENGL2_BACKEND) )
	QOpenGLWidget::resizeEvent( event );
#else
	QGLWidget::resizeEvent( event );
#endif
}

void iAQSplom::updatePlotGridParams()
{
	long plotsRect[2] = {
		width() - settings.tickOffsets.x(),
		height() - settings.tickOffsets.y() };
	long visParamCnt = getVisibleParametersCount();
	int spc = settings.plotsSpacing;
	m_scatPlotSize = QPoint(
		static_cast<int>(( plotsRect[0] - ( visParamCnt - 1 ) * spc - ((m_separationIdx != -1) ? settings.separationMargin : 0) ) / ( (double)visParamCnt )),
		static_cast<int>(( plotsRect[1] - ( visParamCnt - 1 ) * spc - ((m_separationIdx != -1) ? settings.separationMargin : 0) ) / ( (double)visParamCnt ))
	);
	if (settings.quadraticPlots)
	{
		if (m_scatPlotSize.x() < m_scatPlotSize.y())
			m_scatPlotSize.setY(m_scatPlotSize.x());
		else
			m_scatPlotSize.setX(m_scatPlotSize.y());
	}
}

QRect iAQSplom::getPlotRectByIndex( int x, int y )
{
	y = invert( y );
	int spc = settings.plotsSpacing;
	int xpos = settings.tickOffsets.x() + x * ( m_scatPlotSize.x() + spc ) + ((m_separationIdx != -1 && x > m_separationIdx) ? settings.separationMargin : 0);
	int ypos = settings.tickOffsets.y() + y * ( m_scatPlotSize.y() + spc ) + ((m_separationIdx != -1 && y > (getVisibleParametersCount() - m_separationIdx - 2)) ? settings.separationMargin : 0);
	QRect res( xpos, ypos, m_scatPlotSize.x(), m_scatPlotSize.y() );
	return res;
}

void iAQSplom::updateMaxPlotRect()
{
	if( !m_maximizedPlot )
		return;
	long visParamCnt = getVisibleParametersCount();
	QRect topLeftPlot = getPlotRectByIndex(0, visParamCnt - 1);
	QRect bottomRightPlot = getPlotRectByIndex(visParamCnt - 1, 0);
	// default top left for max plot is in the middle of the chart area:
	QPoint topLeft(topLeftPlot.left() + (bottomRightPlot.right() - topLeftPlot.left() + settings.plotsSpacing) / 2,
		topLeftPlot.top() + (bottomRightPlot.bottom() - topLeftPlot.top() + settings.plotsSpacing) / 2);
	// make sure there is enough space for the labels:
	double xofs = std::max(0, settings.tickOffsets.x() - ((visParamCnt % 2) ? m_scatPlotSize.x() / 2 : m_scatPlotSize.x()));
	double yofs = std::max(0, settings.tickOffsets.y() - ((visParamCnt % 2) ? m_scatPlotSize.y() / 2 : m_scatPlotSize.y()));
	topLeft += QPoint(xofs,yofs);
	if (settings.histogramVisible)
		topLeft += QPoint(m_scatPlotSize.x()/2, m_scatPlotSize.y()/2);
	m_maximizedPlot->setRect( QRect(topLeft, bottomRightPlot.bottomRight()) );
}

void iAQSplom::updateSPLOMLayout()
{
	long visParamCnt = getVisibleParametersCount();
	if( !visParamCnt )
		return;
	updatePlotGridParams();
	for( int yind = 0; yind < visParamCnt; ++yind )
	{
		auto & row = m_visiblePlots[yind];
		for( int xind = 0; xind < row.size(); ++xind )
		{
			iAScatterPlot * s = row[xind];
			if( s )
				s->setRect( getPlotRectByIndex( xind, yind ) );
		}
		QRect rect = getPlotRectByIndex(yind, yind);
		if (settings.histogramVisible)
			m_histograms[ m_visibleIndices[yind] ]->setGeometry(rect);
	}
	updateMaxPlotRect();
}

void iAQSplom::updateVisiblePlots()
{
	if (!m_splomData)
		return;
	m_visibleIndices.clear();
	removeMaxPlotIfHidden();
	m_visiblePlots.clear();
	QSignalBlocker listBlocker(m_settingsDlg->parametersList);
	for( size_t y = 0; y < m_splomData->numParams(); ++y )
	{
		m_histograms[y]->setVisible(settings.histogramVisible && m_paramVisibility[y]);
		m_settingsDlg->parametersList->item(y)->setCheckState(m_paramVisibility[y] ? Qt::Checked : Qt::Unchecked);

		if( !m_paramVisibility[y] )
			continue;

		std::vector<iAScatterPlot*> row;
		for( size_t x = 0; x < m_splomData->numParams(); ++x )
		{
			if( !m_paramVisibility[x] )
				continue;
			iAScatterPlot * plot = (m_mode == UPPER_HALF && x >= y) ? nullptr : m_matrix[y][x];
			row.push_back( plot );
		}
		m_visiblePlots.push_back( row );
		m_visibleIndices.push_back(y);
	}
	updateSPLOMLayout();
}

void iAQSplom::removeMaxPlotIfHidden()
{
	if( m_maximizedPlot )
	{
		const int * inds = m_maximizedPlot->getIndices();
		if( !m_paramVisibility[inds[0]] || !m_paramVisibility[inds[1]] || ( getVisibleParametersCount() <= 1 ) )
			removeMaximizedPlot();
	}
}

void iAQSplom::resetTransform()
{
	for( auto & row: m_matrix )
		for( auto s: row )
			if (s)
				s->setTransform( 1.0, QPointF( 0.0f, 0.0f ) );

	if(m_maximizedPlot)
		m_maximizedPlot->setTransform( 1.0, QPointF( 0.0f, 0.0f ) );
	update();
}

void iAQSplom::wheelEvent( QWheelEvent * event )
{
	iAScatterPlot * s = getScatterplotAt( event->pos() );
	if( s )
	{
		s->SPLOMWheelEvent( event );
		if( !event->angleDelta().isNull() )
			update();
	}
	event->accept();
}

void iAQSplom::mousePressEvent( QMouseEvent * event )
{
	setContextMenuPolicy(Qt::DefaultContextMenu);
	if( m_activePlot )
		m_activePlot->SPLOMMousePressEvent( event );
}

void iAQSplom::mouseReleaseEvent( QMouseEvent * event )
{
	if( m_activePlot )
		m_activePlot->SPLOMMouseReleaseEvent( event );
}

void iAQSplom::mouseMoveEvent( QMouseEvent * event )
{
	iAScatterPlot * s = getScatterplotAt( event->pos() );

	//make sure that if a button is pressed another plot will not hijack the event handling
	if (event->buttons()&Qt::RightButton || event->buttons()&Qt::LeftButton)
	{
		setContextMenuPolicy(Qt::PreventContextMenu);
		s = m_activePlot;
	}
	else
		changeActivePlot( s );

	if( s )
		s->SPLOMMouseMoveEvent( event );
}

void iAQSplom::keyPressEvent( QKeyEvent * event )
{
	switch (event->key())
	{
	case Qt::Key_R: //if R is pressed, reset all the applied transformation as offset and scaling
		resetTransform();
		break;
	}
}

void iAQSplom::mouseDoubleClickEvent( QMouseEvent * event )
{
	iAScatterPlot * s = getScatterplotAt(event->pos());
	if (!s)
		return;
	if (m_maximizedPlot &&
		m_maximizedPlot->getIndices()[0] == s->getIndices()[0] &&
		m_maximizedPlot->getIndices()[1] == s->getIndices()[1])
	{
		removeMaximizedPlot();
		updateVisiblePlots();
		update();
	}
	else
	{
		maximizeSelectedPlot(s);
	}
	event->accept();
}

void iAQSplom::changeActivePlot( iAScatterPlot * s )
{
	if( s != m_activePlot )
	{
		if( m_activePlot )
			m_activePlot->leave();
		if( s )
			s->enter();
		m_activePlot = s;
		update();
	}
}

int iAQSplom::getMaxTickLabelWidth(QList<QString> const & textX, QFontMetrics & fm) const
{
	int maxLength = 0;
	for (long i = 0; i < textX.size(); ++i)
	{
		maxLength = std::max(fm.width(textX[i]), maxLength);
	}
	return maxLength+2*TextPadding + fm.height() ;
}


//should be performed in the paint event of qslpom
void iAQSplom::drawVisibleParameters(QPainter &painter)
{
	//save indezes of all visible plots
	unsigned long numParams = m_splomData->numParams();

	//QVector<ulong> ind_VisY;
	QVector<ulong> ind_Vis;

	//save visibilitys of axis
	for (unsigned long y = 0; y < numParams; ++y)
	{
		if (m_paramVisibility[y])
			ind_Vis.push_back(y);
	}

	//getting x positions
	drawPlotLabels(ind_Vis, ind_Vis.length(), painter, false);
	drawPlotLabels(ind_Vis, ind_Vis.length(), painter, true);
}

void iAQSplom::drawPlotLabels(QVector<ulong> &ind_Elements, int axisOffSet, QPainter & painter, bool switchTO_YRow)
{
	QRect currentRect;

	int width;
	int height = 0;
	int top = 0;
	int loopLength = 0;
	int textwidth = 0;
	int textHeight = painter.fontMetrics().height();

	loopLength = ind_Elements.length();

	for (int axisIdx = 0; axisIdx < loopLength; axisIdx++)
	{
		ulong currIdx = ind_Elements[axisIdx];
		QString currentParamName = m_splomData->parameterName(currIdx);
		if (switchTO_YRow) 
		{
			currentRect = getPlotRectByIndex(0, axisIdx);
			//top = TextPadding;
		}
		else
		{
			//get rectangles of current plot
			currentRect = getPlotRectByIndex(/*ind_VisX[*/axisIdx/*]*/, 0/*axisOffSet - 1*/);
			top = 0 + TextPadding;
			currentRect.setTop(top);
			currentRect.setHeight(painter.fontMetrics().height());
		}

		if (switchTO_YRow) 
		{
			textwidth = currentRect.height();
			QPoint pos_center;
			pos_center.setX(currentRect.top() + textwidth / 2);
			pos_center.setY(-(TextPadding + textHeight / 2));
			painter.save();
			painter.rotate(-90);
			painter.translate(-pos_center);

			currentRect.setTopLeft(QPoint(-textwidth / 2, -textHeight / 2));
			currentRect.setSize(QSize(textwidth, textHeight));
			painter.drawText(currentRect, Qt::AlignCenter | Qt::AlignTop, currentParamName);
			painter.restore();
		}
		else
		{
			painter.drawText(currentRect, Qt::AlignHCenter, currentParamName);
		}
	}
}

void iAQSplom::drawTicks( QPainter & painter, QList<double> const & ticksX, QList<double> const & ticksY, QList<QString> const & textX, QList<QString> const & textY)
{
	painter.save();
	painter.setPen( m_visiblePlots[1][0]->settings.tickLabelColor );
	QPoint * tOfs = &settings.tickOffsets;
	long tSpc = settings.tickLabelsOffset;
	for( long i = 0; i < ticksY.size(); ++i )
	{
		double t = ticksY[i]; QString text = textY[i];
		painter.drawText( QRectF( 0, t - tOfs->y(), tOfs->x() - tSpc, 2 * tOfs->y() ), Qt::AlignRight | Qt::AlignVCenter, text );
	}
	painter.rotate( -90 );
	for( long i = 0; i < ticksX.size(); ++i )
	{
		double t = ticksX[i]; QString text = textX[i];
		painter.drawText( QRectF( -tOfs->y() + tSpc, t - tOfs->x(), tOfs->y() - tSpc, 2 * tOfs->x() ), Qt::AlignLeft | Qt::AlignVCenter, text );
	}
	painter.restore();
}

void iAQSplom::showSettings()
{
	m_settingsDlg->gbColorCoding->setEnabled(settings.enableColorSettings);
	m_settingsDlg->show();
}

void iAQSplom::changeParamVisibility(QListWidgetItem * item)
{
	setParameterVisibility(item->text(), item->checkState());
}

void iAQSplom::setParameterToColorCode(int paramIndex)
{
	if (paramIndex != -1)
		m_colorLookupParam = paramIndex;
	updateLookupTable();
}

void iAQSplom::updateLookupTable()
{
	double lutRange[2] = { m_settingsDlg->sbMin->value(), m_settingsDlg->sbMax->value() };
	double alpha = static_cast<double>(m_settingsDlg->slPointOpacity->value()) / m_settingsDlg->slPointOpacity->maximum();
	settings.pointColor.setAlpha(alpha*255);
	switch (settings.colorScheme)
	{
		default:
		case AllPointsSame:
		{
			m_lut->setRange(lutRange);
			m_lut->allocate(2);
			m_lut->setColor(0, settings.pointColor);
			m_lut->setColor(1, settings.pointColor);
			break;
		}
		case ByParameter:
			*m_lut.data() = iALUT::Build(lutRange, settings.colorThemeName, 256, alpha);
			// intentional fall-through!
		case Custom:
			m_lut->setOpacity(alpha);
	}
	applyLookupTable();
	emit lookupTableChanged();
}

void iAQSplom::applyLookupTable()
{
	QSignalBlocker colorChoiceBlock(m_settingsDlg->cbColorParameter);
	m_settingsDlg->cbColorParameter->setCurrentIndex(m_colorLookupParam);
	for (auto & row : m_matrix)
		for (auto s : row)
			if (s)
				s->setLookupTable(m_lut, m_colorLookupParam);

	if (m_maximizedPlot)
		m_maximizedPlot->setLookupTable(m_lut, m_colorLookupParam);

	update();
}

void iAQSplom::pointRadiusChanged(int newValue)
{
	settings.pointRadius = newValue / PointRadiusFractions;
	m_settingsDlg->lbPointSizeValue->setText(QString::number(settings.pointRadius));
	for (auto & row : m_matrix)
		for (auto s : row)
			if (s)
				s->setPointRadius(settings.pointRadius);

	if (m_maximizedPlot)
		m_maximizedPlot->setPointRadius(settings.pointRadius);
	update();
}

void iAQSplom::setPointRadius(double radius)
{
	m_settingsDlg->slPointSize->setValue(radius * PointRadiusFractions);
	// signal from slider will take care of applying the new radius value everywhere!
}

void iAQSplom::pointOpacityChanged(int newValue)
{
	float opacity = static_cast<double>(newValue) / m_settingsDlg->slPointOpacity->maximum();
	m_settingsDlg->lbPointOpacityValue->setText(QString::number(opacity));
	updateLookupTable();
}

void iAQSplom::setPointOpacity(double opacity)
{
	assert(0 <= opacity && opacity <= 1);
	m_settingsDlg->slPointOpacity->setValue(opacity*m_settingsDlg->slPointOpacity->maximum());
	// signal from slider will take care of applying the new opacity value everywhere!
}

size_t iAQSplom::colorLookupParam() const
{
	return m_colorLookupParam;
}

QSharedPointer<iALookupTable> iAQSplom::lookupTable() const
{
	return m_lut;
}

iAQSplom::ColorScheme iAQSplom::colorScheme() const
{
	return settings.colorScheme;
}

void iAQSplom::setColorParam(const QString & paramName)
{
	size_t colorLookupParam = m_splomData->paramIndex(paramName);
	if (colorLookupParam == std::numeric_limits<size_t>::max())
		return;
	m_colorLookupParam = colorLookupParam;
	setColorScheme(ByParameter);
}

void iAQSplom::changePointColor()
{
	QColor newColor = QColorDialog::getColor(settings.pointColor, this, "SPM Point color");
	if (newColor.isValid())
		setPointColor(newColor);
}

void iAQSplom::setPointColor(QColor const & newColor)
{
	if (settings.pointColor == newColor)
		return;
	settings.pointColor = newColor;
	m_settingsDlg->pbPointColor->setStyleSheet(QString("background-color:%1").arg(newColor.name()));
	setColorScheme(AllPointsSame);
}

void iAQSplom::colorSchemeChanged(int colorScheme)
{
	if (!settings.enableColorSettings)
	{
		DEBUG_LOG("setColorScheme called despite enableColorSettings being false!");
		return;
	}
	if (colorScheme == Custom)
	{
		QMessageBox::warning(this, "SPM settings", "Custom color scheme can not be used from the settings dialog");
		return;
	}
	setColorScheme(static_cast<ColorScheme>(colorScheme));
}

void iAQSplom::setColorScheme(ColorScheme colorScheme)
{
	settings.colorScheme = colorScheme;
	QSignalBlocker cbColorBlock(m_settingsDlg->cbColorScheme);
	m_settingsDlg->cbColorScheme->setCurrentIndex(colorScheme);
	m_settingsDlg->pbPointColor->setEnabled(colorScheme == AllPointsSame);
	m_settingsDlg->lbPointColor->setEnabled(colorScheme == AllPointsSame);
	m_settingsDlg->cbColorParameter->setEnabled(colorScheme == ByParameter);
	m_settingsDlg->lbColorParameter->setEnabled(colorScheme == ByParameter);
	m_settingsDlg->lbRange->setEnabled(colorScheme == ByParameter);
	m_settingsDlg->lbRangeMin->setEnabled(colorScheme == ByParameter);
	m_settingsDlg->lbRangeMax->setEnabled(colorScheme == ByParameter);
	m_settingsDlg->sbMin->setEnabled(colorScheme == ByParameter);
	m_settingsDlg->sbMax->setEnabled(colorScheme == ByParameter);
	m_settingsDlg->pbRangeFromParameter->setEnabled(colorScheme == ByParameter);
	m_settingsDlg->lbColorTheme->setEnabled(colorScheme == ByParameter);
	m_settingsDlg->cbColorTheme->setEnabled(colorScheme == ByParameter);
	updateLookupTable();
}

void iAQSplom::setColorTheme(QString const & themeName)
{
	settings.colorThemeName = themeName;
	if (m_settingsDlg->cbColorTheme->currentText() != themeName)
	{
		QSignalBlocker sb(m_settingsDlg->cbColorTheme);
		m_settingsDlg->cbColorTheme->setCurrentText(themeName);
	}
	if (settings.colorScheme == ByParameter)
		updateLookupTable();
}

void iAQSplom::rangeFromParameter()
{
	double const * range = m_splomData->paramRange(m_colorLookupParam);
	QSignalBlocker sb(m_settingsDlg->sbMin);
	m_settingsDlg->sbMin->setValue(range[0]);
	m_settingsDlg->sbMax->setValue(range[1]);
}
