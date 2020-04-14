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
#include "iANModalProgressWidget.h"

#include <defines.h>  // for DIM
#include "iAConnector.h"
#include "iAModality.h"
#include "iASlicer.h"
#include "iAToolsITK.h"
#include "iATypedCallHelper.h"

#include <vtkImageData.h>
#include <vtkLookupTable.h>
#include <vtkPiecewiseFunction.h>

#include <itkBinaryBallStructuringElement.h>
#include <itkBinaryDilateImageFilter.h>
#include <itkBinaryErodeImageFilter.h>
#include <itkBinaryThresholdImageFilter.h>
#include <itkConnectedComponentImageFilter.h>

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QStatusBar>
#include <QVBoxLayout>

namespace
{
static const int BACKGROUND = 0;
static const int FOREGROUND = 1;
}

template <class T>
void iANModalDilationBackgroundRemover::itkBinaryThreshold(iAConnector &conn, int loThresh, int upThresh)
{
	typedef itk::Image<T, DIM> InputImageType;
	typedef itk::Image<int, DIM> OutputImageType;
	typedef itk::BinaryThresholdImageFilter<InputImageType, OutputImageType> BTIFType;

	auto binThreshFilter = BTIFType::New();
	binThreshFilter->SetLowerThreshold(loThresh);
	binThreshFilter->SetUpperThreshold(upThresh);
	binThreshFilter->SetOutsideValue(BACKGROUND);
	binThreshFilter->SetInsideValue(FOREGROUND);
	binThreshFilter->SetInput(dynamic_cast<InputImageType *>(conn.itkImage()));
	//filter->progress()->observe(binThreshFilter);
	binThreshFilter->Update();

	m_itkTempImg = binThreshFilter->GetOutput();
	conn.setImage(m_itkTempImg);
}

void iANModalDilationBackgroundRemover::itkDilateAndCountConnectedComponents(
	ImagePointer itkImgPtr, int &connectedComponentsOut, bool dilate /*= true*/)
{
	typedef itk::Image<int, DIM> ImageType;
	typedef itk::ConnectedComponentImageFilter<ImageType, ImageType> CCIFType;

	typedef itk::FlatStructuringElement<DIM> StructuringElementType;
	typename StructuringElementType::RadiusType elementRadius;
	elementRadius.Fill(1);
	auto structuringElement = StructuringElementType::Ball(elementRadius);
	typedef itk::BinaryDilateImageFilter<ImageType, ImageType, StructuringElementType> BDIFType;

	typename BDIFType::Pointer dilationFilter;
	auto connCompFilter = CCIFType::New();

	//iAProgress progress;

	auto input = dynamic_cast<ImageType *>(itkImgPtr.GetPointer());
	if (dilate)
	{
		// dilate the background (region of higher intensity)...
		dilationFilter = BDIFType::New();
		dilationFilter->SetDilateValue(BACKGROUND);
		dilationFilter->SetKernel(structuringElement);
		dilationFilter->SetInput(input);

		//progress.observe(dilationFilter);

		connCompFilter->SetInput(dilationFilter->GetOutput());
	}
	else
	{
		connCompFilter->SetInput(input);
	}

	// ...until the number of foreground components is equal to connectedComponents
	connCompFilter->SetBackgroundValue(BACKGROUND);
	//progress.observe(connCompFilter);
	//filter->progress()->observe(connCompFilter);
	connCompFilter->Update();

	//filter->addOutput(binThreshFilter->GetOutput());
	//conn.setImage(binThreshFilter->GetOutput());
	m_itkTempImg = connCompFilter->GetOutput();

	connectedComponentsOut = connCompFilter->GetObjectCount();
}

void iANModalDilationBackgroundRemover::itkCountConnectedComponents(ImagePointer itkImgPtr, int &connectedComponentsOut)
{
	itkDilateAndCountConnectedComponents(itkImgPtr, connectedComponentsOut, false);
}

