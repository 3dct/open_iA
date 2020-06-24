/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
#include "iAStringHelper.h"

#include <vtkLookupTable.h>

#include <QAbstractTextDocumentLayout>
#include <QColorDialog>
#include <QFileDialog>
#include <QListWidgetItem>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPropertyAnimation>
#include <QSettings>
#include <QWheelEvent>
#include <QtGlobal> // for QT_VERSION
#include <QtMath>

namespace
{ // apparently QFontMetric width is not returning the full width of the string - correction constant:
	const int TextPadding = 7;
	const double PointRadiusFractions = 10.0;

	const QString CfgKeyPointRadius("SPM/PointRadius");
	const QString CfgKeyHistogramVisible("SPM/HistogramVisible");
	const QString CfgKeyHistogramBins("SPM/HistogramBins");
	const QString CfgKeySelectionMode("SPM/SelectionMode");
	const QString CfgKeyFlipAxes("SPM/FlipAxes");
	const QString CfgKeyShowColorLegend("SPM/ShowColorLegend");
	const QString CfgKeyQuadraticPlots("SPM/QuadraticPlots");
	const QString CfgKeyShowPCC("SPM/ShowPCC");
	const QString CfgKeyShowSCC("SPM/ShowSCC");
	const QString CfgKeyColorMode("SPM/ColorScheme"); // mismatch between setting name is a legacy issue; don't change otherwise old settings aren't readable anymore...
	const QString CfgKeyColorThemeName("SPM/ColorThemeName");
	const QString CfgKeyColorThemeQualName("SPM/ColorThemeQualName");
	const QString CfgKeyColorParameterMode("SPM/ColorParameterMode");
	const QString CfgKeyPointColor("SPM/PointColor");
	const QString CfgKeyPointOpacity("SPM/PointOpacity");
	const QString CfgKeyColorRangeMode("SPM/ColorRangeMode");
	const QString CfgKeyColorCodingMin("SPM/ColorCodingMin");
	const QString CfgKeyColorCodingMax("SPM/ColorCodingMax");
	const QString CfgKeyColorLookupParam("SPM/ColorLookupParam");
	const QString CfgKeyVisibleParameters("SPM/VisibleParameters");
	const QString CfgKeyMaximizedPlot("SPM/MaximizedPlot");
}

iAQSplom::Settings::Settings() :
	plotsSpacing(7),
	tickLabelsOffset(5),
	maxRectExtraOffset(20),
	tickOffsets(45, 45),
	maximizedLinked(false),
	flipAxes(false),
	popupBorderColor(QColor(180, 180, 180, 220)),
	popupFillColor(QColor(250, 250, 250, 200)),
	popupTextColor(QColor(50, 50, 50)),
	selectionColor(QColor(255, 40, 0, 1)),
	popupTipDim{ 5, 10 },
	popupWidth(180),
	pointRadius(1.0),
	isAnimated(true),
	animDuration(100.0),
	animStart(0.0),
	separationMargin(10),
	histogramBins(10),
	histogramVisible(true),
	selectionMode(iAScatterPlot::Polygon),
	selectionEnabled(true),
	quadraticPlots(false),
	showPCC(false),
	showSCC(false),
	showColorLegend(true),
	colorMode(cmAllPointsSame),
	colorParameterMode(pmContinuous),
	colorRangeMode(rmAutomatic),
	colorThemeName("Diverging blue-gray-red"),
	colorThemeQualName("Brewer Set3 (max. 12)"),
	pointColor(QColor(128, 128, 128)),
	enableColorSettings(false)
{
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
	{
		m_mode = smAllPlots;
	}
	else
	{
		m_mode = smUpperHalf;
	}
	updateVisiblePlots();
}

void iAQSplom::setSelectionColor(QColor color)
{
	settings.selectionColor = color;
	// TODO: remove duplication through helper function which
	// calls some given function on all scatter plots of the SPM!
	for (auto& row : m_matrix)
	{
		for (auto s : row)
		{
			if (s)
			{
				s->setSelectionColor(color);
			}
		}
	}
	if (m_maximizedPlot)
	{
		m_maximizedPlot->setSelectionColor(color);
	}
}

void iAQSplom::setSelectionMode(int mode)
{
	if (m_maximizedPlot)
	{
		m_maximizedPlot->settings.selectionMode = static_cast<iAScatterPlot::SelectionMode>(mode);
	}
	QSignalBlocker sb(m_settingsDlg->cbSelectionMode);
	m_settingsDlg->cbSelectionMode->setCurrentIndex(mode);
	settings.selectionMode = mode;
}

