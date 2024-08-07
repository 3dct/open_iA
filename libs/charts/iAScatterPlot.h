// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#ifdef CHART_OPENGL
#include <QOpenGLWidget>
#ifdef SP_OLDOPENGL
#include <QOpenGLBuffer>
#endif
using iAChartParentWidget = QOpenGLWidget;
#else
#include <QWidget>
using iAChartParentWidget = QWidget;
#endif

#include "iacharts_export.h"

#include <QList>
#include <QObject>

class iAColorTheme;
class iALookupTable;
class iAScatterPlotViewData;
class iASPLOMData;

class QTimer;

class vtkLookupTable;

//! Represents a single scatter plot in the scatter plot matrix (SPLOM).
//! The class is managed from the parent SPLOM.
//! Draws its contents in a given rectangle on a parent SPLOM widget.
//! Appearance can be customized via the public settings member.
class iAcharts_API iAScatterPlot : public QObject
{
	Q_OBJECT
public:
	enum SelectionMode
	{ // the order here needs to match the order in the cbSelectionMode combobox in SPMSettings dialog!
		Rectangle,
		Polygon
	};
	enum HighlightDrawMode
	{
		Enlarged = 1,         //!< whether to enlarge highlighted point
		CategoricalColor = 2, //!< if set, use categorical color to draw highlighted point
		Outline  = 4,         //!< if set, use categorical color for an outline around the actual point; using Enlarged, CategoricalColor AND Outline is redundant, only Enlarged and CategoricalColor will have the same effect
	};
	Q_DECLARE_FLAGS(HighlightDrawModes, HighlightDrawMode)
	//! Constructor, initializes some core members
	//! @param spViewData data on the current viewing configuration
	//! @param parent the parent widget
	//! @param numTicks the number of ticks in x and y direction
	//! @param isMaximizedPlot whether this is a maximized plot
	iAScatterPlot(iAScatterPlotViewData * spViewData, iAChartParentWidget* parent, int numTicks = 5, bool isMaximizedPlot = false);
	~iAScatterPlot();

	void setData(size_t x, size_t y, std::shared_ptr<iASPLOMData> &splomData ); //!< Set data to the scatter plot using indices of X and Y parameters and the raw SPLOM data
	void setIndices(size_t x, size_t y);                             //!< Set the indices of the parameters to view
	bool hasData() const;                                            //!< Check if data is already set to the plot
	//! Set color lookup table and the name of a color-coded parameter
	void setLookupTable( std::shared_ptr<iALookupTable> &lut, size_t colInd );
	std::shared_ptr<iALookupTable> lookupTable() const;
	const size_t* getIndices() const { return m_paramIndices; }      //!< Get indices of X and Y parameters
	void setTransform( double scale, QPointF newOffset );            //!< Set new transform: new scale and new offset
	void setTransformDelta( double scale, QPointF deltaOffset );     //!< Set new transform: new scale and change in the offset (delta)
	QRect getRect() const { return m_globRect; }                     //!< Get rectangle where scatter plot contents are displayed
	void setRect( QRect val );                                       //!< Set rectangle where scatter plot contents are displayed
	double getScale() const { return m_scale; }                      //!< Get current scale applied to the plot points
	QPointF getOffset() const { return m_offset; }                   //!< Get current offset applied to the plot points
	QPointF getPointPosition( size_t index ) const;                  //!< Get global position of a point by its index
	double getPointRadius() const;                                   //!< Get point radius (magnified if plot is maximized)
	void setPointRadius(double radius);                              //!< Set point radius (magnified if plot is maximized)

	//!  Output positions and labels of plot ticks for X and Y axes
	void printTicksInfo( QList<double> * posX, QList<double> * posY, QList<QString> * textX, QList<QString> * textY ) const;
	void setCurrentPoint( size_t index );                            //!< Set the index of currently hovered point
	size_t getCurrentPoint() const;                                  //!< Get the index of currently hovered point
	size_t getPreviousIndex() const;                                 //!< Get the index of previously hovered point or iASPLOMData::NoDataIdx
	size_t getPreviousPoint() const;                                 //!< Get the index of point hovered over before previous
	void paintOnParent( QPainter & painter );                        //!< Paint plot's contents on a SPLOM-parent
	void setPreviewState( bool isPreviewPlot );                      //!< Set if this plot is currently previewed (displayed in maximized plot view)
	void leave();                                                    //!< Mouse is hovering over the plot's rectangle
	void enter();                                                    //!< Mouse entered the plot's rectangle
	void updatePoints();
	void applyMarginToRanges();                                      //!< Apply margins to ranges so that points are not stretched border-to-border

