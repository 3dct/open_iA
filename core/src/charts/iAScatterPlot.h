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
#pragma once

#include "open_iA_Core_export.h"

#include <QGLWidget>
#include <QList>
#include <QOpenGLFramebufferObject>
#include <QScopedPointer>
#include <QWidget>

class iALookupTable;
class iAScatterPlotSelectionHandler;
class iASPLOMData;
class QTableWidget;
class QTimer;
class QGLBuffer;
class vtkLookupTable;

//! Represents a single scatter plot in the scatter plot matrix (SPLOM).
/*!
The class is managed from the parent SPLOM.
Draws its contents in a given rectangle on a parent SPLOM widget.
Appearance can be customized via the public settings member.
*/
class open_iA_Core_API iAScatterPlot : public QObject
{
	Q_OBJECT
		//Methods
public:
	enum SelectionMode
	{
		Rectangle,
		Polygon
	};
	//!  Constructor: requires a parent SPLOM widget
	iAScatterPlot(iAScatterPlotSelectionHandler * splom, QGLWidget* parent, int numTicks = 5, bool isMaximizedPlot = false);
	~iAScatterPlot();

	void setData( int x, int y, QSharedPointer<iASPLOMData> &splomData );			//!< Set data to the scatter plot using indices of X and Y parameters and the raw SPLOM data
	bool hasData() const;															//!< Check if data is already set to the plot
	//! Set color lookup table and the name of a color-coded parameter
	void setLookupTable( QSharedPointer<iALookupTable> &lut, QString const & colorArrayName );
	const int * getIndices() const { return m_paramIndices; }						//!< Get indices of X and Y parameters
	void setTransform( double scale, QPointF newOffset );							//!< Set new transform: new scale and new offset
	void setTransformDelta( double scale, QPointF deltaOffset );					//!< Set new transform: new scale and change in the offset (delta)
	QRect getRect() const { return m_globRect; }									//!< Get rectangle where scatter plot contents are displayed
	void setRect( QRect val );														//!< Set rectangle where scatter plot contents are displayed
	double getScale() const { return m_scale; }										//!< Get current scale applied to the plot points
	QPointF getOffset() const { return m_offset; }									//!< Get current offset applied to the plot points
	QPointF getPointPosition( int index ) const;									//!< Get global position of a point by its index
	double getPointRadius() const;													//!< Get point radius (magnified if plot is maximized)
	//!  Output positions and labels of plot ticks for X and Y axes
	void printTicksInfo( QList<double> * posX, QList<double> * posY, QList<QString> * textX, QList<QString> * textY ) const;
	void setCurrentPoint( int index );												//!< Set the index of currently hovered point
	int getCurrentPoint() const;													//!< Get the index of currently hovered point
	int getPreviousIndex() const;													//!< Get the index of previously hovered point or -1 if nothing
	int getPreviousPoint() const;													//!< Get the index of previously hovered point
	void paintOnParent( QPainter & painter );										//!< Paint plot's contents on a SPLOM-parent
	void setPreviewState( bool isPreviewPlot ) { m_isPreviewPlot = isPreviewPlot; }	//!< Set if this plot is currently previewed (displayed in maximized plot view)
	void leave() { m_curInd = m_prevPtInd = -1; m_isPlotActive = false; }							//!< Mouse is hovering over the plot's rectangle
	void enter() { m_isPlotActive = true; }											//!< Mouse left the plot's rectangle
	void UpdatePoints();

