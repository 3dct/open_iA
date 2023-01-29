// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAguibase_export.h"

#include "iAQVTKWidget.h"

#include <vtkSmartPointer.h>

class vtkActor2D;
class vtkCamera;
class vtkInteractorStyle;
class vtkRenderer;

class iAguibase_API iAAbstractMagicLensWidget : public iAQVTKWidget
{
	Q_OBJECT
public:
	enum ViewMode{
		CENTERED,
		OFFSET
	};
	iAAbstractMagicLensWidget( QWidget * parent = 0 );
	virtual ~iAAbstractMagicLensWidget( );
	void magicLensOn( );
	void magicLensOff( );
	void setLensSize( int sizeX, int sizeY );
	vtkRenderer* getLensRenderer( );
	void setViewMode( ViewMode mode );
	bool isMagicLensEnabled() const;

	void setLensBackground(QColor bgTop, QColor bgBottom);

signals:
	void mouseMoved();
	void touchStart();
	void touchScale(float relScale);

protected:
	void mouseMoveEvent(QMouseEvent * event) override;
	void wheelEvent(QWheelEvent* event) override;
	bool event(QEvent* event) override;
	virtual void updateLens( );
	virtual void updateGUI( );
	void getViewportPoints( double points[4] );

	vtkSmartPointer<vtkRenderer> m_lensRen;
	vtkSmartPointer<vtkRenderer> m_GUIRen;
	vtkSmartPointer<vtkActor2D>  m_GUIActor;
	int                          m_pos[2];
	int                          m_size[2];
	double                       m_halfSize[2];
	ViewMode                     m_viewMode;

private:
	static const double          OFFSET_VAL;
	bool                         m_magicLensEnabled;
};
