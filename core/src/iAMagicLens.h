/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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

#include "iAFramedQVTKWidget2.h"
#include "open_iA_Core_export.h"

#include <QContiguousCache>
#include <QImage>

class QWidget;
class QVTKWidget2;

class vtkCamera;
class vtkGenericOpenGLRenderWindow;
class vtkImageActor;
class vtkImageMapToColors;
class vtkImageReslice;
class vtkProp;
class vtkRenderer;
class vtkScalarsToColors;
class vtkTextActor;
class vtkWindowToImageFilter;


class LensData
{
public:
	LensData();
	LensData(QWidget * parent, const QGLWidget * shareWidget, Qt::WindowFlags f, bool interpolate);

	iAFramedQVTKWidget2 * m_qvtkWidget;
	vtkSmartPointer<vtkImageMapToColors> m_imageToColors;
	vtkSmartPointer<vtkImageActor> m_imageActor;
	vtkSmartPointer<vtkImageMapToColors> m_bgImageToColors;
	vtkSmartPointer<vtkImageActor> m_bgImageActor;
	vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_renWnd;
	vtkSmartPointer<vtkRenderer> m_ren;
	vtkSmartPointer<vtkCamera> m_cam;
	vtkSmartPointer<vtkTextActor> m_textActor;
	int m_offset[2];
	QRect m_viewedRect;	//rect of the area of data actually displayed using m-lens
	int m_idx;
};

class open_iA_Core_API iAMagicLens
{
public:
	enum ViewMode{
		CENTERED,
		OFFSET,
		SIDE_BY_SIDE,
	};
	static const int OFFSET_MODE_X_OFFSET;

	iAMagicLens();
	~iAMagicLens();

	void InitWidget(QWidget * parent = NULL, const QGLWidget * shareWidget=0, Qt::WindowFlags f = 0);
	void SetGeometry(QRect & rect);
	void SetEnabled( bool isEnabled );
	bool Enabled();
	void Render();
	void Frame();
	void SetScaleCoefficient(double scaleCoefficient);
	void UpdateCamera(double * focalPt, vtkCamera * cam);
	void Repaint();
	void SetPaintingLocked(bool isLocked);
	void SetViewMode(ViewMode mode);
	ViewMode GetViewMode() const;
	QRect GetViewRect() const;
	int GetCenterSplitOffset() const;
	int GetSplitOffset() const;
	void SetLensCount(int count);
	void AddInput(vtkImageReslice * reslicer, vtkScalarsToColors * cTF, vtkImageReslice * bgReslice, vtkScalarsToColors* bgCTF);
	int GetSize() const;
	int GetOffset() const;
	void SetSize(int newSize);
	void UpdateColors();
	void SetInterpolate(bool on);
	void SetCaption(std::string const & caption);
	void SetFrameWidth(qreal frameWidth);
	qreal GetFrameWidth() const;
	void SetOpacity(double opacity);
	double GetOpacity();

protected:
	QContiguousCache<LensData> m_lenses;
	double m_scaleCoefficient;
	bool m_isEnabled;
	ViewMode m_viewMode;
	QRect m_viewedRect;	//rect of the area of data actually displayed using m-lens
	float m_splitPosition; //position of the split line in the SIDE_BY_SIDE mode, [0,1]
	bool m_isInitialized;
	int m_lensCount;
protected:
	void SetShowFrame( iAFramedQVTKWidget2::FrameStyle frameStyle );
private:
	int m_size;
	static const int DEFAULT_SIZE;
	bool m_interpolate;
	QWidget * m_parent;
	const QGLWidget * m_shareWidget;
	Qt::WindowFlags m_flags;
	void UpdateOffset();
	void UpdateShowFrame();

};
