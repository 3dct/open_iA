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

#include "tf_3mod/BCoord.h"

#include "iASimpleSlicerWidget.h"

#include <charts/iADiagramFctWidget.h>
#include <iATransferFunction.h>

#include <vtkSmartPointer.h>

#include <QSlider>
#include <QComboBox>
#include <QWidget>
#include <QVector>
#include <QSharedPointer>

class MdiChild;

class vtkColorTransferFunction;
class vtkPiecewiseFunction;
class vtkSmartVolumeMapper;
class vtkRenderer;
class vtkVolume;

class QLabel;
class QStackedLayout;
class QCheckBox;

enum NumOfMod {
	UNDEFINED = -1,
	TWO = 2,
	THREE = 3
};

class iAMultimodalWidget : public QWidget {
	Q_OBJECT

// Private methods used by public/protected
private:
	double bCoord_to_t(BCoord bCoord) { return bCoord[1]; }
	BCoord t_to_bCoord(double t) { return BCoord(1-t, t); }
	void setWeights(BCoord bCoord, double t);

public:
	iAMultimodalWidget(QWidget *parent, MdiChild* mdiChild, NumOfMod num);

	QSharedPointer<iADiagramFctWidget> w_histogram(int i) {
		return m_histograms[i];
	}

	QSharedPointer<iASimpleSlicerWidget> w_slicer(int i) {
		return m_slicerWidgets[i];
	}

	QCheckBox* w_checkBox_weightByOpacity() {
		return m_checkBox_weightByOpacity;
	}

	QLabel* w_slicerModeLabel() {
		return m_slicerModeLabel;
	}

	QLabel* w_sliceNumberLabel() {
		return m_sliceNumberLabel;
	}

	void setWeights(BCoord bCoord) {
		setWeights(bCoord, bCoord_to_t(bCoord));
	}
	void setWeights(double t) {
		setWeights(t_to_bCoord(t), t);
	}
	BCoord getWeights();
	double getWeight(int i);

	void setSlicerMode(iASlicerMode slicerMode);
	iASlicerMode getSlicerMode();
	QString getSlicerModeString();
	void setSliceNumber(int sliceNumber);
	int getSliceNumber();

	QSharedPointer<iAModality> getModality(int index);
	vtkSmartPointer<vtkImageData> getModalityImage(int index);
	int getModalitiesCount();
	bool containsModality(QSharedPointer<iAModality> modality);
	void updateModalities();

	bool isReady();

	void updateTransferFunction(int index);

protected:
	void setWeightsProtected(BCoord bCoord, double t);

	void resetSlicers();
	void resetSlicer(int i);

	void setWeightsProtected(double t) {
		setWeightsProtected(t_to_bCoord(t), t);
	}

	void setWeightsProtected(BCoord bCoord) {
		setWeightsProtected(bCoord, bCoord_to_t(bCoord));
	}

	MdiChild *m_mdiChild;
	QLayout *m_innerLayout;

private:
	// User interface {
	void updateDisabledLabel();
	QVector<QSharedPointer<iADiagramFctWidget>> m_histograms;
	QVector<QSharedPointer<iASimpleSlicerWidget>> m_slicerWidgets;
	QStackedLayout *m_stackedLayout;
	QCheckBox *m_checkBox_weightByOpacity;
	QLabel *m_disabledLabel;
	// }

	QTimer *m_timer_updateVisualizations;
	int m_timerWait_updateVisualizations;
	void updateVisualizationsNow();
	void updateVisualizationsLater();
	void updateCopyTransferFunction(int index);
	void updateOriginalTransferFunction(int index);
	void applyWeights();

	BCoord m_weights;

	void updateLabels();
	iASlicerMode m_slicerMode;
	QLabel *m_slicerModeLabel;
	QLabel *m_sliceNumberLabel;

	// Slicers
	//vtkSmartPointer<vtkImageData> m_slicerInputs[3][3];
	vtkSmartPointer<vtkImageData> m_slicerImages[3];

	NumOfMod m_numOfMod = UNDEFINED;
	QVector<QSharedPointer<iAModality>> m_modalitiesActive;
	QVector<bool> m_modalitiesHistogramAvailable;

	vtkSmartPointer<vtkSmartVolumeMapper> m_combinedVolMapper;
	vtkSmartPointer<vtkRenderer> m_combinedVolRenderer;
	vtkSmartPointer<vtkVolume> m_combinedVol;
	bool m_mainSlicersInitialized;
	//bool m_weightByOpacity;
	double m_minimumWeight;

	// Background stuff
	void alertWeightIsZero(QSharedPointer<iAModality> modality);
	QVector<QSharedPointer<iATransferFunction>> m_copyTFs;
	QSharedPointer<iATransferFunction> createCopyTf(int index, vtkSmartPointer<vtkColorTransferFunction> colorTf, vtkSmartPointer<vtkPiecewiseFunction> opacity);

signals:
	void weightsChanged3(BCoord weights);
	void weightsChanged2(double t);

	void slicerModeChangedExternally(iASlicerMode slicerMode);
	void sliceNumberChangedExternally(int sliceNumber);

	void modalitiesLoaded_beforeUpdate();

private slots:
	void updateTransferFunction1() { updateTransferFunction(0); }
	void updateTransferFunction2() { updateTransferFunction(1); }
	void updateTransferFunction3() { updateTransferFunction(2); }

	void originalHistogramChanged();

	void checkBoxSelectionChanged();

	void onMainXYScrollBarPress();
	void onMainXZScrollBarPress();
	void onMainYZScrollBarPress();
	void onMainXYSliceNumberChanged(int sliceNumberXY);
	void onMainXZSliceNumberChanged(int sliceNumberXZ);
	void onMainYZSliceNumberChanged(int sliceNumberYZ);

	//void modalitiesChanged();
	void histogramAvailable();
	void applyVolumeSettings();

	// Timers
	void onUpdateVisualizationsTimeout();
};