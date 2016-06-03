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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 
#ifndef IAMAGICLENS_H
#define IAMAGICLENS_H

#include "iAFramedQVTKWidget2.h"
#include "open_iA_Core_export.h"

#include <QImage>

class QWidget;
class QGLWidget;
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

class open_iA_Core_API iAMagicLens
{
public:
	enum ViewMode{
		CENTERED,
		OFFSET,
		SIDE_BY_SIDE,
	};
	static const int OFFSET_MODE_X_OFFSET;

	iAMagicLens(std::string const & caption = "" );

	~iAMagicLens();

	void InitWidget(QWidget * parent = NULL, const QGLWidget * shareWidget=0, Qt::WindowFlags f = 0);
	void SetGeometry(QRect & rect);
	void SetRenderWindow(vtkGenericOpenGLRenderWindow * renWnd);
	void SetEnabled( bool isEnabled );
	bool Enabled();
	void Render();
	void Frame();
	void SetScaleCoefficient(double scaleCoefficient);
	void UpdateCamera(double * focalPt, vtkCamera * cam);
	void Repaint();
	void SetPaintingLocked(bool isLocked);
	const QObject * GetQObject() const;
	void SetViewMode(ViewMode mode);
	ViewMode GetViewMode() const;
	QRect GetViewRect() const;
	int GetCenterSplitOffset() const;
	int GetSplitOffset() const;
	void SetInput(vtkImageReslice * reslicer, vtkScalarsToColors * cTF, vtkImageReslice * bgReslice, vtkScalarsToColors* bgCTF);
	int GetSize() const;
	int GetOffset(int idx) const;
	void SetSize(int newSize);
	void UpdateColors();
	void SetInterpolate(bool on);
	void SetCaption(std::string const & caption);
	void SetFrameWidth(qreal frameWidth);
	qreal GetFrameWidth() const;
	void SetOpacity(double opacity);
	double GetOpacity();

protected:
	iAFramedQVTKWidget2 * m_qvtkWidget;
	vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_renWnd;
	vtkSmartPointer<vtkRenderer> m_ren;
	vtkSmartPointer<vtkCamera> m_cam;
	double m_scaleCoefficient;
	bool m_isEnabled;
	vtkSmartPointer<vtkWindowToImageFilter> m_w2i;
	int m_offset[2];
	ViewMode m_viewMode;
	QRect m_viewedRect;	//rect of the area of data actually displayed using m-lens
	float m_splitPosition; //position of the split line in the SIDE_BY_SIDE mode, [0,1]
	vtkSmartPointer<vtkImageMapToColors> m_imageToColors;
	vtkSmartPointer<vtkImageActor> m_imageActor;
	vtkSmartPointer<vtkImageMapToColors> m_bgImageToColors;
	vtkSmartPointer<vtkImageActor> m_bgImageActor;
	bool m_isInitialized;
protected:
	void SetShowFrame( iAFramedQVTKWidget2::FrameStyle frameStyle );
private:
	int m_size;
	static const int DEFAULT_SIZE;
	vtkSmartPointer<vtkTextActor> m_textActor;
	void UpdateOffset();
	void UpdateShowFrame();

};

#endif // IAMAGICLENS_H
