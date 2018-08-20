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

#include <QWidget>
#include <QStackedLayout>

#include "mdichild.h"
#include "BCoord.h"

// Slicer
#include "iASimpleSlicerWidget.h"

class iAHistogramStack : public QWidget
{
	Q_OBJECT

public:
	iAHistogramStack(QWidget* parent, MdiChild *mdiChild, Qt::WindowFlags f = 0);
	~iAHistogramStack();

	void setWeight(BCoord bCoord);
	void setSlicerMode(iASlicerMode slicerMode, int dimensionLength);
	void setSliceNumber(int sliceNumber);
	void setModalityLabel(QString label, int index);
	bool containsModality(QSharedPointer<iAModality> modality);
	int modalitiesCount();

	QSharedPointer<iAModality> getModality(int index);
	double getWeight(int index);

	void updateModalities();

	bool isReady();

private slots:
	void updateTransferFunction1() { updateTransferFunction(0); }
	void updateTransferFunction2() { updateTransferFunction(1); }
	void updateTransferFunction3() { updateTransferFunction(2); }

signals:
	void transferFunctionChanged();
	void modalitiesChanged(QSharedPointer<iAModality> modality1, QSharedPointer<iAModality> modality2, QSharedPointer<iAModality> modality3);

protected:
	void resizeEvent(QResizeEvent* event);

private:
	void updateTransferFunction(int index);

	BCoord m_weightCur;
	void updateTransferFunctions(int index);
	void applyWeights();

	void adjustStretch(int totalWidth);

	void disable();
	void enable();

	QSharedPointer<iAModality> m_modalitiesActive[3];
	vtkSmartPointer<vtkPiecewiseFunction> m_opFuncsCopy[3];
	void createOpFuncCopy(int index);
	void deleteOpFuncCopy(int index);

	double m_slicerXYopacity, m_slicerXZopacity, m_slicerYZopacity;

	// Widgets and stuff
	QStackedLayout *m_stackedLayout;
	QLabel *m_disabledLabel;
	QGridLayout *m_gridLayout;
	QLabel *m_weightLabels[3];
	QLabel *m_modalityLabels[3];
	iASimpleSlicerWidget *m_slicerWidgets[3];
	iADiagramFctWidget* m_histograms[3] = { 0, 0, 0 };

	// TODO: remove
	MdiChild *m_mdiChild;
	
};
