// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAUncertaintyImages.h"  // for vtkImagePointer

#include <iAImageCoordinate.h>

#include <QMap>
#include <QWidget>

class vtkColorTransferFunction;
class vtkLookupTable;
class vtkPiecewiseFunction;

class iAChannelData;
class iAImageWidget;

class QToolBar;
class QToolButton;
class QSpinBox;

struct ImageData
{
	ImageData();
	ImageData(QString const & c, vtkImagePointer img);
	QString caption;
	vtkImagePointer image;
};
struct ImageGUIElements
{
	ImageGUIElements();
	void DeleteAll();
	iAImageWidget* imageWidget;
	QWidget* container;
	bool m_selectionChannelInitialized;
};

class iASpatialView: public QWidget
{
	Q_OBJECT
public:
	iASpatialView();
	void SetDatasets(std::shared_ptr<iAUncertaintyImages> imgs, vtkSmartPointer<vtkLookupTable> labelImgLut);
	void AddMemberImage(QString const & caption, vtkImagePointer img, bool keep);
	void ToggleSettings();
	void SetupSelection(vtkImagePointer selectionImg);
public slots:
	void StyleChanged();
	void UpdateSelection();
private slots:
	void SlicerModeButtonClicked(bool checked);
	void SliceChanged(int);
	void ImageButtonClicked();
/*
signals:
	void ROISelected(iAImageCoordinate topLeftFront, iAImageCoordinate bottomRightBack);
*/
private:
	QToolButton* AddImage(QString const & caption, vtkImagePointer img);
	void AddImageDisplay(int idx);
	void RemoveImageDisplay(int idx);
	QMap<int, ImageData> m_images;
	QMap<int, ImageGUIElements> m_guiElements;
	QWidget* m_contentWidget;
	QWidget* m_sliceBar;
	QWidget* m_imageBar;
	QSpinBox* m_sliceControl;
	QVector<QToolButton*> slicerModeButton;
	vtkSmartPointer<vtkLookupTable> m_ctf;
	vtkSmartPointer<vtkPiecewiseFunction> m_otf;
	std::shared_ptr<iAChannelData> m_selectionData;
	int m_curMode;
	QVector<QToolButton*> m_memberButtons;
	QWidget* m_settings;
	int m_slice;
	int newImgID;
	vtkSmartPointer<vtkColorTransferFunction> m_uncertaintyLut;
	vtkSmartPointer<vtkLookupTable> m_labelImgLut;
};