	//! @{ Qt events are redirected from SPLOM to the active plot using these public event handlers
	void SPLOMWheelEvent( QWheelEvent * event );
	void SPLOMMouseMoveEvent( QMouseEvent * event );
	void SPLOMMousePressEvent( QMouseEvent * event );
	void SPLOMMouseReleaseEvent( QMouseEvent * event );
	//! @}
	void setSelectionColor(QColor selCol);
	void setHighlightColor(QColor hltCol);
	void setHighlightColorTheme(iAColorTheme const* theme);
	void setHighlightDrawMode(HighlightDrawModes drawMode);

	double p2x( double pval ) const;                                 //!< Parameter scalar value to X coordinate (pixels)
	double const* yBounds() const;
	void setYBounds(double yMin, double yMax);
	void resetYBounds();

protected:
	int p2binx( double p ) const;                                    //!< Get grid bin index using parameter value X
	int p2biny( double p ) const;                                    //!< Get grid bin index using parameter value Y
	double p2tx( double pval ) const;                                //!< Parameter scalar value to normalized X coordinate [0,1]
	double x2p( double x ) const;                                    //!< X coordinate (pixels) to parameter value
	double p2y( double pval ) const;                                 //!< Parameter scalar value to Y coordinate (pixels)
	double p2ty( double pval ) const;                                //!< Parameter scalar value to normalized Y coordinate [0,1]
	double y2p( double y ) const;                                    //!< Y coordinate (pixels) to parameter value
	double applyTransformX( double v ) const;                        //!< Apply scaling and offset to X coordinate
	double revertTransformX( double v ) const;                       //!< Revert scaling and offset to get X coordinate
	double applyTransformY( double v ) const;                        //!< Apply scaling and offset to Y coordinate
	double revertTransformY( double v ) const;                       //!< Revert scaling and offset to get Y coordinate
	void initGrid();                                                 //!< Allocate lists for grid subdivision ( for point-picking acceleration)
	void updateGrid();                                               //!< Fill subdivision grid with points ( for point-picking acceleration)
	void calculateNiceSteps();                                       //!< Calculates nice steps displayed parameter ranges
	void calculateNiceSteps( double * r, QList<double> * ticks );    //!< Calculates nice steps displayed parameter ranges given a range and a desired number of ticks
	int getBinIndex( int x, int y ) const;                           //!< Get global grid bin offset (index) using X and Y bin indices
	size_t getPointIndexAtPosition( QPointF mpos ) const;            //!< Get index of data point under cursor, iASPLOMData::NoDataIdx if none
	QPointF getPositionFromPointIndex( size_t idx ) const;           //!< Get position of a data point with a given index
	void updateSelectedPoints( bool append, bool remove);            //!< Update selected points; parameters specify whether to append or to remove from previous selection (or create new if both false). if both append and remove are true, then XOR logic is applied (of newly selected, those already selected will be de-selected, new ones will be added)
	void updateDrawRect();                                           //!< Re-calculate dimensions of the plot's rectangle
	QPoint getLocalPos( QPoint pos ) const;                          //!< Local (plot) position from global (SPLOM)
	QPoint cropLocalPos( QPoint locPos ) const;                      //!< Make sure that local position is inside plot's rectangle
	//void drawParameterName( QPainter &painter );                   //!< Draws parameter name (only diagonal plots)
	void drawBorder( QPainter &painter );                            //!< Draws plot's border
	void drawTicks( QPainter &painter );                             //!< Draws plot's ticks
	void drawMaximizedLabels( QPainter &painter );                   //!< Draws additional plot's labels (only maximized plot)
	void drawSelectionPolygon( QPainter &painter );                  //!< Draws selection-lasso polygon
	void drawPoints( QPainter &painter );                            //!< Draws plot's points (uses native OpenGL)
#ifdef SP_OLDOPENGL
	void createVBO();                                                //!< Creates and fills VBO with plot's 2D-points.
	void fillVBO();                                                  //!< Fill existing VBO with plot's 2D-points.
#else
	void drawPoint(QPainter& painter, double ptX, double ptY, int radius, QColor const& color);
#endif

signals:
	void selectionModified();                                        //!< Emitted when selected points changed
	void transformModified( double scale, QPointF deltaOffset );     //!< Emitted when user transforms (scales, translates)
	void currentPointModified( size_t index );                       //!< Emitted when hovered over new point
	void chartPress(double x, double y, Qt::KeyboardModifiers modifiers); //!< Emitted when the mouse is pressed in the chart (and no selection or fixed point selection happened)
	void chartClick(double x, double y, Qt::KeyboardModifiers modifiers); //!< Emitted when the mouse button is clicked in the chart

private slots:
	void dataChanged(size_t paramIndex);

public:
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
		QColor highlightColor;
		iAColorTheme const* highlightColorTheme;
		HighlightDrawModes highlightDrawMode;
		SelectionMode selectionMode;
		bool selectionEnabled;
		bool showPCC, showSCC;
		bool drawGridLines;
	};

	// Members
	Settings settings;
