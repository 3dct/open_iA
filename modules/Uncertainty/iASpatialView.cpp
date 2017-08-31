#include "iASpatialView.h"

#include "iAImageWidget.h"
#include "iASlicerMode.h"

#include "iAChannelVisualizationData.h"
#include "iAChannelID.h"
#include "iASlicer.h"
#include "iASlicerData.h"

//#include <QVTKOpenGLWidget.h"
#include <vtkPiecewiseFunction.h>
#include <vtkLookupTable.h>
#include <vtkImageData.h>

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

iASpatialView::iASpatialView(): QWidget(),
	m_selectionChannelInitialized(false)
{
	xyButton = new QPushButton("XY");
	xzButton = new QPushButton("XZ");
	yzButton = new QPushButton("YZ");
	xyButton->setDown(true);
	xyButton->setAutoExclusive(false);
	xzButton->setAutoExclusive(false);
	yzButton->setAutoExclusive(false);
	// TODO: find out why QPushButton::setCheckable doesn't work as advertised (i.e., not at all)
	connect(xyButton, SIGNAL(clicked()), this, SLOT(xyClicked()));
	connect(xzButton, SIGNAL(clicked()), this, SLOT(xzClicked()));
	connect(yzButton, SIGNAL(clicked()), this, SLOT(yzClicked()));

	m_sliceControl = new QSpinBox();
	m_sliceControl->setMaximum(0);
	connect(m_sliceControl, SIGNAL(valueChanged(int)), this, SLOT(sliceChanged(int)));

	auto sliceButtonBar = new QWidget();
	sliceButtonBar->setLayout(new QHBoxLayout());
	sliceButtonBar->layout()->setSpacing(0);
	sliceButtonBar->layout()->addWidget(xyButton);
	sliceButtonBar->layout()->addWidget(xzButton);
	sliceButtonBar->layout()->addWidget(yzButton);

	m_sliceBar = new QWidget();
	m_sliceBar->setLayout(new QHBoxLayout());
	m_sliceBar->layout()->setSpacing(0);
	m_sliceBar->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
	m_sliceBar->layout()->addWidget(sliceButtonBar);
	m_sliceBar->layout()->addWidget(m_sliceControl);

	m_contentWidget = new QWidget();
	m_contentWidget->setLayout(new QHBoxLayout());
	m_contentWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding);
	m_contentWidget->layout()->setSpacing(0);

	setLayout(new QVBoxLayout());
	layout()->addWidget(m_contentWidget);
	layout()->addWidget(m_sliceBar);
}

void iASpatialView::AddImage(QString const & caption, vtkImagePointer img)
{
	m_images.push_back(QPair<QString, vtkImagePointer>(caption, img));

	auto imgWidget = new iAImageWidget(img);
	QWidget*  container = new QWidget();
	container->setLayout(new QVBoxLayout());
	auto label = new QLabel(caption);
	label->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
	label->setAlignment(Qt::AlignHCenter);
	container->layout()->addWidget(label);
	container->layout()->addWidget(imgWidget);
	m_contentWidget->layout()->addWidget(container);
	m_imageWidgets.push_back(imgWidget);
	m_sliceControl->setMaximum(imgWidget->GetSliceCount()-1);
}

void iASpatialView::StyleChanged()
{
	
	for (int i = 0; i < m_images.size(); ++i)
	{
		m_imageWidgets[i]->StyleChanged();
	}
}


void iASpatialView::xyClicked()
{
	xyButton->setDown(true);
	xzButton->setDown(false);
	yzButton->setDown(false);
	for (int i = 0; i < m_images.size(); ++i)
	{
		m_imageWidgets[i]->SetMode(iASlicerMode::XY);
	}
	if (m_imageWidgets.size() > 0)
		m_sliceControl->setMaximum(m_imageWidgets[0]->GetSliceCount()-1);
}

void iASpatialView::xzClicked()
{
	xyButton->setDown(false);
	xzButton->setDown(true);
	yzButton->setDown(false);
	for (int i = 0; i < m_images.size(); ++i)
	{
		m_imageWidgets[i]->SetMode(iASlicerMode::XZ);
	}
	if (m_imageWidgets.size() > 0)
		m_sliceControl->setMaximum(m_imageWidgets[0]->GetSliceCount()-1);
}

void iASpatialView::yzClicked()
{
	xyButton->setDown(false);
	xzButton->setDown(false);
	yzButton->setDown(true);
	for (int i = 0; i < m_images.size(); ++i)
	{
		m_imageWidgets[i]->SetMode(iASlicerMode::YZ);
	}
	if (m_imageWidgets.size() > 0)
		m_sliceControl->setMaximum(m_imageWidgets[0]->GetSliceCount()-1);
}

void iASpatialView::sliceChanged(int slice)
{
	for (int i = 0; i < m_images.size(); ++i)
	{
		m_imageWidgets[i]->SetSlice(slice);
	}
}


vtkSmartPointer<vtkLookupTable> BuildLabelOverlayLUT()
{
	auto result = vtkSmartPointer<vtkLookupTable>::New();
	result->SetNumberOfTableValues(2);
	result->SetRange(0, 1);                  // alpha value here is not used!
	result->SetTableValue(0.0, 0.0, 0.0, 0.0);
	result->SetTableValue(1.0, 1.0, 1.0, 0.0);
	result->Build();
	return result;
}


vtkSmartPointer<vtkPiecewiseFunction> BuildLabelOverlayOTF()
{
	auto result = vtkSmartPointer<vtkPiecewiseFunction>::New();
	result->AddPoint(0.0, 0.0);
	result->AddPoint(1.0, 0.5);
	return result;
}


void iASpatialView::ShowSelection(vtkImagePointer selectionImg)
{
	iASlicer* slicer = m_imageWidgets[0]->GetSlicer();
	if (!m_selectionChannelInitialized)
	{
		iAChannelID id = static_cast<iAChannelID>(ch_Concentration0);
		m_selectionData = QSharedPointer<iAChannelVisualizationData>(new iAChannelVisualizationData);
		m_ctf = BuildLabelOverlayLUT();
		m_otf = BuildLabelOverlayOTF();
		ResetChannel(m_selectionData.data(), selectionImg, m_ctf, m_otf);
		m_selectionData->SetName("Scatterplot Selection");

		// move to iAImageWidget?
		slicer->initializeChannel(id, m_selectionData.data());
		int sliceNr = slicer->GetSlicerData()->getSliceNumber();
		switch (slicer->GetMode())
		{
			case YZ: slicer->enableChannel(id, true, static_cast<double>(sliceNr) * selectionImg->GetSpacing()[0], 0, 0); break;
			case XY: slicer->enableChannel(id, true, 0, 0, static_cast<double>(sliceNr) * selectionImg->GetSpacing()[2]); break;
			case XZ: slicer->enableChannel(id, true, 0, static_cast<double>(sliceNr) * selectionImg->GetSpacing()[1], 0); break;
		}
		m_selectionChannelInitialized = true;
	}
	slicer->update();
}
