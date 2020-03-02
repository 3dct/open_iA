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

#include <vtkImageData.h>
#include <vtkLookupTable.h>
#include <vtkPiecewiseFunction.h>

#include <itkBinaryThresholdImageFilter.h>

#include <QDialog>
#include <QVBoxLayout>
#include <QSlider>
#include <QSpinBox>
#include <QStatusBar>
#include <QLabel>
#include <QPushButton>

template<class T>
void iANModalDilationBackgroundRemover::itkBinaryThreshold(iAConnector &conn, int loThresh, int upThresh) {
	typedef itk::Image<T, 3> InputImageType;
	typedef itk::Image<T, 3> OutputImageType;
	typedef itk::BinaryThresholdImageFilter<InputImageType, OutputImageType> BTIFType;

	auto binThreshFilter = BTIFType::New();
	binThreshFilter->SetLowerThreshold(loThresh);
	binThreshFilter->SetUpperThreshold(upThresh);
	binThreshFilter->SetOutsideValue(1);
	binThreshFilter->SetInsideValue(0);
	binThreshFilter->SetInput(dynamic_cast<InputImageType *>(conn.itkImage()));
	//filter->progress()->observe(binThreshFilter);
	binThreshFilter->Update();
	
	conn.setImage(binThreshFilter->GetOutput());
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
{
	m_colorTf = vtkSmartPointer<vtkLookupTable>::New();
	m_colorTf->SetNumberOfTableValues(2);
	m_colorTf->SetRange(0, 1);
	m_colorTf->SetTableValue(0.0, 0.0, 0.0, 0.0, 0.0);
	m_colorTf->SetTableValue(1.0, 1.0, 0.0, 0.0, 0.5);
	m_colorTf->Build();

	// TODO: opacity tf
}

vtkSmartPointer<vtkImageData> iANModalDilationBackgroundRemover::removeBackground(QList<QSharedPointer<iAModality>> modalities) {

	QSharedPointer<iAModality> mod;
	int upThresh;
	int loThresh = -1;

	bool success = selectModalityAndThreshold(nullptr, modalities, upThresh, mod);
	if (!success) {
		return nullptr;
	}

	iAConnector conn;
	conn.setImage(mod->image());
	ImagePointer itkImgPtr = conn.itkImage();

	if (conn.itkPixelType() == itk::ImageIOBase::SCALAR) {
		// TODO
	} else { // example if == itk::ImageIOBase::RGBA
		return nullptr;
	}
	
	ITK_TYPED_CALL(itkBinaryThreshold, conn.itkScalarPixelType(), conn, loThresh, upThresh);

	// Perform dilations


	conn.setImage(itkImgPtr);
	auto image = conn.vtkImage();
	return image; // If doesn't work, keep iAConnector alive (member variable)
}

// return - true if a modality and a threshold were successfully chosen
//        - false otherwise
bool iANModalDilationBackgroundRemover::selectModalityAndThreshold(QWidget *parent, QList<QSharedPointer<iAModality>> modalities,
	int &out_threshold, QSharedPointer<iAModality> &out_modality)
{
	QDialog *dialog = new QDialog(parent);
	// TODO: set dialog title
	dialog->setModal(true);

	auto layout = new QVBoxLayout(dialog);

	
	auto displayWidget = new QWidget(dialog);
	auto displayLayout = new QVBoxLayout(displayWidget);  {
		auto displayLabel = new QLabel("Select modality for the thresholding step of the dilation-based background removal", displayWidget);
		
		m_display = new iANModalDisplay(displayWidget, m_mdiChild, modalities, 1, 1);
		m_threholdingMaskChannelId = m_display->createChannel();
		//connect(m_display, SIGNAL(selectionChanged()), this, SLOT(updateThreshold()));
		connect(m_display, SIGNAL(selectionChanged()), this, SLOT(updateModalitySelected()));

		displayLayout->addWidget(displayLabel);
		displayLayout->addWidget(m_display);
		displayLayout->setSpacing(0);

		//displayWidget->setStyleSheet("border: 1px solid black");
		displayWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
	}

	auto thresholdWidget = new QWidget(dialog);
	auto thresholdLayout = new QVBoxLayout(thresholdWidget); {
		auto thresholdLabel = new QLabel("Set threshold", thresholdWidget);

		m_threshold = new iANModalThresholdingWidget(thresholdWidget);
		connect(m_threshold, SIGNAL(thresholdChanged(int)), this, SLOT(updateThreshold()));

		thresholdLayout->addWidget(thresholdLabel);
		thresholdLayout->addWidget(m_threshold);
		thresholdLayout->setSpacing(0);

		thresholdWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
	}

	auto footerWigdet = new QWidget(dialog);
	auto footerLabel = new QLabel(footerWigdet);
	auto footerLayout = new QHBoxLayout(footerWigdet); {
		auto footerOK = new QPushButton("OK");
		QObject::connect(footerOK, SIGNAL(clicked()), dialog, SLOT(accept()));

		auto footerCancel = new QPushButton("Cancel");
		QObject::connect(footerCancel, SIGNAL(clicked()), dialog, SLOT(reject()));

		footerLayout->addWidget(footerLabel);
		footerLayout->setStretchFactor(footerLabel, 1);

		footerLayout->addWidget(footerOK);
		footerLayout->setStretchFactor(footerOK, 0);
		
		footerLayout->addWidget(footerCancel);
		footerLayout->setStretchFactor(footerCancel, 0);

		footerWigdet->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
	}

	layout->addWidget(displayWidget);
	layout->setStretchFactor(displayWidget, 1);

	layout->addWidget(thresholdWidget);
	layout->setStretchFactor(thresholdWidget, 0);

	layout->addWidget(footerWigdet);
	layout->setStretchFactor(footerWigdet, 0);

	auto dialogCode = dialog->exec();
	if (dialogCode == QDialog::Rejected) {
		return false;
	}

	out_threshold = m_threshold->threshold();
	out_modality = m_display->singleSelection();

	return true;
}

void iANModalDilationBackgroundRemover::updateModalitySelected() {
	setModalitySelected(m_display->singleSelection());
}

void iANModalDilationBackgroundRemover::setModalitySelected(QSharedPointer<iAModality> modality) {
	double range[2];
	modality->image()->GetScalarRange(range);
	double min = range[0];
	double max = range[1];
	int value = round(max - min) / 2 + min;

	m_threshold->spinBox()->setRange(min, max);
	m_threshold->spinBox()->setValue(value);

	m_threshold->slider()->setRange(min, max);
	//m_threshold->slider()->setValue(value); // already happens at spinBox()->setValue(value)

	//updateThreshold(); // already happens at spinBox()->setValue(value)
}

void iANModalDilationBackgroundRemover::updateThreshold() {
	auto modality = m_display->singleSelection();
	int threshold = m_threshold->threshold();

	iAConnector conn;
	conn.setImage(modality->image());
	ITK_TYPED_CALL(itkBinaryThreshold, conn.itkScalarPixelType(), conn, 0, threshold);
	auto mask = conn.vtkImage();

	/*m_labelColorTF = iALUT::BuildLabelColorTF(m_itemModel->rowCount(), m_colorTheme);
	m_labelOpacityTF = iALUT::BuildLabelOpacityTF(m_itemModel->rowCount());
	
	auto channelData = slicerData->channelData;
	channelData.setColorTF(m_labelColorTF);
	channelData.setOpacityTF(m_labelOpacityTF);*/

	auto channelData = iAChannelData("Threshold mask", mask, m_colorTf);
	m_display->setChannelData(m_threholdingMaskChannelId, channelData);
}



iANModalThresholdingWidget::iANModalThresholdingWidget(QWidget *parent) :
	QWidget(parent)
{
	int min = 0;
	int max = 0;
	int threshold = round(max - min) / 2 + min;

	//auto slicer = display->createSlicer(m_mod);

	m_spinBox = new QSpinBox(this);
	m_spinBox->setMaximum(max);
	m_spinBox->setMinimum(min);
	m_spinBox->setValue(threshold);

	//auto slider = new QSlider(Qt::Orientation::Vertical, this);
	m_slider = new QSlider(Qt::Orientation::Horizontal, this);
	m_slider->setMaximum(max);
	m_slider->setMinimum(min);
	m_slider->setValue(threshold);

	/*auto layout = new QGridLayout(this);
	layout->addWidget(slicer, 0, 0, 2, 1);
	layout->addWidget(spinBox, 0, 1);
	layout->addWidget(slider, 1, 1);*/
	auto layout = new QHBoxLayout(this);
	layout->addWidget(m_slider);
	layout->setStretchFactor(m_slider, 1);
	layout->addWidget(m_spinBox);
	layout->setStretchFactor(m_spinBox, 0);

	connect(m_slider, SIGNAL(sliderMoved(int)), m_spinBox, SLOT(setValue(int)));
	connect(m_spinBox, SIGNAL(valueChanged(int)), m_slider, SLOT(setValue(int)));
	connect(m_spinBox, SIGNAL(valueChanged(int)), this, SLOT(setThreshold(int)));
	connect(m_spinBox, SIGNAL(valueChanged(int)), this, SIGNAL(thresholdChanged(int)));
}

void iANModalThresholdingWidget::setThreshold(int threshold) {
	m_threshold = threshold;
}

int iANModalThresholdingWidget::threshold() {
	return m_threshold;
}

QSlider* iANModalThresholdingWidget::slider() {
	return m_slider;
}

QSpinBox* iANModalThresholdingWidget::spinBox() {
	return m_spinBox;
}