protected:
	iAChartParentWidget* m_parentWidget;                             //!< the parent widget
#ifdef SP_OLDOPENGL
	QOpenGLBuffer* m_pointsBuffer;                                   //!< OpenGL buffer used for points VBO
	bool m_pointsOutdated;                                           //!< indicates whether we need to fill the points buffer
#endif
	iAScatterPlotViewData* m_viewData;                               //!< selection/highlight/settings handler (if part of a SPLOM, the SPLOM-parent)
	QRect m_globRect;                                                //!< plot's rectangle
	QRectF m_locRect;                                                //!< plot's local drawing rectangle
	std::shared_ptr<iASPLOMData> m_splomData;                         //!< pointer to SPLOM-parent's data
	size_t m_paramIndices[2];                                        //!< indices of plot X, Y parameters
	double m_prX[2], m_prY[2];                                       //!< range of x and y parameter
	size_t m_colInd;                                                 //!< index of color-coded parameter
	std::shared_ptr<iALookupTable> m_lut;                             //!< pointer to SPLOM-parent's lookup table
	QRectF m_maxBtnRect;                                             //!< rectangle of maximized button
	// zooming, translating
	double m_scale;                                                  //!< transform scale component
	QPointF m_offset;                                                //!< transform offset component
	QPointF m_prevPos;                                               //!< used for computing offset component
	// ticks
	int m_numTicks;                                                  //!< number of ticks
	QList<double> m_ticksX;                                          //!< position of ticks X axis
	QList<double> m_ticksY;                                          //!< position of ticks Y axis
	// popup
	bool m_isPlotActive;                                             //!< flag indicating if the plot is active (user hovers mouse over)
	// points
	int m_gridDims[2];                                               //!< dimensions of subdivision grid (point picking acceleration)
	QList<QList<size_t>> m_pointsGrid;                               //!< grid bins containing point indices
	size_t m_prevPtInd;                                              //!< index of point selected before (iASPLOMData::NoDataIdx if none, but keeps point index even if no point was selected in between)
	size_t m_prevInd;                                                //!< index of previously selected point (iASPLOMData::NoDataIdx if none)
	size_t m_curInd;                                                 //!< index of currently selected point (iASPLOMData::NoDataIdx if none)
	//selection polygon
	QPolygon m_selPoly;                                              //!< polygon of selection lasso
	QPoint m_selStart;                                               //!< point where the selection started
	//state flags
	bool m_isMaximizedPlot;                                          //!< flag telling if this plot itself is maximized (bigger plot)
	bool m_isPreviewPlot;                                            //!< flag telling if a large version of this plot is shown maximized currently
	size_t m_curVisiblePts;                                          //!< number of currently visible points
	bool m_dragging;                                                 //!< indicates whether a drag operation is currently going on
private:
	double scc();
	double pcc();
	QColor highlightColorPoint(size_t i, size_t idx);
	double m_pcc, m_scc;                                             //!< correlation coefficients between the two given data columns
	bool m_pccValid, m_sccValid;                                     //!< indicates whether current cached values cor correlation coefficients can be used
	bool m_useFixedYAxis = false;                                    //!< whether y axis uses a custom range or should be set automatically from data ranges

};

Q_DECLARE_OPERATORS_FOR_FLAGS(iAScatterPlot::HighlightDrawModes);
