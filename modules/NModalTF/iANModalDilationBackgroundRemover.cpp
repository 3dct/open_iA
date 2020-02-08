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

#include "iANModalDilationBackgroundRemover.h"

#include "iANModalDisplay.h"

#include "iAModality.h"
#include "iAConnector.h"
#include "iATypedCallHelper.h"
#include "iASlicer.h"

#include "vtkSmartPointer.h"
#include "vtkImageData.h"

#include <itkBinaryThresholdImageFilter.h>

#include <QDialog>
#include <QVBoxLayout>
#include <QSlider>
#include <QSpinBox>
#include <QStatusBar>

template<class T>
void iANModalDilationBackgroundRemover::itkBinaryThreshold(iAConnector *conn, int loThresh, int upThresh) {
	typedef itk::Image<T, 3> InputImageType;
	typedef itk::Image<T, 3> OutputImageType;
	typedef itk::BinaryThresholdImageFilter<InputImageType, OutputImageType> BTIFType;

	auto binThreshFilter = BTIFType::New();
	binThreshFilter->SetLowerThreshold(loThresh);
	binThreshFilter->SetUpperThreshold(upThresh);
	binThreshFilter->SetOutsideValue(1);
	binThreshFilter->SetInsideValue(0);
	binThreshFilter->SetInput(dynamic_cast<InputImageType *>(conn->itkImage()));
	//filter->progress()->observe(binThreshFilter);
	binThreshFilter->Update();
	
	conn->setImage(binThreshFilter->GetOutput());
}

template<class T>
void iANModalDilationBackgroundRemover::binary_threshold(ImagePointer itkImgPtr) {
	/*typedef itk::Image< T, 3 >   InputImageType;
	typedef itk::Image< T, 3 >   OutputImageType;
	typedef itk::BinaryThresholdImageFilter<InputImageType, OutputImageType> BTIFType;
	typedef itk::Erosion...Filter

		auto binThreshFilter = BTIFType::New();
	binThreshFilter->SetLowerThreshold(loThresh);
	binThreshFilter->SetUpperThreshold(upThresh);
	binThreshFilter->SetOutsideValue(1);
	binThreshFilter->SetInsideValue(0);
	binThreshFilter->SetInput(dynamic_cast<InputImageType *>(conn.itkImage());
	//filter->progress()->observe(binThreshFilter);
	//binThreshFilter->Update();

	auto erosionFilter = ...;
	//set everything
	erosionFilter->SetInput(binThreshFilter->GetOutput());
	erosionFilter->Update();

	//filter->addOutput(binThreshFilter->GetOutput());
	//conn.setImage(binThreshFilter->GetOutput());
	m_itkTempImg = binThreshFilter->GetOutput();*/
}

template<class T>
void iANModalDilationBackgroundRemover::binary_threshold2(ImagePointer itkImgPtr) {
	
}

iANModalDilationBackgroundRemover::iANModalDilationBackgroundRemover(MdiChild *mdiChild)
	: m_mdiChild(mdiChild)
{}

vtkSmartPointer<vtkImageData> iANModalDilationBackgroundRemover::removeBackground(QList<QSharedPointer<iAModality>> modalities) {

	QSharedPointer<iAModality> mod;
	int upThresh;
	int loThresh = 0;

	bool success = selectModalityAndThreshold(nullptr, modalities, upThresh, mod);
	if (!success) {
		return nullptr;
	}

	iAConnector *conn;
	conn->setImage(mod->image());
	ImagePointer itkImgPtr = conn->itkImage();

	if (conn->itkPixelType() == itk::ImageIOBase::SCALAR) {
		// TODO
	} else { // example if == itk::ImageIOBase::RGBA
		return nullptr;
	}
	
	ITK_TYPED_CALL(itkBinaryThreshold, conn.itkScalarPixelType(), conn, loThresh, upThresh);

	// Perform dilations


	conn->setImage(itkImgPtr);
	auto image = conn->vtkImage();
	return image; // If doesn't work, keep iAConnector alive (member variable)
}

// return - true if a modality and a threshold were successfully chosen
//        - false otherwise
bool iANModalDilationBackgroundRemover::selectModalityAndThreshold(QWidget *parent, QList<QSharedPointer<iAModality>> modalities,
	int &out_threshold, QSharedPointer<iAModality> &out_modality)
{
	QDialog *dialog = new QDialog(parent);
	dialog->setModal(true);

	auto widget = new QWidget(dialog);
	auto layout = new QVBoxLayout(widget);

	auto display = new iANModalDisplay(widget, m_mdiChild, modalities, 1, 1);
	auto thresholdWidget = new iANModalThresholdingWidget(widget, display);
	auto statusBar = new QStatusBar();

	layout->addWidget(display);
	layout->addWidget(thresholdWidget, Qt::AlignHCenter);
	layout->addWidget(statusBar);

	auto dialogCode = dialog->exec();
	if (dialogCode == QDialog::Rejected) {
		return false;
	}

	out_threshold = thresholdWidget->threshold();
	out_modality = thresholdWidget->modality();

	return true;
}

iANModalThresholdingWidget::iANModalThresholdingWidget(QWidget *parent, iANModalDisplay *display) :
	QWidget(parent)
{
	m_mod = display->singleSelection();
	double range[2];
	m_mod->image()->GetScalarRange(range);
	double min = range[0];
	double max = range[1];
	int threshold = round(max - min) / 2 + min;

	auto slicer = display->createSlicer(m_mod);

	auto spinBox = new QSpinBox(this);
	spinBox->setMaximum(max);
	spinBox->setMinimum(min);
	spinBox->setValue(threshold);

	auto slider = new QSlider(Qt::Orientation::Vertical, this);
	slider->setMaximum(max);
	slider->setMinimum(min);
	slider->setValue(threshold);

	auto layout = new QGridLayout(this);
	layout->addWidget(slicer, 0, 0, 2, 1);
	layout->addWidget(spinBox, 0, 1);
	layout->addWidget(slider, 1, 1);

	connect(slider, SIGNAL(sliderMoved(int)), spinBox, SLOT(setValue(int)));
	connect(spinBox, SIGNAL(valueChanged(int)), slider, SLOT(setValue(int)));
	connect(spinBox, SIGNAL(valueChanged(int)), this, SLOT(setThreshold(int)));
}

void iANModalThresholdingWidget::setThreshold(int threshold) {
	m_threshold = threshold;
}