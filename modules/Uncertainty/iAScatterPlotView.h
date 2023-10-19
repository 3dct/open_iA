// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAUncertaintyImages.h"  // for vtkImagePointer

#include <QWidget>

class iAScatterPlotWidget;
class iAvtkImageData;

class iAScatterPlotView: public QWidget
{
	Q_OBJECT
public:
	iAScatterPlotView();
	void SetDatasets(std::shared_ptr<iAUncertaintyImages> imgs);
	vtkImagePointer GetSelectionImage();
	void ToggleSettings();
private slots:
	void XAxisChoice();
	void YAxisChoice();
	void SelectionUpdated();
signals:
	void SelectionChanged();
private:
	std::shared_ptr<iAUncertaintyImages> m_imgs;

	vtkSmartPointer<iAvtkImageData> m_selectionImg;
	size_t m_voxelCount;
	QWidget* m_xAxisChooser, * m_yAxisChooser;
	int m_xAxisChoice, m_yAxisChoice;
	QWidget* m_settings;
	QWidget* m_scatterPlotContainer;
	iAScatterPlotWidget* m_scatterPlotWidget;

	void AddPlot(vtkImagePointer imgX, vtkImagePointer imgY, QString const & captionX, QString const & captionY);
};
