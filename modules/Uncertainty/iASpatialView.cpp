#include "iASpatialView.h"

#include "iAImageWidget.h"
#include "iASlicerMode.h"

#include "QVTKOpenGLWidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

iASpatialView::iASpatialView(): QWidget()
{
	xyButton = new QPushButton("XY");
	xzButton = new QPushButton("XZ");
	yzButton = new QPushButton("YZ");
	xyButton->setCheckable(true);
	xzButton->setCheckable(true);
	yzButton->setCheckable(true);
	xyButton->setChecked(true);
	connect(xyButton, SIGNAL(clicked()), this, SLOT(xyClicked()));
	connect(xzButton, SIGNAL(clicked()), this, SLOT(xzClicked()));
	connect(yzButton, SIGNAL(clicked()), this, SLOT(yzClicked()));

	m_sliceControl = new QSpinBox();
	m_sliceControl->setMaximum(0);
	connect(m_sliceControl, SIGNAL(valueChanged(int)), this, SLOT(sliceChanged(int)));

	m_sliceBar = new QWidget();
	m_sliceBar->setLayout(new QHBoxLayout());
	m_sliceBar->layout()->addWidget(xyButton);
	m_sliceBar->layout()->addWidget(xzButton);
	m_sliceBar->layout()->addWidget(yzButton);
	m_sliceBar->layout()->addWidget(m_sliceControl);

	m_contentWidget = new QWidget();
	m_contentWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
	m_contentWidget->setLayout(new QHBoxLayout());
	m_contentWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding);

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
	xyButton->setChecked(true);
	xzButton->setChecked(false);
	yzButton->setChecked(false);
	for (int i = 0; i < m_images.size(); ++i)
	{
		m_imageWidgets[i]->SetMode(iASlicerMode::XY);
	}
	if (m_imageWidgets.size() > 0)
		m_sliceControl->setMaximum(m_imageWidgets[0]->GetSliceCount()-1);
}

void iASpatialView::xzClicked()
{
	xzButton->setChecked(false);
	xyButton->setChecked(false);
	yzButton->setChecked(false);
	for (int i = 0; i < m_images.size(); ++i)
	{
		m_imageWidgets[i]->SetMode(iASlicerMode::XZ);
	}
	if (m_imageWidgets.size() > 0)
		m_sliceControl->setMaximum(m_imageWidgets[0]->GetSliceCount()-1);
}

void iASpatialView::yzClicked()
{
	yzButton->setChecked(true);
	xyButton->setChecked(false);
	xyButton->setChecked(false);
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
