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

// source: https://stackoverflow.com/questions/26944831/using-qvtkwidget-and-qopenglwidget-in-the-same-ui

#include "open_iA_core_export.h"

#include "vtkSmartPointer.h"
#include "vtkGenericOpenGLRenderWindow.h"
#include "vtkEventQtSlotConnect.h"

#include "QVTKInteractorAdapter.h"
#include "QVTKInteractor.h"

#include <QOpenGLWidget>
#include <QSurfaceFormat>

class open_iA_Core_API QVTKOpenGLWidget : public QOpenGLWidget
{
	Q_OBJECT

public:
	QVTKOpenGLWidget(QWidget *parent = NULL, Qt::WindowFlags f = 0, QSurfaceFormat format = QSurfaceFormat::defaultFormat());
	virtual ~QVTKOpenGLWidget();

	//! Set a custom render window
	virtual void SetRenderWindow(vtkGenericOpenGLRenderWindow*);
	//! Returns the current render window (creates one if none exists)
	virtual vtkGenericOpenGLRenderWindow* GetRenderWindow();

	//! Returns interactor of the current render window
	virtual QVTKInteractor* GetInteractor();

	public slots:
	//! Slot to make this vtk render window current
	virtual void MakeCurrent();
	//! Slot called when vtk wants to know if the context is current
	virtual void IsCurrent(vtkObject* caller, unsigned long vtk_event, void* client_data, void* call_data);
	//! Slot called when vtk wants to frame the window
	virtual void Frame();
	//! Slot called when vtk wants to start the render
	virtual void Start();
	//! Slot called when vtk wants to end the render
	virtual void End();
	//! Slot called when vtk wants to know if a window is direct
	virtual void IsDirect(vtkObject* caller, unsigned long vtk_event, void* client_data, void* call_data);
	//! Slot called when vtk wants to know if a window supports OpenGL
	virtual void SupportsOpenGL(vtkObject* caller, unsigned long vtk_event, void* client_data, void* call_data);

protected:
	//! Initialize handler
	virtual void initializeGL();
	//! Paint handler
	virtual void paintGL();
	//! Resize handler
	virtual void resizeGL(int, int);
	//! Move handler
	virtual void moveEvent(QMoveEvent* event);

	virtual void mousePressEvent(QMouseEvent* event);
	virtual void mouseMoveEvent(QMouseEvent* event);
	virtual void mouseReleaseEvent(QMouseEvent* event);
	virtual void keyPressEvent(QKeyEvent* event);
	virtual void keyReleaseEvent(QKeyEvent* event);
	virtual void enterEvent(QEvent*);
	virtual void leaveEvent(QEvent*);
	virtual void wheelEvent(QWheelEvent*);

	virtual void contextMenuEvent(QContextMenuEvent*);
	virtual void dragEnterEvent(QDragEnterEvent*);
	virtual void dragMoveEvent(QDragMoveEvent*);
	virtual void dragLeaveEvent(QDragLeaveEvent*);
	virtual void dropEvent(QDropEvent*);

	virtual bool focusNextPrevChild(bool);

	// Members
	vtkGenericOpenGLRenderWindow* m_renWin;
	QVTKInteractorAdapter* m_irenAdapter;
	vtkSmartPointer<vtkEventQtSlotConnect> m_connect;

private:
	//! unimplemented operator=
	QVTKOpenGLWidget const& operator=(QVTKOpenGLWidget const&);
	//! unimplemented copy
	QVTKOpenGLWidget(const QVTKOpenGLWidget&);
};
