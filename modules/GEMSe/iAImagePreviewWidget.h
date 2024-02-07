// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iASlicer.h>

#include <iAITKIO.h>

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
	void SlicerHovered(double x, double y, double z, int mode);
};
