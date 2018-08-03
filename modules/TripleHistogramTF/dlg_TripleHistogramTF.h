/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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

//#include "ui_dlg_TripleHistogramTF.h"
//#include "iAQTtoUIConnector.h"
#include <QDockWidget>
#include "iABarycentricTriangleWidget.h"

#include <QResizeEvent>
#include <qcombobox.h>
#include <qslider.h>

#include "iAModalityWidget.h"
//class BCoord; // TODO: shouldn't this work?
#include "BCoord.h"

//class MdiChild; // TODO: shouldn't this work?
#include "mdichild.h"
#include "iASlicerMode.h"
#include "vtkImageData.h" // TODO: really necessary?

//typedef iAQTtoUIConnector<QDockWidget, Ui_dlg_TripleHistogramTF> TripleHistogramTFConnector;

class dlg_TripleHistogramTF : public QDockWidget//public TripleHistogramTFConnector
{
	Q_OBJECT

public:
	dlg_TripleHistogramTF(MdiChild* parent, Qt::WindowFlags f = 0);
	~dlg_TripleHistogramTF();

public slots:
	//void mousePress(QMouseEvent*);
	//void mouseMove(QMouseEvent*);
	//void mouseWheel(QWheelEvent*);
	void setWeight(BCoord bCoord);
	void updateSlicerMode();
	void setSliceNumber(int sliceNumber);

signals:
	

protected:
	void resizeEvent(QResizeEvent* event);

private:
	void setSlicerMode(iASlicerMode slicerMode);

	QComboBox *m_slicerModeComboBox;
	QSlider *m_sliceSlider;

	// TODO: is it really good to keep the mdiChild as a member variable?
	MdiChild *m_mdiChild;

	iAModalityWidget *m_modality1, *m_modality2, *m_modality3;
	iABarycentricTriangleWidget *m_triangleWidget;

	QHBoxLayout *m_mainLayout;
	
};