void iANModalDilationBackgroundRemover::itkErode(ImagePointer itkImgPtr, int count)
{
	typedef itk::FlatStructuringElement<DIM> StructuringElementType;
	typedef itk::Image<int, DIM> ImageType;
	typedef itk::BinaryErodeImageFilter<ImageType, ImageType, StructuringElementType> BDIFType;

	typename StructuringElementType::RadiusType elementRadius;
	elementRadius.Fill(1);
	auto structuringElement = StructuringElementType::Ball(elementRadius);

	if (count <= 0)
	{
		m_itkTempImg = itkImgPtr;
		return;
	}

	std::vector<typename BDIFType::Pointer> erosionFilters(count);
	auto input = dynamic_cast<ImageType *>(itkImgPtr.GetPointer());
	for (int i = 0; i < count; i++)
	{
		erosionFilters[i] = BDIFType::New();
		erosionFilters[i]->SetKernel(structuringElement);
		erosionFilters[i]->SetErodeValue(FOREGROUND);
		erosionFilters[i]->SetInput(i == 0 ? input : erosionFilters[i - 1]->GetOutput());
	}

	erosionFilters[count - 1]->Update();

	m_itkTempImg = erosionFilters[count - 1]->GetOutput();
}

#ifndef NDEBUG
void iANModalDilationBackgroundRemover::showMask(QSharedPointer<iAModality> mod, vtkSmartPointer<vtkImageData> mask)
{
	QList<QSharedPointer<iAModality>> mods;
	mods.append(mod);
	auto display = new iANModalDisplay(new QWidget(), m_mdiChild, mods);
	uint cid = display->createChannel();
	auto cd = iAChannelData("Binary mask for " + mod->name(), mask, m_colorTf, m_opacityTf);
	display->setChannelData(cid, cd);
	iANModalDisplay::selectModalities(display);
}
void iANModalDilationBackgroundRemover::showMask(ImagePointer itkImgPtr)
{
	auto c = iAConnector();
	c.setImage(itkImgPtr);
	auto vtkImg = c.vtkImage();

	QList<QSharedPointer<iAModality>> mods;
	mods.append(QSharedPointer<iAModality>(new iAModality("Binary mask", "", -1, vtkImg, iAModality::NoRenderer)));
	auto display = new iANModalDisplay(new QWidget(), m_mdiChild, mods);
	auto cd = iAChannelData("temp", vtkImg, m_colorTf, m_opacityTf);
	display->setChannelData(iANModalDisplay::MAIN_CHANNEL_ID, cd);
	iANModalDisplay::selectModalities(display);
}
#else
void iANModalDilationBackgroundRemover::showMask(QSharedPointer<iAModality> mod, vtkSmartPointer<vtkImageData> mask){}
void iANModalDilationBackgroundRemover::showMask(ImagePointer itkImgPtr){}
#endif

iANModalDilationBackgroundRemover::iANModalDilationBackgroundRemover(MdiChild *mdiChild) : m_mdiChild(mdiChild)
{
	m_colorTf = vtkSmartPointer<vtkLookupTable>::New();
	m_colorTf->SetNumberOfTableValues(2);
	m_colorTf->SetRange(0, 1);
	m_colorTf->SetTableValue(0.0, 0.0, 0.0, 0.0, 0.0);
	m_colorTf->SetTableValue(1.0, 1.0, 0.0, 0.0, 0.5);
	m_colorTf->Build();
}

vtkSmartPointer<vtkImageData> iANModalDilationBackgroundRemover::removeBackground(
	QList<QSharedPointer<iAModality>> modalities)
{
	QSharedPointer<iAModality> selectedMod;
	int upThresh;
	int loThresh = 0;
	int regionCountGoal = 1;

	bool success = selectModalityAndThreshold(nullptr, modalities, upThresh, selectedMod);
	if (!success)
	{
		return nullptr;
	}

	iAConnector conn;
	//conn.setImage(mod->image());
	conn.setImage(m_itkTempImg);
	//ImagePointer itkImgPtr = conn.itkImage();

	if (conn.itkPixelType() == itk::ImageIOBase::SCALAR)
	{
		// TODO
	}
	else
	{  // example if == itk::ImageIOBase::RGBA
		return nullptr;
	}

	success = iterativeDilation(m_itkTempImg, regionCountGoal);
	if (success)
	{
		conn.setImage(m_itkTempImg);
		return conn.vtkImage();  // If doesn't work, keep iAConnector alive (member variable)
	}
	else
	{
		return nullptr;
	}
}

