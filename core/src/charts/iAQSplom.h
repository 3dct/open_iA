/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#pragma once

#include "open_iA_Core_export.h"

#include "iAScatterPlotSelectionHandler.h"

#include <QtGlobal>
#include <vtkVersion.h>
#if (VTK_MAJOR_VERSION >= 8 && defined(VTK_OPENGL2_BACKEND) && QT_VERSION >= 0x050400 )
#include <QOpenGLWidget>
#else
#include <QGLWidget>
#endif
#include <QList>

#include <vector>

class iAChartWidget;
class iAColorTheme;
class iALookupTable;
class iAScatterPlot;
class iASPLOMData;
class iASPMSettings;

class vtkLookupTable;

class QGridLayout;
class QListWidgetItem;
class QMenu;
class QPropertyAnimation;
class QTableWidget;

//! A scatter plot matrix (SPLOM) widget.
/*!
	Multidimensional data points are shown using a grid of 2D scatter plots for each pair of dimensions.
	Each plot is interactive and user-transformations such as scaling and translating
	are possible using mouse wheel and right-button dragging correspondingly.
	When 'R' key is pressed all the transformations are reset to default.
	The user can also hover over points of any plot, to see a popup with detailed information about the point.
	When hovering over a point, it will be highlighted in all plots to allow interactive exploration.
	Any parameter of the data can be color-coded using a lookup table.
	Any scatter plot from an upper matrix triangle can be maximized by clicking a button in the upper-right corner.
	The maximized plot can be minimized by using a button in upper-right corner.
	Inherits QGLWidget,manages scatter plots internally.
	Some customization options are available via the public settings member.

	Usage:
	- Create iAQSPLOM
	- add to a window/widget, make visible
	- set some data (using one of the setData methods)
	- set a lookup table for dot colors (setLookupTable)
	- (optional:) set the parameter visibility (by default, all parameters are visible
	- (optional:)

	Example (visibleWidget is some widget, currently shown, with a layout):
		iAQSplom* splom = new iAQSplom();
		visibleWidget->layout()->addWidget(splom);
		// ... you might want enable Qt to process a paint event here, to make sure the OpenGL context is created
		QSharedPoiner<iASPLOMData> splomData = createSPLOMDataSomehow();
		splom->setData(splomData);
		iALookupTable lut;
		// we assume here you want all points colored in a middle gray; we set up a color lookoup table
		// over all values of column 0, and set the same color for the whole range
		// if you want to color the dots by an actual column value, adapt this to your own needs!
		lut.setRange( splomData->paramRange(0) );
		lut.allocate(2);
		QColor CustomDotColor(128, 128, 128)
		lut.setColor( 0, CustomDotColor );
		lut.setColor( 1, CustomDotColor );
		splom->setLookupTable( lut, 0 );
*/

