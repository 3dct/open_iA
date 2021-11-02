#include "iAFrustumActor.h"

#include "vtkRenderer.h"
#include "vtkCamera.h"
#include "vtkCameraActor.h"
#include "vtkActor.h"
#include <vtkFrustumSource.h>
#include <vtkMapper.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkPlanes.h>
#include <vtkShrinkPolyData.h>

iAFrustumActor::iAFrustumActor(vtkRenderer* ren, vtkCamera* cam):
	m_ren(ren), m_cam(cam), m_frustumActor(vtkSmartPointer<vtkActor>::New())
{
	m_visible = false;
	setupFrustumActor();
}

void iAFrustumActor::show()
{
	if (m_visible)
	{
		return;
	}
	m_ren->AddActor(m_frustumActor);
	m_visible = true;
}

void iAFrustumActor::hide()
{
	if (!m_visible)
	{
		return;
	}
	m_ren->RemoveActor(m_frustumActor);
	m_visible = false;
}

//! Computes the frustum based on the given camera and creates a actor
void iAFrustumActor::setupFrustumActor()
{
	double planesArray[24];
	double aspect = m_ren->GetAspect()[0] / m_ren->GetAspect()[1];

	m_cam->GetFrustumPlanes(aspect, planesArray);

	vtkNew<vtkPlanes> planes;
	planes->SetFrustumPlanes(planesArray);

	vtkNew<vtkFrustumSource> frustumSource;
	frustumSource->ShowLinesOff();
	frustumSource->SetPlanes(planes);

	vtkNew<vtkShrinkPolyData> shrink;
	shrink->SetInputConnection(frustumSource->GetOutputPort());
	shrink->SetShrinkFactor(.9);

	vtkNew<vtkPolyDataMapper> mapper;
	mapper->SetInputConnection(shrink->GetOutputPort());

	m_frustumActor->SetMapper(mapper);
	m_frustumActor->GetProperty()->SetColor(1.0, 0.50, 0.0);
	m_frustumActor->GetProperty()->EdgeVisibilityOn();
}
