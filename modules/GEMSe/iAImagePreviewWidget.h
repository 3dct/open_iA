/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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

#include <io/iAITKIO.h>
#include <iASlicer.h>

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
	static const int SliceNumberNotSet;
	iAImagePreviewWidget(QString const & title, QWidget* parent, bool isLabel, vtkCamera* commonCamera, iASlicerMode,
		int labelCount, bool magicLens=false);
	~iAImagePreviewWidget();
	void setImage(iAITKIO::ImagePointer img, bool empty, bool isLabelImg);
	void setImage(vtkSmartPointer<vtkImageData> img, bool empty, bool isLabelImg);
	void addNoMapperChannel(vtkSmartPointer<vtkImageData> img);
	void removeChannel();
	iASlicerMode slicerMode() const;
	void setSlicerMode(iASlicerMode, int sliceNr, vtkCamera*);
	vtkCamera* camera();
	void setCamera(vtkCamera* camera);
	void resetCamera();
	vtkImageData * image() const;
	void setColorTheme(iAColorTheme const * colorTheme);
	int sliceNumber() const;
	double aspectRatio() const;
	vtkSmartPointer<vtkColorTransferFunction> colorTF();
	iASlicer* slicer();
	bool empty() const;

public slots:
	void updateView();
	void setSliceNumber(int sliceNr);

signals:
	void clicked();
	void rightClicked();
	void mouseHover();
	void updated();

private:
	void resizeEvent(QResizeEvent * event) override;
	QSize sizeHint() const override;
	void updateImage();
	bool buildCTF();

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
	vtkSmartPointer<vtkImageActor> m_addChannelImgActor;

private slots:
	void SlicerRightClicked(int x, int y, int z);
	void SlicerHovered(int x, int y, int z, int mode);
};
