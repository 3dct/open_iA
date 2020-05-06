/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
	void setModalities(vtkSmartPointer<vtkImageData> d1, vtkSmartPointer<vtkImageData> d2, vtkSmartPointer<vtkImageData> d3, iABarycentricTriangle triangle);
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