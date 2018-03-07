/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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

#include "iAUncertaintyImages.h"  // for vtkImagePointer

#include "iAImageCoordinate.h"

#include <QMap>
#include <QWidget>

class vtkColorTransferFunction;
class vtkLookupTable;
class vtkPiecewiseFunction;

class iAChannelVisualizationData;
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
	void SetDatasets(QSharedPointer<iAUncertaintyImages> imgs, vtkSmartPointer<vtkLookupTable> labelImgLut);
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
	QSharedPointer<iAChannelVisualizationData> m_selectionData;
	int m_curMode;
	QVector<QToolButton*> m_memberButtons;
	QWidget* m_settings;
	int m_slice;
	int newImgID;
	vtkSmartPointer<vtkColorTransferFunction> m_uncertaintyLut;
	vtkSmartPointer<vtkLookupTable> m_labelImgLut;
};
