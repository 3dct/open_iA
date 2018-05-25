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

#include <vtkSmartPointer.h>

#include <QContiguousCache>
#include <QImage>

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

class open_iA_Core_API iAMagicLens
{
public:
	enum ViewMode{
		CENTERED,
		OFFSET,
	};
	static const int DefaultSize;
	static const double DefaultFrameWidth;
	static const int OffsetModeXOffset;

	iAMagicLens();
	void SetRenderWindow(vtkGenericOpenGLRenderWindow* renderWindow);
	void UpdatePosition(vtkCamera * cam, double const lensPos[3], int const mousePos[2]);
	void UpdateColors();
	void AddInput(vtkImageReslice * reslicer, vtkScalarsToColors * cTF, QString const & name);
	//! shows or hides the magic lens
	void SetEnabled( bool isEnabled );
	bool IsEnabled();
	void SetViewMode(ViewMode mode);
	ViewMode GetViewMode() const;
	void SetLensCount(int count);
	void SetSize(int newSize);
	int GetSize() const;
	void SetInterpolate(bool on);
	void SetFrameWidth(qreal frameWidth);
	qreal GetFrameWidth() const;
	void SetOpacity(double opacity);
	double GetOpacity() const;
	void SetSrcWindowEnabled(bool enabled);
	void Render();
private:
	QVector<QSharedPointer<iALensData>> m_lenses;
	bool m_isEnabled;
	QRect m_viewedRect;	//! @< rect of the area of data actually displayed using m-lens
	bool m_isInitialized;
	int m_maxLensCount;
	int m_size;
	qreal m_frameWidth;
	bool m_interpolate;
	ViewMode m_viewMode;
	double m_opacity;
	vtkGenericOpenGLRenderWindow* m_renderWindow;
	vtkSmartPointer<vtkPolyData> m_srcWindowData;
	vtkSmartPointer<vtkPolyDataMapper2D> m_srcWindowMapper;
	vtkSmartPointer<vtkActor2D> m_srcWindowActor;
	vtkSmartPointer<vtkRenderer> m_srcWindowRenderer;
	void UpdateOffset();
};
