/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
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

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkScatterPlotMatrix.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/


#include "mdichild.h"

#include "vtkChartMatrix.h"
#include "vtkSmartPointer.h" // For ivars
#include "vtkNew.h"          // For ivars
#include "vtkColor.h"        // For member function return
#include "vtkStdString.h"    // For ivars

class QStandardItemModel;

class vtkAxis;
class vtkAnnotationLink;
class vtkRenderWindowInteractor;
class vtkSelection;
class vtkStringArray;
class vtkTable;
class vtkTextProperty;


namespace FiberScout
{
class iAScatterPlotMatrix : public vtkChartMatrix
{
public:
  enum {
    SCATTERPLOT,
    HISTOGRAM,
    ACTIVEPLOT,
    NOPLOT
  };

  vtkTypeMacro(iAScatterPlotMatrix, vtkChartMatrix);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Creates a new object.
  static iAScatterPlotMatrix *New();

  // Description:
  // Perform any updates to the item that may be necessary before rendering.
  virtual void Update();

  // Description:
  // Paint event for the chart matrix.
  virtual bool Paint(vtkContext2D *painter);

  // Description:
  // Set the active plot, the one that will be displayed in the top-right.
  // This defaults to (0, n-2), the plot below the first histogram on the left.
  // \return false is the position specified is not valid.
  virtual bool SetActivePlot(const vtkVector2i& position);

  void SetkMeansMode(bool);

  // Description:
  // Add three columns (RGB) to the the Table and initialize each row with a value of 0.0f.
  void AddColorColumns2Table();

  // Description:
  // Get the position of the active plot.
  virtual vtkVector2i GetActivePlot();

  // Description:
  // Get the AnnotationLink for the scatter plot matrix, this gives you access
  // to the currently selected points in the scatter plot matrix.
  vtkAnnotationLink* GetAnnotationLink();

  // Description:
  // Set the AnnotationLink for the scatter plot matrix.
  void SetSelection(vtkSelection *sel);

  int GetkMeansClusterCount();

  vtkSelection *GetkMeansCluster(int);

  vtkStdString removeUnit(vtkStdString str);

  // Description:
  // Set the input table for the scatter plot matrix. This will cause all
  // columns to be plotted against each other - a square scatter plot matrix.
  virtual void SetInput(vtkTable *table, int fid);

  // Description:
  // Set the visibility of the specified column.
  void SetColumnVisibility(const vtkStdString& name, bool visible);

  // Description:
  // Insert the specified column at the index position of
  // the visible columns.
  void InsertVisibleColumn(const vtkStdString& name, int index);

  // Description:
  // Get the visibility of the specified column.
  bool GetColumnVisibility(const vtkStdString& name);

  // Description:
  // Set the visibility of all columns (true will make them all visible, false
  // will remove all visible columns).
  void SetColumnVisibilityAll(bool visible);

  // Description:
  // Get a list of the columns, and the order in which they are displayed.
  virtual vtkStringArray* GetVisibleColumns();

  // Description:
  // Set the list of visible columns, and the order in which they will be displayed.
  virtual void SetVisibleColumns(vtkStringArray* visColumns);

  // Description:
  // Set the number of bins in the histograms along the central diagonal of the
  // scatter plot matrix.
  virtual void SetNumberOfBins(int numberOfBins);

  // Description:
  // Get the number of bins the histograms along the central diagonal scatter
  // plot matrix. The default value is 10.
  virtual int GetNumberOfBins() const { return this->NumberOfBins; }

  // Description:
  // Set the color for the specified plotType.
  void SetPlotColor(int plotType, const vtkColor4ub& color);

  // Description:
  // Sets the marker style for the specified plotType.
  void SetPlotMarkerStyle(int plotType, int style);

  // Description:
  // Sets the marker size for the specified plotType.
  void SetPlotMarkerSize(int plotType, float size);

