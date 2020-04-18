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

#include "iANModalBackgroundRemover.h"
#include "iANModalProgressWidget.h"

#include <itkImageBase.h>

#include <vtkSmartPointer.h>

#include <QWidget>

class MdiChild;
class iAConnector;
class iANModalDisplay;
class iAProgress;

class vtkLookupTable;
class vtkPiecewiseFunction;

class QSlider;
class QSpinBox;

class iANModalThresholdingWidget : public QWidget {
	Q_OBJECT
public:
	iANModalThresholdingWidget(QWidget *parent);
	int threshold();
	QSlider* slider();
	QSpinBox* spinBox();
private:
	QSlider* m_slider;
	QSpinBox* m_spinBox;
	int m_threshold;
signals:
	void thresholdChanged(int);
private slots:
	void setThreshold(int);
};

class iANModalDilationBackgroundRemover : public QObject, public iANModalBackgroundRemover {
	Q_OBJECT

public:
	iANModalDilationBackgroundRemover(MdiChild *mdiChild);
	vtkSmartPointer<vtkImageData> removeBackground(QList<QSharedPointer<iAModality>>) override;

private:
	typedef itk::ImageBase<3>::Pointer ImagePointer;

	//iAConnector *m_temp_connector = nullptr;
	//vtkSmartPointer<vtkImageData> m_vtkTempImg;
	// TODO: Try without
	ImagePointer m_itkTempImg;

	MdiChild *m_mdiChild;

	// return - true if a modality and a threshold were successfully chosen
	//        - false otherwise
	bool selectModalityAndThreshold(QWidget *parent, QList<QSharedPointer<iAModality>> modalities, int &out_threshold, QSharedPointer<iAModality> &out_modality);
	
	// TODO describe
	bool iterativeDilation(ImagePointer mask, int regionCountGoal);

	iANModalDisplay *m_display;
	uint m_threholdingMaskChannelId;
	vtkSmartPointer<vtkLookupTable> m_colorTf;
	vtkSmartPointer<vtkPiecewiseFunction> m_opacityTf;

	iANModalThresholdingWidget *m_threshold;

	template<class T>
	void itkBinaryThreshold(iAConnector &conn, int loThresh, int upThresh);

	// TODO make debug only
	void showMask(QSharedPointer<iAModality> mod, vtkSmartPointer<vtkImageData> mask);
	void showMask(ImagePointer itkImgPtr);

public slots:
	void setModalitySelected(QSharedPointer<iAModality>);
	void updateModalitySelected();
	void updateThreshold();
};

class iANModalIterativeDilationThread : public iANModalProgressUpdater {
	Q_OBJECT

private:
	typedef itk::ImageBase<3>::Pointer ImagePointer;

	iAProgress *m_progDil;
	iAProgress *m_progCc;
	iAProgress *m_progEro;
	ImagePointer m_mask;
	int m_regionCountGoal;

	void itkDilateAndCountConnectedComponents(ImagePointer itkImgPtr, int &connectedComponents, bool dilate = true);
	void itkCountConnectedComponents(ImagePointer itkImgPtr, int &connectedComponents);
	void itkErode(ImagePointer itkImgPtr, int count);

public:
	iANModalIterativeDilationThread(iANModalProgressWidget *progressWidget, iAProgress *progress[3], ImagePointer mask, int regionCountGoal);
	void run() override;

signals:
	void addValue(int v);
};

class iANModalIterativeDilationPlot : public QWidget {
	Q_OBJECT

public:
	iANModalIterativeDilationPlot(QWidget *parent);

private:
	QList<int> m_values;
	int m_max = 0;

public slots:
	void addValue(int v);

protected:
	void paintEvent(QPaintEvent* event) override;
	QSize sizeHint() const override;
};