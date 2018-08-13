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

#include "iAScatterPlot.h"	// for iAScatterPlot::SelectionMode

#include <vtkVersion.h>
#if (VTK_MAJOR_VERSION >= 8 && defined(VTK_OPENGL2_BACKEND) )
#include <QOpenGLWidget>
#else
#include <QGLWidget>
#endif

class iASPLOMData;
class iAScatterPlotStandaloneHandler;

//! Widget for using a single scatter plot (outside of a SPLOM)
#if (VTK_MAJOR_VERSION >= 8 && defined(VTK_OPENGL2_BACKEND) )
class open_iA_Core_API iAScatterPlotWidget : public QOpenGLWidget
#else
class open_iA_Core_API iAScatterPlotWidget : public QGLWidget
#endif
{
public:
	static const int PaddingTop;
	static const int PaddingRight;
	int PaddingBottom();
	int PaddingLeft();
	static const int TextPadding;
	iAScatterPlotWidget(QSharedPointer<iASPLOMData> data);
	std::vector<size_t> & GetSelection();
	void SetSelection(std::vector<size_t> const & selection);
	void SetPlotColor(QColor const & c, double rangeMin, double rangeMax);
	void SetSelectionColor(QColor const & c);
	void SetSelectionMode(iAScatterPlot::SelectionMode mode);
protected:
	virtual void paintEvent(QPaintEvent * event);
	virtual void resizeEvent(QResizeEvent* event);
	virtual void wheelEvent(QWheelEvent * event);
	virtual void mousePressEvent(QMouseEvent * event);
	virtual void mouseReleaseEvent(QMouseEvent * event);
	virtual void mouseMoveEvent(QMouseEvent * event);
	virtual void keyPressEvent(QKeyEvent * event);
public:
	iAScatterPlot* m_scatterplot;
private:
	QSharedPointer<iASPLOMData> m_data;
	QSharedPointer<iAScatterPlotStandaloneHandler> m_scatterPlotHandler;
	int m_fontHeight, m_maxTickLabelWidth;
};