void iAQSplom::enableSelection(bool enabled)
{
	if (m_maximizedPlot)
	{
		m_maximizedPlot->settings.selectionEnabled = enabled;
	}
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

iAQSplom::iAQSplom(QWidget * parent , Qt::WindowFlags f):
	iAQGLWidget(parent),
	settings(),
	m_lut(new iALookupTable()),
	m_colorLookupParam(0),
	m_activePlot(nullptr),
	m_mode(smAllPlots),
	m_splomData(new iASPLOMData()),
	m_previewPlot(nullptr),
	m_maximizedPlot(nullptr),
	m_animIn(1.0),
	m_animOut(0.0),
	m_animationIn(new QPropertyAnimation(this, "m_animIn")),
	m_animationOut(new QPropertyAnimation(this, "m_animOut")),
	m_popupHeight(0),
	m_separationIdx(-1),
	m_bgColorTheme(iAColorThemeManager::instance().theme("White")),
	m_contextMenu(new QMenu(this)),
	m_settingsDlg(new iASPMSettings(this, f))
{
	setWindowFlags(f);
	setMouseTracking( true );
	setFocusPolicy( Qt::StrongFocus );
	setBackgroundRole(QPalette::Base);
	setAutoFillBackground(true);
	iAQGLFormat format;
	format.setSamples(4);
	setFormat(format);
	m_animationIn->setDuration( settings.animDuration );
	m_animationOut->setDuration( settings.animDuration );

	// set up context menu:
	showHistogramAction = new QAction(tr("Show Histograms"), this);
	showHistogramAction->setCheckable(true);
	showHistogramAction->setChecked(settings.histogramVisible);
	flipAxesAction = new QAction(tr("Flip Axes of max. Plot"), this);
	flipAxesAction->setCheckable(true);
	flipAxesAction->setChecked(settings.flipAxes);
	showColorLegendAction = new QAction(tr("Show Color Legend"), this);
	showColorLegendAction->setCheckable(true);
	showColorLegendAction->setChecked(settings.showColorLegend);
	quadraticPlotsAction = new QAction(tr("Quadratic Plots"), this);
	quadraticPlotsAction->setCheckable(true);
	quadraticPlotsAction->setChecked(settings.quadraticPlots);
	showPCCAction = new QAction(tr("Show Pearsons's Correlation Coefficient"), this);
	showPCCAction->setCheckable(true);
	showPCCAction->setChecked(settings.quadraticPlots);
	showSCCAction = new QAction(tr("Show Spearman's Correlation Coefficient"), this);
	showSCCAction->setCheckable(true);
	showSCCAction->setChecked(settings.quadraticPlots);
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
	addContextMenuAction(quadraticPlotsAction);
	addContextMenuAction(showPCCAction);
	addContextMenuAction(showSCCAction);
	addContextMenuAction(selectionModeRectangleAction);
	addContextMenuAction(selectionModePolygonAction);
	addContextMenuAction(showHistogramAction);
	addContextMenuAction(flipAxesAction);
	addContextMenuAction(showColorLegendAction);
	addContextMenuAction(showSettingsAction);
	connect(showHistogramAction, &QAction::toggled, this, &iAQSplom::setHistogramVisible);
	connect(showColorLegendAction, &QAction::toggled, this, &iAQSplom::setShowColorLegend);
	connect(flipAxesAction, &QAction::toggled, this, &iAQSplom::setFlipAxes);
	connect(quadraticPlotsAction, &QAction::toggled, this, &iAQSplom::setQuadraticPlots);
	connect(showPCCAction, &QAction::toggled, this, &iAQSplom::setShowPCC);
	connect(showSCCAction, &QAction::toggled, this, &iAQSplom::setShowSCC);
	connect(selectionModePolygonAction, &QAction::toggled, this, &iAQSplom::selectionModePolygon);
	connect(selectionModeRectangleAction, &QAction::toggled, this, &iAQSplom::selectionModeRectangle);
	connect(showSettingsAction, &QAction::triggered, this, &iAQSplom::showSettings);
	connect(m_settingsDlg->parametersList, &QListWidget::itemChanged, this, &iAQSplom::changeParamVisibility);
	connect(m_settingsDlg->cbColorParameter, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &iAQSplom::setParameterToColorCode);
	connect(m_settingsDlg->cbColorMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &iAQSplom::colorModeChanged);
	connect(m_settingsDlg->sbMin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &iAQSplom::updateLookupTable);
	connect(m_settingsDlg->sbMax, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &iAQSplom::updateLookupTable);
	connect(m_settingsDlg->slPointOpacity, &QSlider::valueChanged, this, &iAQSplom::pointOpacityChanged);
	connect(m_settingsDlg->slPointSize, &QSlider::valueChanged, this, &iAQSplom::pointRadiusChanged);
	connect(m_settingsDlg->pbPointColor, &QPushButton::clicked, this, &iAQSplom::changePointColor);
	connect(m_settingsDlg->pbRangeFromParameter, &QPushButton::clicked, this, &iAQSplom::rangeFromParameter);
	connect(m_settingsDlg->pbSaveSettings, &QPushButton::clicked, this, &iAQSplom::saveSettingsSlot);
	connect(m_settingsDlg->pbLoadSettings, &QPushButton::clicked, this, &iAQSplom::loadSettingsSlot);
	connect(m_settingsDlg->cbSelectionMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &iAQSplom::setSelectionMode);
	connect(m_settingsDlg->cbQuadraticPlots, &QCheckBox::toggled, this, &iAQSplom::setQuadraticPlots);
	connect(m_settingsDlg->cbShowPCC, &QCheckBox::toggled, this, &iAQSplom::setShowPCC);
	connect(m_settingsDlg->cbShowSCC, &QCheckBox::toggled, this, &iAQSplom::setShowSCC);
	connect(m_settingsDlg->cbShowHistograms, &QCheckBox::toggled, this, &iAQSplom::setHistogramVisible);
	connect(m_settingsDlg->sbHistogramBins, QOverload<int>::of(&QSpinBox::valueChanged), this, &iAQSplom::setHistogramBins);
	connect(m_settingsDlg->cbFlipAxes, &QCheckBox::toggled, this, &iAQSplom::setFlipAxes);
	connect(m_settingsDlg->cbShowColorLegend, &QCheckBox::toggled, this, &iAQSplom::setShowColorLegend);
	connect(m_settingsDlg->rbContinuous, &QRadioButton::toggled, this, &iAQSplom::setContinousParamMode);
	connect(m_settingsDlg->rbQualitative, &QRadioButton::toggled, this, &iAQSplom::setQualitativeParamMode);
	connect(m_settingsDlg->cbColorRangeMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &iAQSplom::colorRangeModeChanged);
	m_settingsDlg->cbColorTheme->addItems(iALUT::GetColorMapNames());
	m_settingsDlg->cbColorThemeQual->addItems(iAColorThemeManager::instance().availableThemes());
	m_settingsDlg->cbColorThemeQual->setCurrentIndex(1); // to avoid "Black" default theme
	connect(m_settingsDlg->cbColorTheme, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &iAQSplom::setColorThemeFromComboBox);
	connect(m_settingsDlg->cbColorThemeQual, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &iAQSplom::setColorThemeQual);
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
		{
			hist_InputValues.push_back(m_splomData->paramData(paramIndex)[i]);
		}
	}
	if (m_histograms[paramIndex]->plots().size() > 0)
	{
		m_histograms[paramIndex]->removePlot(m_histograms[paramIndex]->plots()[0]);
	}

	auto histogramData = iAHistogramData::create(hist_InputValues, settings.histogramBins);
	auto histogramPlot = QSharedPointer<iABarGraphPlot>(new iABarGraphPlot(histogramData, QColor(70, 70, 70, 255)));
	m_histograms[paramIndex]->addPlot(histogramPlot);
	m_histograms[paramIndex]->update();
}

void iAQSplom::updateHistograms()
{
	if (!settings.histogramVisible)
	{
		return;
	}
	for (size_t y = 0; y < m_splomData->numParams(); ++y)
	{
		if (m_paramVisibility[y])
		{
			updateHistogram(y);
		}
	}
}

void iAQSplom::addFilter(size_t paramIndex, double value)
{
	m_splomData->addFilter(paramIndex, value);
	updateFilter();
}

void iAQSplom::removeFilter(size_t paramIndex, double value)
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
	for (auto& row : m_visiblePlots)
	{
		for (iAScatterPlot* s : row)
		{
			if (s)
			{
				s->updatePoints();
			}
		}
	}
	if (m_maximizedPlot)
	{
		m_maximizedPlot->updatePoints();
	}
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

void iAQSplom::setData( QSharedPointer<iASPLOMData> data, std::vector<char> const & visibility )
{
	if (data->numPoints() > std::numeric_limits<int>::max())
	{
		DEBUG_LOG(QString("Number of points (%1) larger than currently supported (%2).")
			.arg(data->numPoints())
			.arg(std::numeric_limits<int>::max()));
	}
	if (data->numParams() > std::numeric_limits<int>::max())
	{
		DEBUG_LOG(QString("Number of parameters (%1) larger than currently supported (%2).")
			.arg(data->numParams())
			.arg(std::numeric_limits<int>::max()));
	}
	m_splomData = data;
	dataChanged(visibility);
}

QSharedPointer<iASPLOMData> iAQSplom::data()
{
	return m_splomData;
}