#if (VTK_MAJOR_VERSION >= 8 && defined(VTK_OPENGL2_BACKEND) && QT_VERSION >= 0x050400 )
class open_iA_Core_API iAQSplom : public QOpenGLWidget, public iAScatterPlotSelectionHandler
#else
class open_iA_Core_API iAQSplom : public QGLWidget, public iAScatterPlotSelectionHandler
#endif
{
	Q_OBJECT
	Q_PROPERTY( double m_animIn READ getAnimIn WRITE setAnimIn )
	Q_PROPERTY( double m_animOut READ getAnimOut WRITE setAnimOut )

	enum splom_mode    //!< two possible states of SPLOM: upper triangle with maximized plot or all possible combinations of scatter plots
	{
		UPPER_HALF,
		ALL_PLOTS
	};
public:
	enum ColorScheme   //!< what color scheme to use for coloring the dots
	{
		AllPointsSame, //!< all points have the same color
		ByParameter,   //!< points are colored by a specific parameter, using a diverging, perceptually uniform lookup table
		Custom         //!< points are colored
	};
// Methods
#if (VTK_MAJOR_VERSION >= 8 && defined(VTK_OPENGL2_BACKEND) && QT_VERSION >= 0x050400 )
	iAQSplom( QWidget * parent = 0, Qt::WindowFlags f = 0 );
#else
	iAQSplom( QWidget * parent = 0, const QGLWidget * shareWidget = 0, Qt::WindowFlags f = 0 );
#endif
	~iAQSplom();

	virtual void setData( const QTableWidget * data );               //! import data from QTableWidget, first row should contain parameter names, each column corresponds to one parameter.
	void setData(QSharedPointer<iASPLOMData> data, std::vector<char> const & visibility);                  //! set SPLOM data directly.
	QSharedPointer<iASPLOMData> data();                              //! retrieve SPLOM data
	void setLookupTable( vtkLookupTable * lut, const QString & paramName ); //!< Set lookup table from VTK (vtkLookupTable) given the name of a parameter to color-code.
	void setLookupTable( iALookupTable &lut, size_t paramIndex );    //!< Set lookup table given the index of a parameter to color-code.
	void setColorParam( const QString & paramName );                 //!< Set the parameter to color code, lookup table will be auto-determined (perceptually uniform, divergent)
	void setParameterVisibility(std::vector<char> const & visibility);//!< Adapt visibility of all parameters at once.
	void setParameterVisibility( size_t paramIndex, bool isVisible );//!< Show/hide scatter plots of a parameter given parameter's index.
	void setParameterVisibility( const QString & paramName, bool isVisible ); //!< Show/hide scatter plots of a parameter given parameter's name.
	void setParameterInverted( size_t paramIndex, bool isInverted);  //!< whether to invert the axis for a given parameter's index.
	void setPointRadius( double radius );                            //!< set the radius for scatter plot points
	void setPointColor( QColor const & color );                      //!< set the color for all data points
	void setPointOpacity( double opacity );                          //!< set the opacity for all data points
	SelectionType & getSelection() override;                         //!< Get const vector of indices of currently selected data points.
	SelectionType const & getSelection() const override;             //!< Get vector of indices of currently selected data points.
	SelectionType const & getFilteredSelection() const override;     //!< Get currently selected data points, as indices in the list of filtered data points. These indices are always sorted.
	void setFilteredSelection(SelectionType const & filteredSelInds);//!< Set selected data points from indices within the filtered data points
	void setSelection( SelectionType const & selInds );              //!< Set selected data points from a vector of indices.
	void clearSelection();                                           //!< deletes current selection
	void setSelectionColor(QColor color);                            //!< set the color for selected points
	void enableSelection(bool enable);                               //!< set whether selections are allowed or not
	void getActivePlotIndices( int * inds_out );                     //!< Get X and Y parameter indices of currently active scatter plot.
	int getVisibleParametersCount() const override;                  //!< Get the number of parameters currently displayed
	double getAnimIn() const override { return m_animIn; }           //!< Getter for animation in property
	void setAnimIn( double anim );                                   //!< Setter for animation in property
	double getAnimOut() const override { return m_animOut; }         //!< Getter for animation in property
	void setAnimOut( double anim );                                  //!< Setter for animation in property
	SelectionType const & getHighlightedPoints() const override;     //!< get the list of highlighted points
	void setSeparation(int idx);                                     //!< define an index at which a separation margin is inserted
	void setBackgroundColorTheme(iAColorTheme const * theme);        //!< define the color theme to use for coloring the different separated regions
	iAColorTheme const * getBackgroundColorTheme();                  //!< retrieve the theme for background colors for the separated regions
	void showAllPlots(const bool enableAllPlotsVisible);             //!< switch between showing all plots or only upper half
	void showDefaultMaxizimedPlot();                                 //!< maximize plot in upper left corner
	void addFilter(int paramIndex, double value);                    //!< set filter on data to be shown; only data points where given parameter contains given value will be shown
	void removeFilter(int paramIndex, double value);                 //!< remove a specific filter
	void resetFilter();                                              //!< reset filter on data; after calling this method, all data points will be shown again
	void addContextMenuAction(QAction* action);                      //!< add an additional option to the context menu
	size_t colorLookupParam() const;                                 //!< parameter currently used for color lookup
	iALookupTable const & lookupTable() const;                       //!< get lookup table
	ColorScheme colorScheme() const;                                 //!< get current color scheme
public slots:
	void setHistogramVisible(bool visible);                          //!< set visibility of histograms
	void setHistogramBins(int bins);                                 //!< set the number of histogram bins
	void showSettings();                                             //!< Show the settings dialog
	void setSelectionMode(int mode);                                 //!< set selection mode to either rectangle or polygon mode
	void setColorTheme(QString const & themeName);                   //!< Call to adapt color theme used for coloring by a parameter
signals:
	void selectionModified(SelectionType const & selInds);           //!< Emitted when new data points are selected. Contains a list of selected data points.
	void currentPointModified(size_t index);                         //!< Emitted when hovered over a new point.
	void parameterVisibilityChanged(size_t paramIndex, bool visible);//!< Emitted when the visibility of a parameter has changed (from within SPLOM, not triggered if it was set from the outside via setParameterVisibility).
	void lookupTableChanged();                                       //!< Emitted when the lookup table has changed
protected:
	void clear();                                                    //!< Clear all scatter plots in the SPLOM.
	void initializeGL() override;                                    //!< overrides function inherited from base class.
	void paintEvent( QPaintEvent * event ) override;                 //!< Draws all scatter plots, tick labels and axes.
	virtual bool drawPopup( QPainter& painter );                     //!< Draws popup on the splom
	iAScatterPlot * getScatterplotAt( QPoint pos );                  //!< Get a scatter plot at mouse position.
	void changeActivePlot( iAScatterPlot * s);
	void drawVisibleParameters(QPainter & painter);                  //!< draws label for the whole scatter plot matrix
	void drawPlotLabels(QVector<ulong> &ind_VisX, int axisOffSet, QPainter & painter, bool switchXY);
	void drawTicks( QPainter & painter, QList<double> const & ticksX, QList<double> const & ticksY, QList<QString> const & textX,
	    QList<QString> const & textY);                               //!< Draw ticks for X and Y axes of all plots in the SPLOM.
	void updateMaxPlotRect();                                        //!< Updates the rectangle of the maximized scatter plot.
	void updateSPLOMLayout();                                        //!< Updates SPLOM layout: every plot in the matrix + maximized plot (if exists).
	void updatePlotGridParams();                                     //!< Updates some parameters used for the matrix grid layout calculations.
	void updateVisiblePlots();                                       //!< Updates matrix using only plots that are currently visible.
	void removeMaxPlotIfHidden();                                    //!< Removes maximized plot if any of his parameters is hidden.
	void resetTransform();                                           //!< Resets transform in all plots of the matrix.
	QRect getPlotRectByIndex( int x, int y );                        //!< Get a rectangle of a plot by its indices.
	void removeMaximizedPlot();                                      //!< Removes maximized plot.
	int invert( int val ) const;                                     //!< Inverts parameter index. Used for inverting SPLOM Y indexing order.
	int getMaxTickLabelWidth(QList<QString> const & textX, QFontMetrics & fm) const; //!< Get the width of the longest tick label width
	void maximizeSelectedPlot(iAScatterPlot * selectedPlot);         //!< shows a maximized preview of the selected plot
	//! @{ Overrides of Qt event handlers
	void wheelEvent( QWheelEvent * event ) override;
	void resizeEvent( QResizeEvent * event ) override;
	void mousePressEvent( QMouseEvent * event ) override;
	void mouseReleaseEvent( QMouseEvent * event ) override;
	void mouseMoveEvent( QMouseEvent * event ) override;
	void keyPressEvent( QKeyEvent * event ) override;
	void mouseDoubleClickEvent( QMouseEvent * event ) override;
	void contextMenuEvent(QContextMenuEvent *event) override;
	//! @}
	virtual void addHighlightedPoint(size_t index);                  //!< Keep a point with index always highlighted
	virtual void removeHighlightedPoint(size_t index);               //!< Remove a point from the highlighted list
protected slots:
	virtual void currentPointUpdated(size_t index);                  //!< When hovered over a new point.
private:
	void dataChanged(std::vector<char> visibleParams);               //!< handles changes of the internal data
	void updateFilter();                                             //!< update filter in internal scatter plots
	void updateHistograms();                                         //!< Updates all histograms when data or filter changes
	void updateHistogram(size_t paramIndex);                         //!< Updates the histogram of the given parameter
	void setColorScheme(ColorScheme colorScheme);                    //!< Set color scheme (method how points are colored)
	void applyLookupTable();                                         //!< Apply lookup table to all the scatter plots.
	void createScatterPlot(size_t y, size_t x, bool initial);        //!< Creates a single scatter plot at location y, y
private slots:
	void selectionUpdated();                                         //!< When selection of data points is modified.
	void transformUpdated( double scale, QPointF deltaOffset );      //!< When transform of scatter plots is modified.
	void setQuadraticPlots(bool quadratic);                          //!< set whether plots are restricted to quadratic size
	void setShowPCC(bool showPCC);                                   //!< set whether the correlation coefficient is shown in each plot
	void selectionModePolygon();                                     //!< set selection mode to polygon
	void selectionModeRectangle();                                   //!< set selection mode to rectangle
	void parameterVisibilityToggled(bool enabled);                   //!< called when parameter visibility is adapted through the context menu
	void changeParamVisibility(QListWidgetItem * item);              //!< Show/hide a parameter in SPLOM when list widget item is clicked
	void setParameterToColorCode(int paramIndex);                    //!< Apply color coding based on the parameter index
	void updateLookupTable();                                        //!< Update lookup table sensitivity
	void pointRadiusChanged(int);                                    //!< Called from settings dialog when point size slider changes
	void pointOpacityChanged(int);                                   //!< Called from settings dialog when opacity slider changes
	void colorSchemeChanged(int colorScheme);                        //!< Called from settings dialog when the color scheme is changed
	void changePointColor();                                         //!< Called from settings dialog when the point color is clicked
	void rangeFromParameter();                                       //!< Called from settings dialog when color range should be determined from parameter

// Members:
public:
	//! All settings of the plot in one struct
	struct Settings
	{
		Settings();
		long plotsSpacing;
		long tickLabelsOffset;
		long maxRectExtraOffset;
		QPoint tickOffsets;
		QColor backgroundColor;
		bool maximizedLinked;

		QColor popupBorderColor;
		QColor popupFillColor;
		QColor popupTextColor;
		QColor selectionColor;

		double popupTipDim[2];
		double popupWidth;
		double pointRadius;

		bool isAnimated;
		double animDuration;
		double animStart;

		int separationMargin;
		int histogramBins;
		bool histogramVisible;

		int selectionMode;                       //!< The selection mode of all scatter plots
		bool selectionEnabled;                   //!< Whether selection is enabled in the SPLOM
		bool quadraticPlots;                     //!< Whether the scatter plots are constrained to quadratic sizes
		bool showPCC;                            //!< Whether to show the Pearson's correlation coefficient
		ColorScheme colorScheme;                 //!< Scheme to use for coloring the matrix dots
		QString colorThemeName;                  //!< Name of a color theme for when points are colored by a parameter (from iALUT)
		QColor pointColor;                       //!< Color for each point if color scheme is uniform
		bool enableColorSettings;                //!< Whether color coding settings are accessible
	};
	Settings settings;
protected:
	std::vector<std::vector<iAScatterPlot*> > m_matrix; //!< cache for all scatter plots
	std::vector<std::vector<iAScatterPlot*> > m_visiblePlots; //!< matrix of visible scatter plots
	std::vector<char> m_paramVisibility;         //!< array of individual parameter visibility
	std::vector<int> m_visibleIndices;           //!< stores mapping from visible plot index to parameter index
	QSharedPointer<iALookupTable> m_lut;         //!< lookup table, shared with individual scatter plots
	size_t m_colorLookupParam;                   //!< index of the column to use for color lookup
	QPoint m_scatPlotSize;                       //!< size of one scatter plot in the layout
	iAScatterPlot * m_activePlot;                //!< scatter plot that user currently interacts with
	splom_mode m_mode;                           //!< SPLOM current state: all plots or upper triangle with maximized plot
	QSharedPointer<iASPLOMData> m_splomData;     //!< contains raw data points used in SPLOM
	iAScatterPlot * m_previewPlot;               //!< plot currently being previewed (shown in maximized plot)
	iAScatterPlot * m_maximizedPlot;             //!< pointer to the maximized plot
	SelectionType m_selInds;                     //!< contains indices of currently selected data points
	mutable SelectionType m_filteredSelInds;     //!< contains indices of selected points in filtered list (TODO: update only when selection changes and when filters change, remove mutable)
	double m_animIn;                             //!< In animation parameter
	double m_animOut;                            //!< Out animation parameter
	QPropertyAnimation * m_animationIn;
	QPropertyAnimation * m_animationOut;
	SelectionType m_highlightedPoints;           //!< contains indices of always highlighted points
	double m_popupHeight;                        //!< height of the last drawn popup
	int m_separationIdx;                         //!< index at which to separate scatterplots spatially (e.g. into in- and output parameters)
	iAColorTheme const * m_bgColorTheme;         //!< background colors for regions in the scatterplot
	QMenu* m_contextMenu;                        //!< the context menu (can be extended via addContextMenuAction)
	QMenu* m_columnPickMenu;                     //!< sub-menu of the context menu for picking which columns are visible
private:
	QAction *showHistogramAction, *selectionModePolygonAction, *selectionModeRectangleAction, *quadraticPlotsAction, *showPCCAction;
	QVector<iAChartWidget*> m_histograms;        //!< histograms of scatter plot matrix
	iASPMSettings * m_settingsDlg;               //!< dialog with all the SPLOM settings (which params are visible, opacity of each dot, which column to use for coloring...
};
