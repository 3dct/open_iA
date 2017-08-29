#include "iASpatialView.h"

#include "iAImageWidget.h"

#include "QVTKOpenGLWidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

iASpatialView::iASpatialView(): QWidget()
{
	this->setLayout(new QHBoxLayout());
}

void iASpatialView::AddImage(QString const & caption, vtkImagePointer img)
{
	auto imgWidget = new iAImageWidget(img);
	// m_images.push_back(imgWidget);
	QWidget*  container = new QWidget();
	container->setLayout(new QVBoxLayout());
	auto label = new QLabel(caption);
	label->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
	label->setAlignment(Qt::AlignHCenter);
	container->layout()->addWidget(label);
	container->layout()->addWidget(imgWidget);
	layout()->addWidget(container);
}
