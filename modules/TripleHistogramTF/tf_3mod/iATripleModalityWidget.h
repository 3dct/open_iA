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

#include "iAMultimodalWidget.h"

#include "iAHistogramAbstract.h"
#include "BCoord.h"

#include <iASlicerMode.h>

#include <vtkSmartPointer.h>

class iABarycentricTriangleWidget;
class iASimpleSlicerWidget;
class iABarycentricContextRenderer;

class iADiagramFctWidget;
class iAModality;
class iATransferFunction;
class MdiChild;

class vtkColorTransferFunction;
class vtkPiecewiseFunction;

class QComboBox;
class QLabel;
class QSlider;
class QSpinBox;

class iATripleModalityWidget : public iAMultimodalWidget
{
	Q_OBJECT

public:
	iATripleModalityWidget(QWidget* parent, MdiChild *mdiChild, Qt::WindowFlags f = 0);
	~iATripleModalityWidget();

	iAHistogramAbstractType getLayoutTypeAt(int comboBoxIndex);
	void setHistogramAbstractType(iAHistogramAbstractType type);

	iABarycentricTriangleWidget* w_triangle() {
		return m_triangleWidget;
	}

	QComboBox* w_layoutComboBox() {
		return m_layoutComboBox;
	}

private slots:
	void layoutComboBoxIndexChanged(int newIndex);
	void triangleWeightChanged(BCoord newWeights);
	void weightsChangedSlot(BCoord newWeights);
	void modalitiesLoaded_beforeUpdateSlot();

private:
	QComboBox *m_layoutComboBox;
	void setLayoutTypePrivate(iAHistogramAbstractType type);

	iABarycentricTriangleWidget *m_triangleWidget;
	iABarycentricContextRenderer *m_triangleRenderer;

	iAHistogramAbstract *m_histogramAbstract = nullptr;
	iAHistogramAbstractType m_histogramAbstractType;

	void updateModalities();
};