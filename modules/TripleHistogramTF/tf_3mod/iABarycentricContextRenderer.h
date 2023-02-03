// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iABarycentricTriangle.h"

#include <vtkSmartPointer.h>
#include <vtkImageData.h>

#include <QWidget>

class QImage;
class QRect;
class QTimer;

class iABarycentricContextRenderer : public QWidget
{
	Q_OBJECT

public:
	iABarycentricContextRenderer();
	void setData(vtkSmartPointer<vtkImageData> d1, vtkSmartPointer<vtkImageData> d2, vtkSmartPointer<vtkImageData> d3, iABarycentricTriangle triangle);
	void setTriangle(iABarycentricTriangle triangle);
	QImage* getImage();
	QRect getImageRect();

private:
	void calculateCoordinates(vtkSmartPointer<vtkImageData> d1, vtkSmartPointer<vtkImageData> d2, vtkSmartPointer<vtkImageData> d3);
	void updateTriangle(iABarycentricTriangle triangle);
	void drawImageLater();
	void drawImageNow();

	vtkSmartPointer<vtkImageData> m_barycentricCoordinates;
	QImage *m_image;
	QRect m_imageRect;
	iABarycentricTriangle m_triangle;

	QTimer *m_timer_heatmap;
	int m_timerWait_heatmap;

signals:
	void heatmapReady();

private slots:
	void onHeatmapTimeout();
};