	/*Qt events are redirected from SPLOM to the active plot using these public event handlers*/
	void SPLOMWheelEvent( QWheelEvent * event );
	void SPLOMMouseMoveEvent( QMouseEvent * event );
	void SPLOMMousePressEvent( QMouseEvent * event );
	void SPLOMMouseReleaseEvent( QMouseEvent * event );
	void setSelectionColor(QColor selCol);

protected:
	int p2binx( double p ) const;											//!< Get grid bin index using parameter value X
	int p2biny( double p ) const;											//!< Get grid bin index using parameter value Y
	double p2x( double pval ) const;										//!< Parameter scalar value to X coordinate (pixels)
	double p2tx( double pval ) const;										//!< Parameter scalar value to normalized X coordinate [0,1]
	double x2p( double x ) const;											//!< X coordinate (pixels) to parameter value
	double p2y( double pval ) const;										//!< Parameter scalar value to Y coordinate (pixels)
	double p2ty( double pval ) const;										//!< Parameter scalar value to normalized Y coordinate [0,1]
	double y2p( double y ) const;											//!< Y coordinate (pixels) to parameter value
	double applyTransformX( double v ) const;								//!< Apply scaling and offset to X coordinate
	double revertTransformX( double v ) const;								//!< Revert scaling and offset to get X coordinate
	double applyTransformY( double v ) const;								//!< Apply scaling and offset to Y coordinate
	double revertTransformY( double v ) const;								//!< Revert scaling and offset to get Y coordinate
	void initGrid();														//!< Allocate lists for grid subdivision ( for point-picking acceleration)
	void updateGrid();														//!< Fill subdivision grid with points ( for point-picking acceleration)
	void calculateRanges();													//!< Compute parameter ranges
	void applyMarginToRanges();												//!< Apply margins to ranges so that points are not stretched border-to-border
	void calculateNiceSteps();												//!< Calculates nice steps displayed parameter ranges
	void calculateNiceSteps( double * r, QList<double> * ticks );				//!< Calculates nice steps displayed parameter ranges given a range and a desired number of ticks
	int getBinIndex( int x, int y ) const { return  y*m_gridDims[0] + x; }	//!< Get global grid bin offset (index) using X and Y bin indices
	int getPointIndexAtPosition( QPointF mpos ) const;						//!< Get index of data point under cursor, -1 if none
	QPointF getPositionFromPointIndex( int ind ) const;						//!< Get position of a data point with a given index
	void updateSelectedPoints( bool append = false );						//!< Update a set of selected points: if append add to the previous selection.
	void updateDrawRect();													//!< Re-calculate dimensions of the plot's rectangle
	QPoint getLocalPos( QPoint pos ) const;									//!< Local (plot) position from global (SPLOM)
	QPoint cropLocalPos( QPoint locPos ) const;								//!< Make sure that local position is inside plot's rectangle
	//void drawParameterName( QPainter &painter );							//!< Draws parameter name (only diagonal plots)
	void drawBorder( QPainter &painter );									//!< Draws plot's border
	void drawTicks( QPainter &painter );									//!< Draws plot's ticks
	void drawMaximizedLabels( QPainter &painter );							//!< Draws additional plot's labels (only maximized plot)
	void drawSelectionPolygon( QPainter &painter );							//!< Draws selection-lasso polygon
	void drawPoints( QPainter &painter );									//!< Draws plot's points (uses native OpenGL)
	//void drawMaximizeButton( QPainter & painter );							//!< Draws plot's maximized button (only active plot)
	void createAndFillVBO();												//!< Creates and fills VBO with plot's 2D-points.
	void fillVBO();															//!< Fill existing VBO with plot's 2D-points.



signals:
	void selectionModified();												//!< Emitted when selected points changed
	void transformModified( double scale, QPointF deltaOffset );			//!< Emitted when user transforms (scales, translates)
	void currentPointModified( int index );									//!< Emitted when hovered over new point

protected:
	//! All settings of the plot in one struct
	struct Settings
	{
		Settings();

		double pickedPointMagnification;

		int tickOffset;
		int tickSpacing;
		int maximizedParamsOffset;
		int textRectHeight;

		double rangeMargin;
		double pointRadius;
		double maximizedPointMagnification;
		int defaultGridDimensions;
		int defaultMaxBtnSz;

		long paramTextOffset;

		double previewBorderWidth;
		QColor previewBorderColor;

		QColor selectionPolyColor;
		QColor plotBorderColor;
		QColor tickLineColor;
		QColor tickLabelColor;
		QColor backgroundColor;
		QColor selectionColor;

		SelectionMode selectionMode;
	};

	//Members
public:
	Settings settings;
protected:
	QGLWidget* m_parentWidget;					//!< the parent widget
	iAScatterPlotSelectionHandler * m_splom;	//!< selection/highlight/settings handler (if part of a SPLOM, the SPLOM-parent)
	QRect m_globRect;							//!< plot's rectangle
	QRectF m_locRect;							//!< plot's local drawing rectangle
	QSharedPointer<iASPLOMData> m_splomData;	//!< pointer to SPLOM-parent's data
	int m_paramIndices[2];						//!< indices of plot X, Y parameters
	double m_prX[2];							//!< range of X parameter
	double m_prY[2];							//!< range of Y parameter
	int m_colInd;								//!< index of color-coded parameter
	QSharedPointer<iALookupTable> m_lut;		//!< pointer to SPLOM-parent's lookup table
	QRectF m_maxBtnRect;						//!< rectangle of maximized button
	//zooming, translating
	double m_scale;								//!< transform scale component
	QPointF m_offset;							//!< transform offset component
	QPointF m_prevPos;							//!< used for computing offset component
	//ticks
	int m_numTicks;								//!< number of ticks
	QList<double> m_ticksX;						//!< position of ticks X axis
	QList<double> m_ticksY;						//!< position of ticks Y axis
	//popup
	bool m_isPlotActive;						//!< flag indicating if the plot is active (user hovers mouse over)
	//points
	int m_gridDims[2];							//!< dimensions of subdivision grid (point picking acceleration)
	QList<QList<int>> m_pointsGrid;				//!< grid bins containing point indices
	int m_prevPtInd;							//!< index of previously selected point
	int m_prevInd;								//!< index of previously selected point(-1 if none)
	int m_curInd;								//!< index of currently selected point (-1 if none)
	QGLBuffer * m_pointsBuffer;					//!< OpenGL buffer used for points VBO
	//selection polygon
	QPolygon m_selPoly;							//!< polygon of selection lasso
	QPoint m_selStart;
	//state flags
	bool m_isMaximizedPlot;						//!< flag telling if plot is maximized (bigger plot)
	bool m_isPreviewPlot;						//!< flag telling if plot is previewed (displayed in maximized plot)
};