void iAQSplom::createScatterPlot(size_t y, size_t x, bool initial)
{
	if (!m_paramVisibility[y] || !m_paramVisibility[x] || (m_mode == smUpperHalf && x >= y)
		|| (m_matrix.size() > y&& m_matrix[y][x]))
	{
		return;
	}
	iAScatterPlot * s = new iAScatterPlot(this, this);
	s->settings.backgroundColor = settings.backgroundColor;
	connect(s, &iAScatterPlot::transformModified, this, &iAQSplom::transformUpdated);
	connect(s, &iAScatterPlot::currentPointModified, this, &iAQSplom::currentPointUpdated);
	s->setData(x, y, m_splomData);
	s->setSelectionColor(settings.selectionColor);
	s->setPointRadius(settings.pointRadius);
	s->settings.showPCC = settings.showPCC;
	s->settings.showSCC = settings.showSCC;
	if (!initial)
	{
		s->setLookupTable(m_lut, m_colorLookupParam);
		QPointF offset;
		double scale = 1.0;
		size_t curPoint = iAScatterPlot::NoPointIndex;
		for (size_t p = 0; p < m_splomData->numParams(); ++p)
		{
			if (m_matrix[p][x])
			{
				offset.setX(m_matrix[p][x]->getOffset().x());
				scale = m_matrix[p][x]->getScale();
				curPoint = m_matrix[p][x]->getCurrentPoint();
				break;
			}
		}
		for (size_t p = 0; p < m_splomData->numParams(); ++p)
		{
			if (m_matrix[y][p])
			{
				offset.setY(m_matrix[y][p]->getOffset().y());
				scale = m_matrix[y][p]->getScale();
				curPoint = m_matrix[y][p]->getCurrentPoint();
				break;
			}
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
	size_t numParams = m_splomData->numParams();

	QSignalBlocker blockListSignals(m_settingsDlg->parametersList);
	QSignalBlocker colorSignals(m_settingsDlg->cbColorParameter);
	m_settingsDlg->parametersList->clear();
	m_settingsDlg->cbColorParameter->clear();
	m_matrix.resize(numParams);
	for(size_t y = 0; y < numParams; ++y )
	{
		m_matrix[y].resize(numParams, nullptr);
		for (size_t x = 0; x < numParams; ++x)
		{
			createScatterPlot(y, x, true);
		}

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
	{
		return;
	}
	m_lut->copyFromVTK( lut );
	m_colorLookupParam = colorLookupCol;
	setColorMode(cmCustom);
}

void iAQSplom::setLookupTable( iALookupTable &lut, size_t columnIndex)
{
	if (columnIndex >= m_splomData->numParams())
	{
		return;
	}
	*m_lut = lut;
	m_colorLookupParam = columnIndex;
	setColorMode(cmCustom);
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
	if (paramIndex >= m_paramVisibility.size() || static_cast<bool>(m_paramVisibility[paramIndex]) == isVisible)
	{
		return;
	}
	m_paramVisibility[paramIndex] = isVisible;
	if (settings.histogramVisible)
	{
		updateHistogram(paramIndex);
	}
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
	{
		for (size_t x = 0; x < m_splomData->numParams(); ++x)
		{
			createScatterPlot(y, x, false);
		}
	}

	updateVisiblePlots();
	updateHistograms();
	update();
}

void iAQSplom::setParameterInverted( size_t paramIndex, bool isInverted )
{
	m_splomData->setInverted(paramIndex, isInverted);
	size_t numParams = m_splomData->numParams();
	for (size_t row = 0; row < numParams; ++row)
	{
		auto s = m_matrix[row][paramIndex];
		if (s)
		{
			s->updatePoints();
		}
	}
	for (size_t col = 0; col < numParams; ++col)
	{  // avoid double updated of row==col plot
		auto s = m_matrix[paramIndex][col];
		if (s && col != paramIndex)
		{
			s->updatePoints();
		}
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
	for (size_t curIdx = 0; curIdx < m_splomData->numPoints(); ++curIdx)
	{
		if (!m_splomData->matchesFilter(curIdx))
		{
			continue;
		}
		if (curSelIdx >= m_selInds.size())
		{
			break;
		}
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
		{
			continue;
		}
		if (curSelIdx >= sortedFilteredSelInds.size())
		{
			break;
		}
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
		inds_out[0] = static_cast<int>(m_activePlot->getIndices()[0]);
		inds_out[1] = static_cast<int>(m_activePlot->getIndices()[1]);
	}
}

int iAQSplom::getVisibleParametersCount() const
{
	return static_cast<int>(m_visiblePlots.size());
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
	for (auto histo : m_histograms)
	{
		delete histo;
	}
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
	{
		return;
	}

	if (m_maximizedPlot && settings.maximizedLinked)
	{
		m_maximizedPlot->setTransform(sender->getScale(),
			QPointF(sender->getOffset().x() / sender->getRect().width() * m_maximizedPlot->getRect().width(),
					sender->getOffset().y() / sender->getRect().height() * m_maximizedPlot->getRect().height()));
		if (sender == m_maximizedPlot)
		{
			deltaOffset = QPointF(deltaOffset.x() / m_maximizedPlot->getRect().width() * m_previewPlot->getRect().width(),
				deltaOffset.y() / m_maximizedPlot->getRect().height() * m_previewPlot->getRect().height());
		}
	}

	const size_t* ind = sender->getIndices();
	for (auto& row : m_matrix)
	{
		for (auto s : row)
		{
			if (s && s != sender)
			{
				if (s->getIndices()[0] == ind[0])
				{
					s->setTransformDelta(scale, QPointF(deltaOffset.x(), 0.0f));
				}
				else if (s->getIndices()[1] == ind[1])
				{
					s->setTransformDelta(scale, QPointF(0.0f, deltaOffset.y()));
				}
				else
				{
					s->setTransformDelta(scale, QPointF(0.0f, 0.0f));
				}
			}
		}
	}

	update();
}

void iAQSplom::currentPointUpdated( size_t index )
{
	for (auto& row : m_matrix)
	{
		for (auto s : row)
		{
			if (s && s != QObject::sender())
			{
				s->setCurrentPoint(index);
			}
		}
	}
	if( m_maximizedPlot && ( m_maximizedPlot != QObject::sender() ) )
	{
		m_maximizedPlot->setCurrentPoint( index );
	}

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
	{
		emit currentPointModified( index );
	}
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
	{
		return;
	}
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

void iAQSplom::setShowColorLegend(bool visible)
{
	settings.showColorLegend = visible;
	QSignalBlocker sb(m_settingsDlg->cbShowColorLegend);
	m_settingsDlg->cbShowColorLegend->setChecked(visible);
	update();
}

void iAQSplom::setFlipAxes(bool flip)
{
	settings.flipAxes = flip;
	QSignalBlocker sb(m_settingsDlg->cbFlipAxes);
	m_settingsDlg->cbFlipAxes->setChecked(flip);
	if (m_maximizedPlot)
	{
		auto curSelected = m_previewPlot;
		removeMaximizedPlot();
		maximizeSelectedPlot(curSelected);
	}
}

void iAQSplom::setHistogramBins(int bins)
{
	settings.histogramBins = bins;
	if (!settings.histogramVisible)
	{
		return;
	}
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
	QSignalBlocker sb(m_settingsDlg->cbShowPCC);
	m_settingsDlg->cbShowPCC->setChecked(showPCC);
	for (auto row : m_matrix)
	{
		for (auto s : row)
		{
			if (s)
			{
				s->settings.showPCC = showPCC;
			}
		}
	}
	if (m_maximizedPlot)
	{
		m_maximizedPlot->settings.showPCC = showPCC;
	}
	update();
}

void iAQSplom::setShowSCC(bool showSCC)
{
	settings.showSCC = showSCC;
	QSignalBlocker sb(m_settingsDlg->cbShowSCC);
	m_settingsDlg->cbShowSCC->setChecked(showSCC);
	for (auto row : m_matrix)
	{
		for (auto s : row)
		{
			if (s)
			{
				s->settings.showSCC = showSCC;
			}
		}
	}
	if (m_maximizedPlot)
	{
		m_maximizedPlot->settings.showSCC = showSCC;
	}
	update();
}

void iAQSplom::contextMenuEvent(QContextMenuEvent * event)
{
	showHistogramAction->setChecked(settings.histogramVisible);
	flipAxesAction->setChecked(settings.flipAxes);
	quadraticPlotsAction->setChecked(settings.quadraticPlots);
	showColorLegendAction->setChecked(settings.showColorLegend);
	showColorLegendAction->setVisible(settings.enableColorSettings);
	{
		QSignalBlocker sb1(selectionModeRectangleAction), sb2(selectionModePolygonAction);
		selectionModeRectangleAction->setChecked(settings.selectionMode == iAScatterPlot::Rectangle);
		selectionModePolygonAction->setChecked(settings.selectionMode == iAScatterPlot::Polygon);
	}
	showPCCAction->setChecked(settings.showPCC);
	showSCCAction->setChecked(settings.showSCC);
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
	{
		return;
	}

	if (m_previewPlot)
	{
		m_previewPlot->setPreviewState(false);
	}

	selectedPlot->setPreviewState(true);
	m_previewPlot = selectedPlot;
	if (m_mode == smAllPlots)
	{	// hide lower triangle
		m_mode = smUpperHalf;
		for (int y = 0; y < getVisibleParametersCount(); ++y)
		{
			for (int x = 0; x < getVisibleParametersCount(); ++x)
			{
				if (x >= y)
				{
					m_visiblePlots[y][x] = nullptr;
				}
			}
		}
	}

	delete m_maximizedPlot;
	m_maximizedPlot = new iAScatterPlot(this, this, 11, true);
	m_maximizedPlot->settings.backgroundColor = settings.backgroundColor;
	connect(m_maximizedPlot, &iAScatterPlot::selectionModified, this, &iAQSplom::selectionUpdated);
	connect(m_maximizedPlot, &iAScatterPlot::currentPointModified, this, &iAQSplom::currentPointUpdated);

	if (settings.maximizedLinked)
	{
		connect(m_maximizedPlot, &iAScatterPlot::transformModified, this, &iAQSplom::transformUpdated);
	}

	const size_t * plotInds = selectedPlot->getIndices();
	size_t actualPlotInds[2] = {
		plotInds[(settings.flipAxes) ? 1 : 0],
		plotInds[(settings.flipAxes) ? 0 : 1]
	};
	m_maximizedPlot->setData(actualPlotInds[0], actualPlotInds[1], m_splomData);
	m_maximizedPlot->setLookupTable(m_lut, m_colorLookupParam);
	m_maximizedPlot->setSelectionColor(settings.selectionColor);
	m_maximizedPlot->setPointRadius(settings.pointRadius);
	m_maximizedPlot->settings.selectionMode = static_cast<iAScatterPlot::SelectionMode>(settings.selectionMode);
	m_maximizedPlot->settings.selectionEnabled = settings.selectionEnabled;
	m_maximizedPlot->settings.showPCC = settings.showPCC;
	m_maximizedPlot->settings.showSCC = settings.showSCC;
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

void iAQSplom::paintEvent(QPaintEvent * /*event*/)
{
	QPainter painter( this );
	painter.setRenderHint(QPainter::Antialiasing);
	painter.beginNativePainting();
	QColor bg(settings.backgroundColor);
	if (!bg.isValid())
	{
		bg = QWidget::palette().color(QWidget::backgroundRole());
	}
	glClearColor(bg.redF(), bg.greenF(), bg.blueF(), bg.alphaF());
	glClear(GL_COLOR_BUFFER_BIT);
	painter.endNativePainting();
	painter.setPen(QWidget::palette().color(QPalette::Text));
	if (m_visiblePlots.size() < 2)
	{
		painter.drawText(geometry(), Qt::AlignCenter | Qt::AlignVCenter, "Too few parameters selected!");
		return;
	}
	QFontMetrics fm = painter.fontMetrics();

	// collect tick labels text and positions:
	QList<double> ticksX, ticksY; QList<QString> textX, textY;
	for (size_t i = 0; i < m_visiblePlots.size() -1; ++i)
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
		QColor c1(m_bgColorTheme->color(0)); c1.setAlpha(64);
		painter.fillRect(r1, QBrush(c1));
		if (!m_maximizedPlot)
		{
			QColor c2(m_bgColorTheme->color(1)); c2.setAlpha(64);
			QColor c3(m_bgColorTheme->color(3)); c3.setAlpha(64);
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
	{
		return;
	}

	//draw elements
	drawVisibleParameters(painter);

	drawTicks( painter, ticksX, ticksY, textX, textY );
	for( auto & row: m_visiblePlots )
	{
		for( iAScatterPlot * s: row )
		{
			if( s )
			{
				s->paintOnParent( painter );
			}
		}
	}
	if( m_maximizedPlot )
	{
		m_maximizedPlot->paintOnParent( painter );
	}
	drawPopup( painter );

	if (!settings.enableColorSettings || m_mode == smAllPlots || !settings.showColorLegend)
	{
		return;
	}
	// Draw scalar bar:
	// maybe reuse code from iALinearColorGradientBar (DynamicVolumeLines)
	QPoint topLeft = getMaxRect().topLeft();
	int barWidth = clamp(5, 10, m_scatPlotSize.x() / 10);
	topLeft += QPoint(- (barWidth + 3*settings.plotsSpacing + settings.tickOffsets.x()),
		settings.plotsSpacing
		+ m_scatPlotSize.y() / ((((getVisibleParametersCount() + (settings.histogramVisible ? 1 : 0)) % 2) == 1) ? 2 : 1)
	);

	double minVal = settings.colorMode == cmAllPointsSame ? 0 : m_lut->getRange()[0];
	double maxVal = settings.colorMode == cmAllPointsSame ? 0 : m_lut->getRange()[1];
	QRect colorBarRect(topLeft.x(), topLeft.y(),
		barWidth, height() - topLeft.y() - settings.plotsSpacing);
	QLinearGradient grad(topLeft.x(), topLeft.y(), topLeft.x(), topLeft.y()+colorBarRect.height() );
	QMap<double, QColor>::iterator it;
	for (size_t i = 0; i < m_lut->numberOfValues(); ++i)
	{
		double rgba[4];
		m_lut->getTableValue(i, rgba);
		QColor color(rgba[0] * 255, rgba[1] * 255, rgba[2] * 255, rgba[3] * 255);
		double key = 1 - (static_cast<double>(i) / (m_lut->numberOfValues()-1) );
		grad.setColorAt(key, color);
	}
	painter.fillRect(colorBarRect, grad);
	painter.drawRect(colorBarRect);
	QString minStr = dblToStringWithUnits(minVal);
	QString maxStr = dblToStringWithUnits(maxVal);
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
	int textWidth = std::max(fm.horizontalAdvance(minStr), fm.horizontalAdvance(maxStr));
#else
	int textWidth = std::max(fm.width(minStr), fm.width(maxStr));
#endif
	// Draw color bar / name of parameter used for coloring
	int colorBarTextX = topLeft.x() - (textWidth + settings.plotsSpacing);
	painter.drawText(colorBarTextX, topLeft.y() + fm.height(), maxStr);
	painter.drawText(colorBarTextX, height() - settings.plotsSpacing, minStr);
	int textHeight = height() - (topLeft.y() + 2 * fm.height() + 2*settings.plotsSpacing);
	textWidth = std::max(textWidth, fm.height()); // guarantee that label has at least text height
	QString scalarBarCaption;
	switch (settings.colorMode)
	{
	case cmAllPointsSame: scalarBarCaption = "Uniform"; break;
	case cmCustom:        // intentional fall-through:
	case cmByParameter  : scalarBarCaption = m_splomData->parameterName(m_colorLookupParam); break;
	default:              scalarBarCaption = "Unknown"; break;
	}
	painter.save();
	painter.translate(colorBarTextX - settings.plotsSpacing, topLeft.y() + fm.height() + settings.plotsSpacing + textHeight);
	painter.rotate(-90);
	painter.drawText(QRect(0, 0, textHeight, textWidth), Qt::AlignHCenter | Qt::AlignBottom, scalarBarCaption);
	painter.restore();
}

bool iAQSplom::drawPopup( QPainter& painter )
{
	if( !m_activePlot )
	{
		return false;
	}
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
	{
		anim = m_animIn;
	}
	const size_t * pInds = m_activePlot->getIndices();

	painter.save();
	QPointF popupPos = m_activePlot->getPointPosition( curInd );
	double pPM = m_activePlot->settings.pickedPointMagnification;
	double ptRad = m_activePlot->getPointRadius();
	popupPos.setY( popupPos.y() -  pPM * ptRad ); //popupPos.setY( popupPos.y() - ( 1 + ( pPM - 1 )*m_anim ) * ptRad );
	QColor col = settings.popupFillColor; col.setAlpha( col.alpha()* anim );
	painter.setBrush( col );
	col = settings.popupBorderColor; col.setAlpha( col.alpha()* anim );
	painter.setPen( col );
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
	{
		return nullptr;
	}
	QPoint offsetPos = pos - settings.tickOffsets;
	QPoint grid( m_scatPlotSize.x() + settings.plotsSpacing, m_scatPlotSize.y() + settings.plotsSpacing );
	if (grid.x() == 0 || grid.y() == 0)
	{
		return nullptr;	// to avoid division by 0
	}
	int ind[2] = { offsetPos.x() / grid.x(), offsetPos.y() / grid.y() };
	//boundary checks
	for( int i = 0; i < 2; ++i )
	{
		ind[i] = clamp(0, getVisibleParametersCount() - 1, ind[i]);
	}
	if (ind[0] > m_separationIdx)
	{
		offsetPos.setX(offsetPos.x() - settings.separationMargin);
	}
	if (ind[1] > m_separationIdx)
	{
		offsetPos.setY(offsetPos.y() - settings.separationMargin);
	}
	//are we between plots due to the spacing?
	bool isBetween =
		(offsetPos.x() - ind[0] * grid.x() >= m_scatPlotSize.x()) ||
		(offsetPos.y() - ind[1] * grid.y() >= m_scatPlotSize.y());
	ind[1] = invert( ind[1] );	// indexing is bottom to top -> invert index Y
	//get the resulting plot
	iAScatterPlot * s = isBetween ? 0 : m_visiblePlots[ind[1]][ind[0]];
	//check if we hit the maximized plot if necessary
	if( ( smUpperHalf == m_mode ) && !s )
	{
		if( m_maximizedPlot )
		{
			QRect r = m_maximizedPlot->getRect();
			if( r.contains( pos ) )
			{
				s = m_maximizedPlot;
			}
		}
	}
	return s;
}

void iAQSplom::resizeEvent( QResizeEvent * event )
{
	updateSPLOMLayout();
	iAQGLWidget::resizeEvent( event );
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
		{
			m_scatPlotSize.setY(m_scatPlotSize.x());
		}
		else
		{
			m_scatPlotSize.setX(m_scatPlotSize.y());
		}
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

QRect iAQSplom::getMaxRect()
{
	long visParamCnt = getVisibleParametersCount();
	QRect topLeftPlot = getPlotRectByIndex(0, visParamCnt - 1);
	QRect bottomRightPlot = getPlotRectByIndex(visParamCnt - 1, 0);
	// default top left for max plot is in the middle of the chart area:
	QPoint topLeft(topLeftPlot.left() + (bottomRightPlot.right() - topLeftPlot.left() + settings.plotsSpacing) / 2,
		topLeftPlot.top() + (bottomRightPlot.bottom() - topLeftPlot.top() + settings.plotsSpacing) / 2);
	// make sure there is enough space for the labels:
	double xofs = std::max(0, settings.tickOffsets.x() - ((visParamCnt % 2) ? m_scatPlotSize.x() / 2 : m_scatPlotSize.x()));
	double yofs = std::max(0, settings.tickOffsets.y() - ((visParamCnt % 2) ? m_scatPlotSize.y() / 2 : m_scatPlotSize.y()));
	topLeft += QPoint(xofs, yofs);
	if (settings.histogramVisible)
	{
		topLeft += QPoint(m_scatPlotSize.x() / 2, m_scatPlotSize.y() / 2);
	}
	return QRect(QRect(topLeft, bottomRightPlot.bottomRight()));
}

void iAQSplom::updateMaxPlotRect()
{
	if( !m_maximizedPlot )
	{
		return;
	}
	m_maximizedPlot->setRect( getMaxRect() );
}

void iAQSplom::updateSPLOMLayout()
{
	int visParamCnt = getVisibleParametersCount();
	if( !visParamCnt )
	{
		return;
	}
	updatePlotGridParams();
	for( int yind = 0; yind < visParamCnt; ++yind )
	{
		auto & row = m_visiblePlots[yind];
		for( int xind = 0; xind < static_cast<int>(row.size()); ++xind )
		{   // cast above is guaranteed to work because we checked numPoints/numParams to be smaller than int max in setData!
			iAScatterPlot * s = row[xind];
			if( s )
			{
				s->setRect( getPlotRectByIndex( xind, yind ) );
			}
		}
		QRect rect = getPlotRectByIndex(yind, yind);
		if (settings.histogramVisible)
		{
			m_histograms[ m_visibleIndices[yind] ]->setGeometry(rect);
		}
	}
	updateMaxPlotRect();
}

void iAQSplom::updateVisiblePlots()
{
	if (!m_splomData)
	{
		return;
	}
	m_visibleIndices.clear();
	removeMaxPlotIfHidden();
	m_visiblePlots.clear();
	QSignalBlocker listBlocker(m_settingsDlg->parametersList);
	for( size_t y = 0; y < m_splomData->numParams(); ++y )
	{
		m_histograms[y]->setVisible(settings.histogramVisible && m_paramVisibility[y]);
		m_settingsDlg->parametersList->item(static_cast<int>(y))->setCheckState(m_paramVisibility[y] ? Qt::Checked : Qt::Unchecked);

		if( !m_paramVisibility[y] )
		{
			continue;
		}

		std::vector<iAScatterPlot*> row;
		for( size_t x = 0; x < m_splomData->numParams(); ++x )
		{
			if( !m_paramVisibility[x] )
			{
				continue;
			}
			iAScatterPlot * plot = (m_mode == smUpperHalf && x >= y) ? nullptr : m_matrix[y][x];
			row.push_back( plot );
		}
		m_visiblePlots.push_back( row );
		m_visibleIndices.push_back(y);
	}
	updateSPLOMLayout();
}

void iAQSplom::removeMaxPlotIfHidden()
{
	if (m_maximizedPlot)
	{
		const size_t* inds = m_maximizedPlot->getIndices();
		if (!m_paramVisibility[inds[0]] || !m_paramVisibility[inds[1]] || (getVisibleParametersCount() <= 1))
		{
			removeMaximizedPlot();
		}
	}
}

void iAQSplom::resetTransform()
{
	for (auto& row : m_matrix)
	{
		for (auto s : row)
		{
			if (s)
			{
				s->setTransform(1.0, QPointF(0.0f, 0.0f));
			}
		}
	}

	if (m_maximizedPlot)
	{
		m_maximizedPlot->setTransform(1.0, QPointF(0.0f, 0.0f));
	}
	update();
}

void iAQSplom::wheelEvent( QWheelEvent * event)
{
#if QT_VERSION < QT_VERSION_CHECK(5,15,0)
	iAScatterPlot * s = getScatterplotAt( event->pos() );
#else
	iAScatterPlot* s = getScatterplotAt(event->position().toPoint());
#endif
	if( s )
	{
		s->SPLOMWheelEvent( event );
		if( !event->angleDelta().isNull() )
		{
			update();
		}
	}
	event->accept();
}

void iAQSplom::mousePressEvent( QMouseEvent * event )
{
	setContextMenuPolicy(Qt::DefaultContextMenu);
	if (m_activePlot)
	{
		m_activePlot->SPLOMMousePressEvent(event);
	}
}

void iAQSplom::mouseReleaseEvent( QMouseEvent * event )
{
	if (m_activePlot)
	{
		m_activePlot->SPLOMMouseReleaseEvent(event);
	}
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
	{
		changeActivePlot(s);
	}
	if (s)
	{
		s->SPLOMMouseMoveEvent(event);
	}
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
	{
		return;
	}
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
		if (m_activePlot)
		{
			m_activePlot->leave();
		}
		if (s)
		{
			s->enter();
		}
		m_activePlot = s;
		update();
	}
}

int iAQSplom::getMaxTickLabelWidth(QList<QString> const & textX, QFontMetrics & fm) const
{
	int maxLength = 0;
	for (long i = 0; i < textX.size(); ++i)
	{
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
		maxLength = std::max(fm.horizontalAdvance(textX[i]), maxLength);
#else
		maxLength = std::max(fm.width(textX[i]), maxLength);
#endif
	}
	return maxLength+2*TextPadding + fm.height() ;
}

void iAQSplom::drawVisibleParameters(QPainter &painter)
{
	//getting x positions
	drawPlotLabels(painter, false);
	drawPlotLabels(painter, true);
}

void iAQSplom::drawPlotLabels(QPainter & painter, bool switchTO_YRow)
{
	QRect currentRect;
	int textHeight = painter.fontMetrics().height();
	int loopLength = static_cast<int>(m_visibleIndices.size());

	for (int axisIdx = 0; axisIdx < loopLength; axisIdx++)
	{
		if (!settings.histogramVisible &&
			((!switchTO_YRow && axisIdx == loopLength - 1) ||
			(  switchTO_YRow && axisIdx == 0)))
		{
			continue;
		}
		QString currentParamName = m_splomData->parameterName(m_visibleIndices[axisIdx]);
		if (switchTO_YRow)
		{
			currentRect = getPlotRectByIndex(0, axisIdx);
			//top = TextPadding;
			int textwidth = currentRect.height();
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
			//get rectangles of current plot
			currentRect = getPlotRectByIndex(/*ind_VisX[*/axisIdx/*]*/, 0/*axisOffSet - 1*/);
			currentRect.setTop(TextPadding);
			currentRect.setHeight(painter.fontMetrics().height());
			painter.drawText(currentRect, Qt::AlignHCenter, currentParamName);
		}
	}
}

void iAQSplom::drawTicks( QPainter & painter, QList<double> const & ticksX, QList<double> const & ticksY, QList<QString> const & textX, QList<QString> const & textY)
{
	painter.save();
	//painter.setPen( m_visiblePlots[1][0]->settings.tickLabelColor );
	painter.setPen(QWidget::palette().color(QPalette::Text));
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
	m_settingsDlg->gbColorCoding->setVisible(settings.enableColorSettings);
	m_settingsDlg->cbShowColorLegend->setVisible(settings.enableColorSettings);
	m_settingsDlg->show();
}

void iAQSplom::changeParamVisibility(QListWidgetItem * item)
{
	setParameterVisibility(item->text(), item->checkState());
}

void iAQSplom::setParameterToColorCode(int paramIndex)
{
	size_t unsignedParamIndex = static_cast<size_t>(paramIndex);
	if (paramIndex < 0 || unsignedParamIndex >= m_splomData->numParams())
	{
		DEBUG_LOG(QString("setParameterToColorCode: Invalid paramIndex (%1) given!").arg(paramIndex));
		return;
	}
	m_colorLookupParam = unsignedParamIndex;
	if (settings.colorRangeMode == rmAutomatic)
	{
		rangeFromParameter();
	}
	updateLookupTable();
}

void iAQSplom::updateLookupTable()
{
	if (m_splomData->numParams() == 0)
	{
		return;
	}
	double lutRange[2] = { m_settingsDlg->sbMin->value(), m_settingsDlg->sbMax->value() };
	double alpha = static_cast<double>(m_settingsDlg->slPointOpacity->value()) / m_settingsDlg->slPointOpacity->maximum();
	settings.pointColor.setAlpha(alpha*255);
	switch (settings.colorMode)
	{
		default:
		case cmAllPointsSame:
		{
			m_lut->setRange(lutRange);
			m_lut->allocate(2);
			m_lut->setColor(0, settings.pointColor);
			m_lut->setColor(1, settings.pointColor);
			break;
		}
		case cmByParameter:
			if (m_settingsDlg->rbContinuous->isChecked())
			{
				*m_lut.data() = iALUT::Build(lutRange, settings.colorThemeName, 256, alpha);
			}
			else if (m_settingsDlg->rbQualitative->isChecked())
			{
				m_lut->setRange(lutRange);
				m_lut->allocate(lutRange[1] - lutRange[0]);
				auto theme = iAColorThemeManager::instance().theme(settings.colorThemeQualName);
				for (size_t colorIdx = 0; colorIdx < lutRange[1] - lutRange[0]; ++colorIdx)
				{
					m_lut->setColor(colorIdx, theme->color(colorIdx % theme->size()));
				}
			}
			else
			{
				DEBUG_LOG("Invalid color state!");
			}
			// intentional fall-through!
		case cmCustom:
			m_lut->setOpacity(alpha);
	}
	applyLookupTable();
	emit lookupTableChanged();
}

void iAQSplom::applyLookupTable()
{
	QSignalBlocker colorChoiceBlock(m_settingsDlg->cbColorParameter);
	m_settingsDlg->cbColorParameter->setCurrentIndex(static_cast<int>(m_colorLookupParam));
	for (auto& row : m_matrix)
	{
		for (auto s : row)
		{
			if (s)
			{
				s->setLookupTable(m_lut, m_colorLookupParam);
			}
		}
	}
	if (m_maximizedPlot)
	{
		m_maximizedPlot->setLookupTable(m_lut, m_colorLookupParam);
	}
	update();
}

void iAQSplom::pointRadiusChanged(int newValue)
{
	settings.pointRadius = newValue / PointRadiusFractions;
	m_settingsDlg->lbPointSizeValue->setText(QString::number(settings.pointRadius, 'f', 1));
	for (auto& row : m_matrix)
	{
		for (auto s : row)
		{
			if (s)
			{
				s->setPointRadius(settings.pointRadius);
			}
		}
	}
	if (m_maximizedPlot)
	{
		m_maximizedPlot->setPointRadius(settings.pointRadius);
	}
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
	m_settingsDlg->lbPointOpacityValue->setText(QString::number(opacity, 'f', 2));
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

iAQSplom::ColorMode iAQSplom::colorMode() const
{
	return settings.colorMode;
}

void iAQSplom::setColorParam(const QString & paramName)
{
	size_t colorLookupParam = m_splomData->paramIndex(paramName);
	setColorParam(colorLookupParam);
}

void iAQSplom::setColorParam(size_t colorLookupParam)
{
	if (colorLookupParam == std::numeric_limits<size_t>::max())
	{
		return;
	}
	m_colorLookupParam = colorLookupParam;
	setColorMode(cmByParameter);
}

void iAQSplom::changePointColor()
{
	QColor newColor = QColorDialog::getColor(settings.pointColor, this, "SPM Point color");
	if (newColor.isValid())
	{
		setPointColor(newColor);
	}
}

void iAQSplom::saveSettingsSlot()
{
	QString fileName = QFileDialog::getSaveFileName(this, "Save settings", "",
		tr("Settings file (*.ini);;"));
	if (fileName.isEmpty())
	{
		return;
	}
	QSettings iniFile(fileName, QSettings::IniFormat);
	iniFile.setIniCodec("UTF-8");
	saveSettings(iniFile);
}

void iAQSplom::loadSettingsSlot()
{
	QString fileName = QFileDialog::getOpenFileName(this, "Load settings", "",
		tr("Settings file (*.ini);;"));
	if (fileName.isEmpty())
	{
		return;
	}
	QSettings iniFile(fileName, QSettings::IniFormat);
	iniFile.setIniCodec("UTF-8");
	loadSettings(mapFromQSettings(iniFile));
}

/*
	// settable only by application:
	bool selectionEnabled, enableColorSettings, maximizedLinked;
	int separationMargin;
	long plotsSpacing, tickLabelsOffset, maxRectExtraOffset;
	QPoint tickOffsets;
	QColor backgroundColor, popupBorderColor, popupFillColor, popupTextColor, selectionColor;
	double popupTipDim[2];
	double popupWidth;
	bool isAnimated;
	double animDuration;
	double animStart;

	// settable by user:
	double pointRadius;
	bool histogramVisible; int histogramBins;
	int selectionMode;
	bool flipAxes, quadraticPlots, showPCC, showSCC;
	ColorMode colorMode;  same color for all points, color-code by parameter value, custom (lut set by application)
	QString colorThemeName;
	QString colorThemeNameQual;
	QColor pointColor;

	list of visible parameters
	point opacity
	index of parameter used for color coding if by parameter value
	color coding min/max
	};
*/

void iAQSplom::saveSettings(QSettings & iniFile) const
{
	double pointOpacity = static_cast<double>(m_settingsDlg->slPointOpacity->value()) / m_settingsDlg->slPointOpacity->maximum();
	double colorCodingMin = m_settingsDlg->sbMin->value();
	double colorCodingMax = m_settingsDlg->sbMax->value();
	iniFile.setValue(CfgKeyPointRadius, settings.pointRadius);
	iniFile.setValue(CfgKeyHistogramVisible, settings.histogramVisible);
	iniFile.setValue(CfgKeyHistogramBins, settings.histogramBins);
	iniFile.setValue(CfgKeySelectionMode, settings.selectionMode);
	iniFile.setValue(CfgKeyFlipAxes, settings.flipAxes);
	iniFile.setValue(CfgKeyShowColorLegend, settings.showColorLegend);
	iniFile.setValue(CfgKeyQuadraticPlots, settings.quadraticPlots);
	iniFile.setValue(CfgKeyShowPCC, settings.showPCC);
	iniFile.setValue(CfgKeyShowSCC, settings.showSCC);
	iniFile.setValue(CfgKeyColorMode, settings.colorMode);
	iniFile.setValue(CfgKeyColorThemeName, settings.colorThemeName);
	iniFile.setValue(CfgKeyColorThemeQualName, settings.colorThemeQualName);
	iniFile.setValue(CfgKeyColorParameterMode, settings.colorParameterMode);
	iniFile.setValue(CfgKeyPointColor, settings.pointColor.rgba());
	iniFile.setValue(CfgKeyPointOpacity, pointOpacity);
	iniFile.setValue(CfgKeyColorRangeMode, settings.colorRangeMode);
	iniFile.setValue(CfgKeyColorCodingMin, colorCodingMin);
	iniFile.setValue(CfgKeyColorCodingMax, colorCodingMax);
	iniFile.setValue(CfgKeyColorLookupParam, static_cast<qulonglong>(m_colorLookupParam));
	iniFile.setValue(CfgKeyVisibleParameters, joinAsString(m_visibleIndices, ","));
	if (m_maximizedPlot)
	{
		iniFile.setValue(CfgKeyMaximizedPlot, QString("%1,%2").arg(m_maximizedPlot->getIndices()[0]).arg(m_maximizedPlot->getIndices()[1]));
	}
}

void iAQSplom::loadSettings(iASettings const & config)
{
	bool ok;
	double newPointRadius = config.value(CfgKeyPointRadius, settings.pointRadius).toDouble(&ok);
	if (!ok)
	{
		DEBUG_LOG(QString("Invalid value for 'PointRadius' setting ('%1') in Scatter Plot Matrix settings")
			.arg(config.value(CfgKeyPointRadius).toString()));
	}
	if (settings.pointRadius != newPointRadius)
	{
		setPointRadius(newPointRadius);
	}

	int newHistogramBins = config.value(CfgKeyHistogramBins, settings.histogramBins).toInt();
	bool newHistogramVisible = config.value(CfgKeyHistogramVisible, settings.histogramVisible).toBool();
	if (settings.histogramVisible != newHistogramVisible || settings.histogramBins != newHistogramBins)
	{
		settings.histogramBins = newHistogramBins;
		QSignalBlocker sb(m_settingsDlg->sbHistogramBins);
		m_settingsDlg->sbHistogramBins->setValue(newHistogramBins);
		setHistogramVisible(newHistogramVisible);
	}

	int newSelectionMode = config.value(CfgKeySelectionMode, settings.selectionMode).toInt();
	if (settings.selectionMode != newSelectionMode)
	{
		setSelectionMode(newSelectionMode);
	}
	bool newFlipAxes = config.value(CfgKeyFlipAxes, settings.flipAxes).toBool();
	if (settings.flipAxes != newFlipAxes)
	{
		setFlipAxes(newFlipAxes);
	}
	bool newShowColorLegend = config.value(CfgKeyShowColorLegend, settings.showColorLegend).toBool();
	if (settings.showColorLegend != newShowColorLegend)
	{
		setShowColorLegend(newShowColorLegend);
	}
	bool newQuadraticPlots = config.value(CfgKeyQuadraticPlots, settings.quadraticPlots).toBool();
	if (settings.quadraticPlots != newQuadraticPlots)
	{
		setQuadraticPlots(newQuadraticPlots);
	}
	bool newShowPCC = config.value(CfgKeyShowPCC, settings.showPCC).toBool();
	if (settings.showPCC != newShowPCC)
	{
		setShowPCC(newShowPCC);
	}
	bool newShowSCC = config.value(CfgKeyShowSCC, settings.showSCC).toBool();
	if (settings.showSCC != newShowSCC)
	{
		setShowSCC(newShowSCC);
	}

	// load visible parameters:
	if (config.contains(CfgKeyVisibleParameters))
	{
		QStringList newVisibleIndices = config.value(CfgKeyVisibleParameters).toString().split(",");
		std::vector<char> newParamVis(m_paramVisibility.size(), 0);
		int paramsSetVisible = 0;
		for (QString idxStr : newVisibleIndices)
		{
			size_t idx = idxStr.toULongLong(&ok);
			if (!ok)
			{
				DEBUG_LOG(QString("Invalid index %1 in VisibleParameter Scatter Plot Matrix setting.").arg(idxStr));
				continue;
			}
			if (idx >= newParamVis.size())
			{
				DEBUG_LOG(QString("Index %1 in VisibleParameter settings is larger than currently available number of parameters (%2); "
					"probably these settings were stored for a different Scatter Plot Matrix!").arg(idx).arg(newParamVis.size()));
			}
			else
			{
				++paramsSetVisible;
				newParamVis[idx] = 1;
			}
		}
		if (paramsSetVisible >= 2)
		{
			setParameterVisibility(newParamVis);
		}
	}

	// write all settings directly to settings object / blocked GUI elements.
	// Except for opacity, which triggers the required updateLookupTable to apply the settings
	settings.colorMode = static_cast<ColorMode>(config.value(CfgKeyColorMode, settings.colorMode).toInt());
	QSignalBlocker blockColorMode(m_settingsDlg->cbColorMode);
	m_settingsDlg->cbColorMode->setCurrentIndex(settings.colorMode);

	settings.colorParameterMode = static_cast<ColorParameterMode>(config.value(CfgKeyColorParameterMode, settings.colorParameterMode).toInt());
	QSignalBlocker blockRBContinuous(m_settingsDlg->rbContinuous), blockRBQualitative(m_settingsDlg->rbQualitative);
	m_settingsDlg->rbContinuous->setChecked(settings.colorParameterMode == pmContinuous);
	m_settingsDlg->rbQualitative->setChecked(settings.colorParameterMode == pmQualitative);

	settings.colorThemeName = config.value(CfgKeyColorThemeName, settings.colorThemeName).toString();
	QSignalBlocker blockColorTheme(m_settingsDlg->cbColorTheme);
	m_settingsDlg->cbColorTheme->setCurrentText(settings.colorThemeName);

	settings.colorThemeQualName = config.value(CfgKeyColorThemeQualName, settings.colorThemeQualName).toString();
	QSignalBlocker blockColorThemeQual(m_settingsDlg->cbColorThemeQual);
	m_settingsDlg->cbColorThemeQual->setCurrentText(settings.colorThemeQualName);

	QColor newPointColor(QColor::fromRgba(config.value(CfgKeyPointColor, settings.pointColor.rgba()).toUInt()));
	m_settingsDlg->pbPointColor->setStyleSheet(QString("background-color:%1").arg(newPointColor.name()));

	QSignalBlocker blockcbColorRangeMode(m_settingsDlg->cbColorRangeMode);
	settings.colorRangeMode = static_cast<ColorRangeMode>(config.value(CfgKeyColorRangeMode, settings.colorRangeMode).toInt());
	m_settingsDlg->cbColorRangeMode->setCurrentIndex(settings.colorRangeMode);

	auto tmpColorLookupParam = config.value(CfgKeyColorLookupParam, static_cast<qulonglong>(m_colorLookupParam)).toULongLong();
	if (tmpColorLookupParam < m_splomData->numParams())
	{
		m_colorLookupParam = tmpColorLookupParam;
		QSignalBlocker blockColorParameter(m_settingsDlg->cbColorParameter);
		m_settingsDlg->cbColorParameter->setCurrentIndex(static_cast<int>(m_colorLookupParam));
	}
	else
	{
		DEBUG_LOG(QString("Stored index of parameter to color by (%1) exceeds valid range (0..%2)")
			.arg(tmpColorLookupParam)
			.arg(m_splomData->numParams()));
	}

	if (settings.colorRangeMode == rmManual)
	{
		QSignalBlocker blockMin(m_settingsDlg->sbMin), blockMax(m_settingsDlg->sbMax);
		m_settingsDlg->sbMin->setValue(config.value(CfgKeyColorCodingMin, m_settingsDlg->sbMin->value()).toDouble());
		m_settingsDlg->sbMax->setValue(config.value(CfgKeyColorCodingMax, m_settingsDlg->sbMax->value()).toDouble());
	}
	else
	{
		rangeFromParameter();
	}

	// setting value to slPointOpacity is not blocked, because this triggers the one updateLookupTable we want here
	double opacity = static_cast<double>(m_settingsDlg->slPointOpacity->value()) / m_settingsDlg->slPointOpacity->maximum();
	opacity = config.value(CfgKeyPointOpacity, opacity).toDouble();
	m_settingsDlg->slPointOpacity->setValue(opacity * m_settingsDlg->slPointOpacity->maximum());

	if (config.contains(CfgKeyMaximizedPlot))
	{
		QStringList strInds = config[CfgKeyMaximizedPlot].toString().split(",");
		if (strInds.size() != 2)
		{
			DEBUG_LOG(QString("Expected two indices separated by comma in %1 setting, but got %2")
				.arg(CfgKeyMaximizedPlot)
				.arg(config[CfgKeyMaximizedPlot].toString()));
		}
		else
		{
			bool ok1, ok2;
			size_t idx1 = strInds[0].toULongLong(&ok1);
			size_t idx2 = strInds[1].toULongLong(&ok2);
			if (!ok1 || !ok2 || idx1 >= m_splomData->numParams() || idx2 >= m_splomData->numParams())
			{
				DEBUG_LOG(QString("Cannot create maximized plot from setting %1, invalid or out-of-range indices: %2")
					.arg(CfgKeyMaximizedPlot)
					.arg(config[CfgKeyMaximizedPlot].toString()));
			}
			else
			{
				auto idx1VisIt = std::find(m_visibleIndices.begin(), m_visibleIndices.end(), idx1);
				auto idx2VisIt = std::find(m_visibleIndices.begin(), m_visibleIndices.end(), idx2);
				if (idx1VisIt == m_visibleIndices.end() || idx1VisIt == m_visibleIndices.end())
				{
					DEBUG_LOG(QString("Cannot create maximized plot from setting %1, given parameter indices %2, %3 were not among visible plots!")
						.arg(CfgKeyMaximizedPlot)
						.arg(idx1)
						.arg(idx2));
				}
				else
				{
					size_t visIdx1 = std::distance(m_visibleIndices.begin(), idx1VisIt);
					size_t visIdx2 = std::distance(m_visibleIndices.begin(), idx2VisIt);
					iAScatterPlot* s = m_visiblePlots[visIdx2][visIdx1];
					maximizeSelectedPlot(s);
				}
			}
		}
	}
}

void iAQSplom::setPointColor(QColor const & newColor)
{
	if (settings.pointColor == newColor)
	{
		return;
	}
	settings.pointColor = newColor;
	m_settingsDlg->pbPointColor->setStyleSheet(QString("background-color:%1").arg(newColor.name()));
	setColorMode(cmAllPointsSame);
}

void iAQSplom::colorModeChanged(int colorMode)
{
	if (!settings.enableColorSettings)
	{
		DEBUG_LOG("setColorMode called despite enableColorSettings being false!");
		return;
	}
	if (colorMode == cmCustom)
	{
		QMessageBox::warning(this, "SPM settings", "Custom color mode can not be used from the settings dialog");
		return;
	}
	setColorMode(static_cast<ColorMode>(colorMode));
}

void iAQSplom::setColorMode(ColorMode colorMode)
{
	settings.colorMode = colorMode;
	if (settings.colorMode == cmByParameter && settings.colorRangeMode == rmAutomatic)
	{
		rangeFromParameter();
	}
	updateColorControls();
}

void iAQSplom::setColorParameterMode(ColorParameterMode paramMode)
{
	settings.colorParameterMode = paramMode;
	updateColorControls();
}

void iAQSplom::setColorRangeMode(ColorRangeMode rangeMode)
{
	settings.colorRangeMode = rangeMode;
	if (settings.colorRangeMode == rmAutomatic)
	{
		rangeFromParameter();
	}
	updateColorControls();
}

void iAQSplom::setContinousParamMode()
{
	settings.colorParameterMode = pmContinuous;
	updateColorControls();
}

void iAQSplom::setQualitativeParamMode()
{
	settings.colorParameterMode = pmQualitative;
	updateColorControls();
}

void iAQSplom::colorRangeModeChanged()
{
	setColorRangeMode(static_cast<ColorRangeMode>(m_settingsDlg->cbColorRangeMode->currentIndex()));
}

void iAQSplom::updateColorControls()
{
	QSignalBlocker cbColorBlock(m_settingsDlg->cbColorMode);
	m_settingsDlg->cbColorMode->setCurrentIndex(settings.colorMode);
	m_settingsDlg->pbPointColor->setEnabled(settings.colorMode == cmAllPointsSame);
	m_settingsDlg->lbPointColor->setEnabled(settings.colorMode == cmAllPointsSame);
	m_settingsDlg->cbColorParameter->setEnabled(settings.colorMode == cmByParameter);
	m_settingsDlg->lbColorParameter->setEnabled(settings.colorMode == cmByParameter);
	m_settingsDlg->lbRange->setEnabled(settings.colorMode == cmByParameter);
	m_settingsDlg->lbRangeMin->setEnabled(settings.colorMode == cmByParameter && settings.colorRangeMode == rmManual);
	m_settingsDlg->lbRangeMax->setEnabled(settings.colorMode == cmByParameter && settings.colorRangeMode == rmManual);
	m_settingsDlg->sbMin->setEnabled(settings.colorMode == cmByParameter && settings.colorRangeMode == rmManual);
	m_settingsDlg->sbMax->setEnabled(settings.colorMode == cmByParameter && settings.colorRangeMode == rmManual);
	m_settingsDlg->pbRangeFromParameter->setEnabled(settings.colorMode == cmByParameter && settings.colorRangeMode == rmManual);
	m_settingsDlg->lbColorTheme->setEnabled(settings.colorMode == cmByParameter);
	m_settingsDlg->cbColorTheme->setEnabled(settings.colorMode == cmByParameter && settings.colorParameterMode == pmContinuous);
	m_settingsDlg->cbColorThemeQual->setEnabled(settings.colorMode == cmByParameter && settings.colorParameterMode == pmQualitative);
	m_settingsDlg->rbQualitative->setEnabled(settings.colorMode == cmByParameter);
	m_settingsDlg->rbContinuous->setEnabled(settings.colorMode == cmByParameter);
	m_settingsDlg->cbColorRangeMode->setEnabled(settings.colorMode == cmByParameter);
	updateLookupTable();
}

void iAQSplom::setColorThemeFromComboBox(int index)
{
	setColorTheme(m_settingsDlg->cbColorTheme->itemText(index));
}

void iAQSplom::setColorTheme(QString const & themeName)
{
	settings.colorThemeName = themeName;
	if (m_settingsDlg->cbColorTheme->currentText() != themeName)
	{
		QSignalBlocker sb(m_settingsDlg->cbColorTheme);
		m_settingsDlg->cbColorTheme->setCurrentText(themeName);
	}
	if (settings.colorMode == cmByParameter && settings.colorParameterMode == pmContinuous)
	{
		updateLookupTable();
	}
}

void iAQSplom::setColorThemeQual(int index)
{
	QString const themeName = m_settingsDlg->cbColorThemeQual->itemText(index);
	settings.colorThemeQualName = themeName;
	if (m_settingsDlg->cbColorThemeQual->currentText() != themeName)
	{
		QSignalBlocker sb(m_settingsDlg->cbColorThemeQual);
		m_settingsDlg->cbColorThemeQual->setCurrentText(themeName);
	}
	if (settings.colorMode == cmByParameter && settings.colorParameterMode == pmQualitative)
	{
		updateLookupTable();
	}
}

void iAQSplom::rangeFromParameter()
{
	double const * range = m_splomData->paramRange(m_colorLookupParam);
	QSignalBlocker sb(m_settingsDlg->sbMin);
	m_settingsDlg->sbMin->setValue(range[0]);
	m_settingsDlg->sbMax->setValue(range[1]);
}
