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
#pragma once

#include "open_iA_Core_export.h"

#include "iAScatterPlotSelectionHandler.h"

#include <QGLWidget>
#include <QList>

class iAScatterPlot;
class iASPLOMData;
class QTableWidget;
class QGridLayout;
class QPropertyAnimation;
class iALookupTable;
class vtkLookupTable;

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
*/
class open_iA_Core_API iAQSplom : public QGLWidget, public iAScatterPlotSelectionHandler
{
	Q_OBJECT
	Q_PROPERTY( double m_animIn READ getAnimIn WRITE setAnimIn )
	Q_PROPERTY( double m_animOut READ getAnimOut WRITE setAnimOut )

	enum splom_mode //!< two possible states of SPLOM: upper triangle with maximized plot or all possible combinations of scatter plots
	{
		UPPER_HALF,
		ALL_PLOTS
	};
//Methods
public:
	iAQSplom( QWidget * parent = 0, const QGLWidget * shareWidget = 0, Qt::WindowFlags f = 0 );
	~iAQSplom();

	//! Import data into SPLOM from QTableWidget. It is assumed that the first row contains parameter names and each column corresponds to one parameter.
	virtual void setData( const QTableWidget * data ); 
	void setLookupTable( vtkLookupTable * lut, const QString & colorArrayName );	//!< Set lookup table from VTK (vtkLookupTable) given the name of a parameter to color-code.
	void setLookupTable( iALookupTable &lut, const QString & colorArrayName );	//!< Set lookup table given the name of a parameter to color-code.
	void applyLookupTable();												//!< Apply lookup table to all the scatter plots.
	void setParameterVisibility( int paramIndex, bool isVisible );			//!< Show/hide scatter plots of a parameter given parameter's index.
	void setParameterVisibility( const QString & paramName, bool isVisible ); //!< Show/hide scatter plots of a parameter given parameter's name.
	QVector<unsigned int> & getSelection();									//!< Get vector of indices of currently selected data points.
	void setSelection( const QVector<unsigned int> * selInds );				//!< Set selected data points from a vector of indices.
	void getActivePlotIndices( int * inds_out );							//!< Get X and Y parameter indices of currently active scatter plot.
	inline int getVisibleParametersCount() const							//!< Get the number of parameters currently displayed
	{
		return m_visiblePlots.size();
	}
	double getAnimIn() const { return m_animIn; }							//!< Getter for animation in property
	void setAnimIn( double anim );											//!< Setter for animation in property
	double getAnimOut() const { return m_animOut; }							//!< Getter for animation in property
	void setAnimOut( double anim );											//!< Setter for animation in property
	const QList<int> & getHighlightedPoints() const;

protected:
	void clear();												//!< Clear all scatter plots in the SPLOM.
	virtual void initializeGL();								//!< Re-implements QGLWidget.
	virtual void paintEvent( QPaintEvent * event );				//!< Draws SPLOM. Re-implements QGLWidget.
	virtual bool drawPopup( QPainter& painter );				//!< Draws popup on the splom
	iAScatterPlot * getScatterplotAt( QPoint pos );				//!< Get a scatter plot at mouse position.
	void changeActivePlot( iAScatterPlot * s);					//!< Specify the new active scatter plot.
	void drawTicks( QPainter & painter );						//!< Draw ticks for X and Y parameters of plots in the SPLOM.
	void updateMaxPlotRect();									//!< Updates the rectangle of the maximized scatter plot.
	void updateSPLOMLayout();									//!< Updates SPLOM layout: every plot in the matrix + maximized plot (if exists).
	void updatePlotGridParams();								//!< Updates some parameters used for the matrix grid layout calculations.
	void updateVisiblePlots();									//!< Updates matrix using only plots that are currently visible.
	void removeMaxPlotIfHidden();								//!< Removes maximized plot if any of his parameters is hidden.
	void resetTransform();										//!< Resets transform in all plots of the matrix.
	QRect getPlotRectByIndex( int x, int y );					//!< Get a rectangle of a plot by its indices.
	void removeMaximizedPlot();									//!< Removes maximized plot.
	int invert( int val ) const;								//!< Inverts parameter index. Used for inverting SPLOM Y indexing order.

	//Re-implements Qt event handlers
protected:
	virtual void wheelEvent( QWheelEvent * event );
	virtual void resizeEvent( QResizeEvent * event );
	virtual void mousePressEvent( QMouseEvent * event );
	virtual void mouseReleaseEvent( QMouseEvent * event );
	virtual void mouseMoveEvent( QMouseEvent * event );
	virtual void keyPressEvent( QKeyEvent * event );
	virtual void mouseDoubleClickEvent( QMouseEvent * event );

	//slots
protected slots:
	void selectionUpdated();									//!< When selection of data points is modified.
	void transformUpdated( double scale, QPointF deltaOffset );	//!< When transform of scatter plots is modified.
	virtual void currentPointUpdated( int index );				//!< When hovered over a new point.
	virtual void addHighlightedPoint(int index);				//!< Keep a point with index always highlighted
	virtual void removeHighlightedPoint( int index );			//!< Remove a point from the highlighted list
	void plotMaximized();										//!< When a scatter plot is maximized
	void plotMinimized();										//!< When maximized scatter plot is removed.

signals:
	void selectionModified( QVector<unsigned int> * selInds );		//!< Emitted when new data points are selected. Contains a list of selected data points. 
	void currentPointModified( int index );							//!< Emitted when hovered over a new point.

protected:
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

		double popupTipDim[2];
		double popupWidth;

		bool isAnimated;
		double animDuration;
		double animStart;
	};

//Members
public:
	Settings settings;
protected:
	QList<QList<iAScatterPlot*>> m_matrix;			//!< matrix of all scatter plots
	QList<QList<iAScatterPlot*>> m_visiblePlots;	//!< matrix of visible scatter plots
	QList<bool> m_paramVisibility;					//!< array of individual parameter visibility
	QSharedPointer<iALookupTable> m_lut;			//!< lookup table, shared with individual scatter plots
	QString m_colorArrayName;						//!< name of a color-coded parameter
	QPoint m_scatPlotSize;							//!< size of one scatter plot in the layout
	iAScatterPlot * m_activePlot;					//!< scatter plot that user currently interacts with
	splom_mode m_mode;								//!< SPLOM current state: all plots or upper triangle with maximized plot
	QSharedPointer<iASPLOMData> m_splomData;		//!< contains raw data points used in SPLOM
	iAScatterPlot * m_previewPlot;					//!< plot currently being previewed (shown in maximized plot)
	iAScatterPlot * m_maximizedPlot;				//!< pointer to the maximized plot
	QVector<unsigned int> m_selInds;				//!< array containing indices of currently selected data points
	const bool m_isIndexingBottomToTop;				//!< flag indicating if Y-indices in the SPLOM are inverted
	double m_animIn;								//!< In animation parameter
	double m_animOut;								//!< Out animation parameter
	QPropertyAnimation * m_animationIn;
	QPropertyAnimation * m_animationOut;
	QList<int> m_highlightedPoints;					//!< list of always highlighted points
	double m_popupHeight;							//!< height of the last drawn popup
};
