// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iaguibase_export.h"

#include <vtkSmartPointer.h>

#include <QColor>
#include <QVector>

#include <memory>

class QWidget;

class vtkActor2D;
class vtkCamera;
class vtkGenericOpenGLRenderWindow;
class vtkImageActor;
class vtkImageMapToColors;
class vtkImageReslice;
class vtkPolyData;
class vtkPolyDataMapper2D;
class vtkProp;
class vtkRenderer;
class vtkScalarsToColors;
class vtkTextActor;
class vtkWindowToImageFilter;

class iALensData;

//! Provides magic lens functionality for a given vtk render window,
//! which can have up to eight lenses into other datasets.
class iAguibase_API iAMagicLens
{
public:
	enum ViewMode{
		CENTERED,
		OFFSET,
	};
	static const double DefaultFrameWidth;
	static const int OffsetModeXOffset;

	iAMagicLens(QColor const& bgColor);
	void setRenderWindow(vtkGenericOpenGLRenderWindow* renderWindow);
	void updatePosition(vtkCamera * cam, double const lensPos[3], int const mousePos[2]);
	void updateColors();
	void addInput(vtkImageReslice * reslicer, vtkScalarsToColors * cTF, QString const & name);
	//! shows or hides the magic lens
	void setEnabled( bool isEnabled );
	bool isEnabled();
	void setViewMode(ViewMode mode);
	ViewMode viewMode() const;
	void setLensCount(int count);
	void setSize(int newSize);
	int size() const;
	void setInterpolate(bool on);
	void setFrameWidth(qreal frameWidth);
	qreal frameWidth() const;
	void setOpacity(double opacity);
	double opacity() const;
	void setSrcWindowEnabled(bool enabled);
	void render();
	void setBackgroundColor(QColor const& bgColor);

private:
	QVector<std::shared_ptr<iALensData>> m_lenses;
	bool m_isEnabled;
	bool m_isInitialized;
	int m_maxLensCount;
	int m_size;
	qreal m_frameWidth;
	bool m_interpolate;
	ViewMode m_viewMode;
	double m_opacity;
	QColor m_bgColor;
	vtkGenericOpenGLRenderWindow* m_renderWindow;
	vtkSmartPointer<vtkPolyData> m_srcWindowData;
	vtkSmartPointer<vtkPolyDataMapper2D> m_srcWindowMapper;
	vtkSmartPointer<vtkActor2D> m_srcWindowActor;
	vtkSmartPointer<vtkRenderer> m_srcWindowRenderer;

	void updateOffset();
};
