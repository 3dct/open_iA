#include "iASpatialView.h"

#include "QVTKOpenGLWidget.h"

iASpatialView::iASpatialView():
	m_vtkWidget(new QVTKOpenGLWidget())
{
}

void iASpatialView::ShowImage(ImagePointer img)
{

}

void iASpatialView::ROISelected(iAImageCoordinate topLeftFront, iAImageCoordinate bottomRightBack)
{

}