// return - true if a modality and a threshold were successfully chosen
//        - false otherwise
bool iANModalDilationBackgroundRemover::selectModalityAndThreshold(QWidget *parent,
	QList<QSharedPointer<iAModality>> modalities, int &out_threshold, QSharedPointer<iAModality> &out_modality)
{
	QDialog *dialog = new QDialog(parent);
	// TODO: set dialog title
	dialog->setModal(true);

	auto layout = new QVBoxLayout(dialog);


	auto displayWidget = new QWidget(dialog);
	auto displayLayout = new QVBoxLayout(displayWidget);
	{
		auto displayLabel = new QLabel(
			"Select modality for the thresholding step of the dilation-based background removal", displayWidget);

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
	auto thresholdLayout = new QVBoxLayout(thresholdWidget);
	{
		auto thresholdLabel = new QLabel("Set threshold", thresholdWidget);

		m_threshold = new iANModalThresholdingWidget(thresholdWidget);
		connect(m_threshold, SIGNAL(thresholdChanged(int)), this, SLOT(updateThreshold()));

		thresholdLayout->addWidget(thresholdLabel);
		thresholdLayout->addWidget(m_threshold);
		thresholdLayout->setSpacing(0);

		thresholdWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
	}

	layout->addWidget(displayWidget);
	layout->setStretchFactor(displayWidget, 1);

	layout->addWidget(thresholdWidget);
	layout->setStretchFactor(thresholdWidget, 0);

	QWidget *footerWidget = iANModalDisplay::createOkCancelFooter(dialog);
	layout->addWidget(footerWidget);
	layout->setStretchFactor(footerWidget, 0);

	auto dialogCode = dialog->exec();
	if (dialogCode == QDialog::Rejected)
	{
		return false;
	}

	out_threshold = m_threshold->threshold();
	out_modality = m_display->singleSelection();

	return true;
}

void iANModalDilationBackgroundRemover::updateModalitySelected()
{
	setModalitySelected(m_display->singleSelection());
}

void iANModalDilationBackgroundRemover::setModalitySelected(QSharedPointer<iAModality> modality)
{
	double range[2];
	modality->image()->GetScalarRange(range);
	double min = range[0];
	double max = range[1];
	int value = round(max - min) / 2 + min;

	m_threshold->spinBox()->setRange(min, max);
	m_threshold->spinBox()->setValue(value);

	m_threshold->slider()->setRange(min, max);
	m_threshold->slider()->setValue(value);

	//updateThreshold(); // already happens at spinBox()->setValue(value)
}

void iANModalDilationBackgroundRemover::updateThreshold()
{
	auto modality = m_display->singleSelection();
	int threshold = m_threshold->threshold();

	iAConnector conn;
	conn.setImage(modality->image());
	ITK_TYPED_CALL(itkBinaryThreshold, conn.itkScalarPixelType(), conn, 0, threshold);
	auto mask = conn.vtkImage();

	auto channelData = iAChannelData("Threshold mask", mask, m_colorTf);
	m_display->setChannelData(m_threholdingMaskChannelId, channelData);
}

bool iANModalDilationBackgroundRemover::iterativeDilation(ImagePointer mask, int regionCountGoal)
{
	QLabel *statusLabel = new QLabel("Total progress");
	QLabel *filterLabel = new QLabel("Filter progress");
	QLabel *dilationsLabel = new QLabel("Dilations progress");

	iANModalProgressWidget *progress = new iANModalProgressWidget();
	int status = progress->addProgressBar(3, statusLabel, true);  // 3 steps: counts; counts+dilations; erosions
	progress->addSeparator();
	int filter = progress->addProgressBar(100, filterLabel, true);  // 100 steps, because that's how iAProgress works
	int dilations = progress->addProgressBar(1, dilationsLabel, false);  // unkown number of steps... change later
	progress->show();

	// Now begin the heavy computation

	int connectedComponents;

	itkCountConnectedComponents(mask, connectedComponents);
	//storeImage(m_itkTempImg, "connectedComponents.mhd", true);

	int dilationCount = 0;
	while (connectedComponents > regionCountGoal)
	{
		itkDilateAndCountConnectedComponents(m_itkTempImg, connectedComponents);
		//storeImage(m_itkTempImg, "connectedComponents" + QString::number(dilationCount) + ".mhd", true);
		dilationCount++;
	}

	itkErode(m_itkTempImg, dilationCount);

	return true;
}


// ----------------------------------------------------------------------------------------------
// iANModalThresholdingWidget
// ----------------------------------------------------------------------------------------------

iANModalThresholdingWidget::iANModalThresholdingWidget(QWidget *parent) : QWidget(parent)
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

void iANModalThresholdingWidget::setThreshold(int threshold)
{
	m_threshold = threshold;
}

int iANModalThresholdingWidget::threshold()
{
	return m_threshold;
}

QSlider *iANModalThresholdingWidget::slider()
{
	return m_slider;
}

QSpinBox *iANModalThresholdingWidget::spinBox()
{
	return m_spinBox;
}