  // Description:
  // Return true if the supplied x, y coordinate is inside the item.
  bool Hit(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse move event.
  bool MouseMoveEvent(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse button down event
  bool MouseButtonPressEvent(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse button release event.
  bool MouseButtonReleaseEvent(const vtkContextMouseEvent &mouse);

  // Description:
  // Returns the type of the plot at the given position. The return
  // value is one of: SCATTERPLOT, HISTOGRAM, ACTIVEPLOT, or NOPLOT.
  int GetPlotType(const vtkVector2i &pos);
  int GetPlotType(int row, int column);

  // Description:
  // Set/get the scatter plot title.
  void SetTitle(const vtkStdString& title);
  vtkStdString GetTitle();

  // Description:
  // Set/get the text properties for the chart title, i.e. color, font, size.
  void SetTitleProperties(vtkTextProperty *prop);
  vtkTextProperty* GetTitleProperties();

  // Description:
  // Sets whether or not the grid for the given axis is visible given a plot
  // type, which refers to
  // vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
  void SetGridVisibility(int plotType, bool visible);
  bool GetGridVisibility(int plotType);

  // Description:
  // Sets the background color for the chart given a plot type, which refers to
  // vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
  void SetBackgroundColor(int plotType, const vtkColor4ub& color);
  vtkColor4ub GetBackgroundColor(int plotType);

  // Description:
  // Sets the color for the axes given a plot type, which refers to
  // vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
  void SetAxisColor(int plotType, const vtkColor4ub& color);
  vtkColor4ub GetAxisColor(int plotType);

  // Description:
  // Sets the color for the axes given a plot type, which refers to
  // vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
  void SetGridColor(int plotType, const vtkColor4ub& color);
  vtkColor4ub GetGridColor(int plotType);

  // Description:
  // Sets whether or not the labels for the axes are visible, given a plot type,
  // which refers to
  // vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
  void SetAxisLabelVisibility(int plotType, bool visible);
  bool GetAxisLabelVisibility(int plotType);

  // Description:
  // Set/get the text property for the axis labels of the given plot type,
  // possible types are vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
  void SetAxisLabelProperties(int plotType, vtkTextProperty *prop);
  vtkTextProperty* GetAxisLabelProperties(int plotType);

  // Description:
  // Sets the axis label notation for the axes given a plot type, which refers to
  // vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
  void SetAxisLabelNotation(int plotType, int notation);
  int GetAxisLabelNotation(int plotType);

  // Description:
  // Sets the axis label precision for the axes given a plot type, which refers to
  // vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
  void SetAxisLabelPrecision(int plotType, int precision);
  int GetAxisLabelPrecision(int plotType);

  // Description:
  // Set chart's tooltip notation and precision, given a plot type, which refers to
  // vtkScatterPlotMatrix::{SCATTERPLOT, HISTOGRAM, ACTIVEPLOT}.
  void SetTooltipNotation(int plotType, int notation);
  void SetTooltipPrecision(int plotType, int precision);
  int GetTooltipNotation(int plotType);
  int GetTooltipPrecision(int plotType);

  // Description:
  // Set the scatter plot selected row/column charts' background color.
  void SetScatterPlotSelectedRowColumnColor(const vtkColor4ub& color);
  vtkColor4ub GetScatterPlotSelectedRowColumnColor();

  // Description:
  // Set the scatter plot selected active chart background color.
  void SetScatterPlotSelectedActiveColor(const vtkColor4ub& color);
  vtkColor4ub GetScatterPlotSelectedActiveColor();
  
  // Description:
  // Collectinig object IDs and color information from the 'Parallel Coordinates Class Tree'.
  void UpdateColorInfo(QStandardItemModel *classTree, QList<QColor> colorList);

  // Description:
  // Set the scatter plot histograms visibility. 
  void ShowHideHistograms();

  // Description:
  //Sets the class number (0,1,2,3,...) that will be plotted. -1 plots all classes.  
  void SetClass2Plot(short);

  // Description:
  // Internal helper to do the layout of the charts in the scatter plot matrix.
  void UpdateLayout();

  // Description:
  // Convenient method to update all the chart settings
  void UpdateSettings();

  // Description:
  // Update charts based on settings given the plot type
  void UpdateChartSettings(int plotType);

  // Description:
  // Shows a color class legend with small statistics. 
  void UpdateCustomLegend();

  int NumberOfClusters;

  void setPolygonSelectionOn();
  void setRectangleSelectionOn();
 
  // Description:
  // Set/get the Selection Mode that will be used by the chart while doing
  // selection. The only valid enums are vtkContextScene::SELECTION_NONE,
  // SELECTION_DEFAULT, SELECTION_ADDITION, SELECTION_SUBTRACTION, SELECTION_TOGGLE
  virtual void SetSelectionMode(int);
  vtkGetMacro(SelectionMode, int);

protected:
  iAScatterPlotMatrix();
  ~iAScatterPlotMatrix();

  // Description:
  // Attach axis range listener so we can forward to dependent axes in matrix.
  void AttachAxisRangeListener(vtkAxis*);
  void AxisRangeForwarderCallback(vtkObject*, unsigned long, void*);

  // Description:
  // The callback function when SelectionChangedEvent is invoked from
  // the Big chart. This class will just forward the event.
  void BigChartSelectionCallback(vtkObject*, unsigned long, void*);

  // Description:
  // Given a new position for the active plot, calculate a
  // an animation path from the old active plot to the new
  // active plot.
  virtual void UpdateAnimationPath(const vtkVector2i& newActivePos);

  // Description:
  // Given the render window interactor, start animation of the
  // animation path calculated above.
  virtual void StartAnimation(vtkRenderWindowInteractor* interactor);

  class PIMPL;
  PIMPL *Private;

  MdiChild *activeChild;

  // The position of the active plot (defaults to 0, 1).
  vtkVector2i ActivePlot;

  // Weakly owned input data for the scatter plot matrix.
  vtkSmartPointer<vtkTable> Input;

  // Strongly owned internal data for the column visibility.
  vtkNew<vtkStringArray> VisibleColumns;

  // The number of bins in the histograms.
  int NumberOfBins;

  // The title of the scatter plot matrix.
  vtkStdString Title;
  vtkSmartPointer<vtkTextProperty> TitleProperties;

  // The mode when the chart is doing selection.
  int SelectionMode;

  //ClassTree and colorList from the parallel coordinates system
  QStandardItemModel *pcClassTree;
  QList<QColor> pcColorList;

  vtkSmartPointer<vtkTable> kMeansTable;

  vtkSmartPointer<vtkTable> CurrentClassTable;

  int m_selectionTool;

  //map which olds the class tables when multi-view rendering
  //std::map <int, vtkTable *> tablelist;

private:
  int filterID;
  iAScatterPlotMatrix(const iAScatterPlotMatrix &); // Not implemented.
  void operator=(const iAScatterPlotMatrix &); // Not implemented.
};

} // namespace FiberScout

