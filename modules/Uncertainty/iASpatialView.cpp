#include "iASpatialView.h"

#include "iAImageWidget.h"

#include "QVTKOpenGLWidget.h"

#include <QHBoxLayout>

iASpatialView::iASpatialView(): QWidget()
{
	this->setLayout(new QHBoxLayout());
}

void iASpatialView::AddImage(vtkImagePointer img)
{
	m_images.push_back(new iAImageWidget(img));
	layout()->addWidget(m_images[m_images.size() - 1]);
}
