// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "iAguibase_export.h"

#include "iAQVTKWidget.h"

#include <vtkSmartPointer.h>

class vtkActor2D;
class vtkCamera;
class vtkInteractorStyle;
class vtkPolyDataMapper2D;
class vtkRenderer;

//! Base class for 3D content rendering, main functionality currently:
//!     - The ability to show a 3D magic lens where the camera of the lens adapts to the camera of the "main" view.
//!     - Touch-based pinch-zoom
//!		- context menu for triggering showing settings and switching magic lens modes
//! Should probably be merged with iARendererImpl
class iAguibase_API iAFast3DMagicLensWidget : public iAQVTKWidget
{
	Q_OBJECT
public:
	enum ViewMode {
		CENTERED,
		OFFSET
	};
	iAFast3DMagicLensWidget(QWidget * parent = nullptr);
	virtual ~iAFast3DMagicLensWidget( );
	void magicLensOn();
	void magicLensOff();
	void setLensSize(int sizeX, int sizeY);
	vtkRenderer* getLensRenderer();
	void setViewMode(ViewMode mode);
	bool isMagicLensEnabled() const;
	void setLensBackground(QColor bgTop, QColor bgBottom);
	void setContextMenuEnabled(bool enabled);

signals:
	void rightButtonReleasedSignal();
	void leftButtonReleasedSignal();
	void mouseMoved();
	void touchStart();
	void touchScale(float relScale);
	void editSettings();

protected:
	void resizeEvent( QResizeEvent * event ) override;
	void mouseReleaseEvent(QMouseEvent * event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void wheelEvent(QWheelEvent* event) override;
	bool event(QEvent* event) override;
	void contextMenuEvent(QContextMenuEvent* event) override;

	virtual void updateLens();
	virtual void updateGUI();
	void getViewportPoints(double points[4]);

private:
	double calculateZ( double viewAngle );

	static const double          OFFSET_VAL;
	vtkSmartPointer<vtkRenderer> m_lensRen;
	vtkSmartPointer<vtkPolyDataMapper2D> m_GUIMapper;
	vtkSmartPointer<vtkActor2D>  m_GUIActor;
	vtkSmartPointer<vtkRenderer> m_GUIRen;
	ViewMode                     m_viewMode;
	double                       m_viewAngle;
	bool                         m_magicLensEnabled;
	int                          m_pos[2];
	int                          m_size[2];
	double                       m_halfSize[2];
	bool                         m_contextMenuEnabled;
};
