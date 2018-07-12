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

#include <vtkVersion.h>
#if (VTK_MAJOR_VERSION >= 8 && defined(VTK_OPENGL2_BACKEND) )
#include <QVTKOpenGLWidget.h>
#else
#include <QVTKWidget.h>
#endif

#include <QMouseEvent>

/**
 * \brief	Qvtk widget which emits signals when button released.
 *
 * Solution for a "non-bug" in VTK http://www.vtk.org/pipermail/vtkusers/2013-December/082291.html
 * which will not get fixed.
 */
#if (VTK_MAJOR_VERSION >= 8 && defined(VTK_OPENGL2_BACKEND) )
class open_iA_Core_API QVTKWidgetMouseReleaseWorkaround : public QVTKOpenGLWidget
#else
class open_iA_Core_API QVTKWidgetMouseReleaseWorkaround : public QVTKWidget
#endif
{
	Q_OBJECT
public:
	QVTKWidgetMouseReleaseWorkaround(QWidget* parent = NULL, Qt::WindowFlags f = 0);
protected:
	virtual void mouseReleaseEvent ( QMouseEvent * event );
	virtual void resizeEvent ( QResizeEvent * event );
Q_SIGNALS:
	void rightButtonReleasedSignal();
	void leftButtonReleasedSignal();
};
