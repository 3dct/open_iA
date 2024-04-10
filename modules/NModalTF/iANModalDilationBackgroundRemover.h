// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iANModalBackgroundRemover.h"
#include "iANModalProgressWidget.h"

#include <defines.h>  // for DIM

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#endif
#include <itkImageBase.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include <vtkSmartPointer.h>

#include <QWidget>


class iAConnector;
class iAImageData;
class iAMdiChild;
class iANModalDisplay;
class iAProgress;

class vtkLookupTable;
class vtkPiecewiseFunction;

class QSlider;
class QSpinBox;

class iANModalThresholdingWidget : public QWidget
{
	Q_OBJECT
public:
	iANModalThresholdingWidget(QWidget* parent);
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

class iANModalDilationBackgroundRemover : public QObject, public iANModalBackgroundRemover
{
	Q_OBJECT

public:
	iANModalDilationBackgroundRemover(iAMdiChild* mdiChild);
	Mask removeBackground(const QList<std::shared_ptr<iAImageData>>&) override;

private:
	using ImagePointer = itk::ImageBase<DIM>::Pointer;

	//iAConnector *m_temp_connector = nullptr;
	//vtkSmartPointer<vtkImageData> m_vtkTempImg;
	// TODO: Try without
	ImagePointer m_itkTempImg;

	iAMdiChild* m_mdiChild;

	// return - true if a dataset and a threshold were successfully chosen
	//        - false otherwise
	bool selectDataSetAndThreshold(QWidget* parent, const QList<std::shared_ptr<iAImageData>>& dataSets,
		int& out_threshold, std::shared_ptr<iAImageData>& out_dataSet,
		iANModalBackgroundRemover::MaskMode& out_maskMode);

	// TODO describe
	bool iterativeDilation(ImagePointer mask, int regionCountGoal);

	iANModalDisplay* m_display;
	unsigned int m_threholdingMaskChannelId;
	vtkSmartPointer<vtkLookupTable> m_colorTf;
	vtkSmartPointer<vtkPiecewiseFunction> m_opacityTf;

	iANModalThresholdingWidget* m_threshold;

	template <class T>
	void itkBinaryThreshold(iAConnector& conn, int loThresh, int upThresh);

#ifndef NDEBUG
	// Note: currently not called from anywhere:
	void showMask(std::shared_ptr<iAImageData> mod, vtkSmartPointer<vtkImageData> mask);
	void showMask(ImagePointer itkImgPtr);
#endif

public slots:
	void setDataSetSelected(std::shared_ptr<iAImageData>);
	void updateDataSetSelected();
	void updateThreshold();
};

class iANModalIterativeDilationThread : public iANModalProgressUpdater
{
	Q_OBJECT

private:
	typedef itk::ImageBase<DIM>::Pointer ImagePointer;

	iAProgress* m_progDil;
	iAProgress* m_progCc;
	iAProgress* m_progEro;
	ImagePointer m_mask;
	int m_regionCountGoal;

	void itkDilateAndCountConnectedComponents(ImagePointer itkImgPtr, int& connectedComponents, bool dilate = true);
	void itkCountConnectedComponents(ImagePointer itkImgPtr, int& connectedComponents);
	void itkDilate(ImagePointer itkImgPtr);
	void itkErodeAndInvert(ImagePointer itkImgPtr, int count);

	bool m_canceled = false;

public:
	iANModalIterativeDilationThread(
		iANModalProgressWidget* progressWidget, iAProgress* progress[3], ImagePointer mask, int regionCountGoal);
	void run() override;
	ImagePointer mask()
	{
		return m_mask;
	}

public slots:
	void setCanceled(bool);

signals:
	void addValue(int v);
};

class iANModalIterativeDilationPlot : public QWidget
{
	Q_OBJECT

public:
	iANModalIterativeDilationPlot(QWidget* parent);

private:
	QList<int> m_values;
	int m_max = 0;

public slots:
	void addValue(int v);

protected:
	void paintEvent(QPaintEvent* event) override;
	QSize sizeHint() const override;
};
