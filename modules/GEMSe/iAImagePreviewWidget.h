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
 
#ifndef IA_IMAGE_PREVIEW_WIDGET
#define IA_IMAGE_PREVIEW_WIDGET

//#include "iAImageTypes.h"
#include "iAITKIO.h"
#include "iASlicer.h"

#include <vtkSmartPointer.h>

#include <QWidget>

class iAColorTheme;
class iAConnector;
class iASlicer;

class vtkCamera;
class vtkColorTransferFunction;
class vtkImageData;
class vtkTransform;

class iAImagePreviewWidget: public QWidget
{
	Q_OBJECT
public:
	static const int SliceNumberNotSet = -1;
	iAImagePreviewWidget(QString const & title, QWidget* parent, bool isLabel, vtkCamera* commonCamera, iASlicerMode,
		int labelCount, iAColorTheme const * colorTheme, bool magicLens=false);
	~iAImagePreviewWidget();
	void SetImage(iAITKIO::ImagePointer img, bool empty, bool isLabelImg);
	void SetImage(vtkSmartPointer<vtkImageData> img, bool empty, bool isLabelImg);
	iASlicerMode GetSlicerMode() const;
	void SetSlicerMode(iASlicerMode, int sliceNr, vtkCamera*);
	vtkCamera* GetCamera();
	void SetCamera(vtkCamera* camera);
	vtkImageData * GetImage() const;
	void SetColorTheme(iAColorTheme const * colorTheme);
	int GetSliceNumber() const;
	double GetAspectRatio() const;
	vtkSmartPointer<vtkColorTransferFunction> GetCTF();
	iASlicer* GetSlicer();
public slots:
	void UpdateView();
	void SetSliceNumber(int sliceNr);
signals:
	void Clicked();
	void MouseHover();
	void Updated();
private:
	virtual void resizeEvent(QResizeEvent * event);
	virtual QSize sizeHint() const;
	void InitializeSlicer();
	void UpdateImage();
	void BuildCTF();

	bool m_isLabelImage;
	bool m_empty;
	bool m_enableInteractions;
	iASlicer* m_slicer;
	iAConnector* m_conn;
	vtkSmartPointer<vtkImageData> m_imageData;
	vtkSmartPointer<vtkTransform> m_slicerTransform;
	vtkSmartPointer<vtkColorTransferFunction> m_ctf;
	QString m_title;
	vtkCamera* m_commonCamera;
	int m_labelCount;
	int m_sliceNumber;
	iASlicerMode m_mode;
	double m_aspectRatio;
	iAColorTheme const * m_colorTheme;
private slots:
	void SlicerClicked();
	void SlicerHovered(int x, int y, int z, int mode);
};

#endif // IA_IMAGE_PREVIEW_WIDGET
