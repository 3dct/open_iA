#include "iAFrustumActor.h"

#include "vtkRenderer.h"
#include "vtkCamera.h"
#include "vtkCameraActor.h"
#include "vtkCameraActor.h"
#include <vtkFrustumSource.h>
#include <vtkMapper.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkPlanes.h>
#include <vtkShrinkPolyData.h>

iAFrustumActor::iAFrustumActor(vtkRenderer* ren, vtkCamera* cam):
	m_ren(ren), m_cam(cam), m_frustumActor(vtkSmartPointer<vtkCameraActor>::New())
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
	m_frustumActor->SetCamera(m_cam);
}
