#include "iAFrustumActor.h"

#include "iAVec3.h"

#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkCubeSource.h>
#include <vtkSphereSource.h>
#include <vtkRenderer.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkTransform.h>

iAFrustumActor::iAFrustumActor(vtkRenderer* ren, vtkCamera* cam, double size) :
	m_ren(ren),
	m_cam(cam),
	m_size(size),
	m_camPosActor(vtkSmartPointer<vtkActor>::New()),
	m_camDirActor(vtkSmartPointer<vtkActor>::New()),
	m_camPosSource(vtkSmartPointer<vtkSphereSource>::New()),
	m_camDirSource(vtkSmartPointer<vtkCubeSource>::New()),
	m_visible(false)
{
	vtkNew<vtkPolyDataMapper> camPosMapper;
	m_camPosSource->SetRadius(0.5 * size);
	camPosMapper->SetInputConnection(m_camPosSource->GetOutputPort());
	m_camPosActor->SetMapper(camPosMapper);
	m_camPosActor->GetProperty()->SetOpacity(0.2);
	m_camPosActor->GetProperty()->SetColor(0, 255, 0);

	vtkNew<vtkPolyDataMapper> camDirMapper;
	m_camDirSource->SetXLength(size * 2);
	m_camDirSource->SetYLength(size * 0.5);
	m_camDirSource->SetZLength(size * 0.5);
	camDirMapper->SetInputConnection(m_camDirSource->GetOutputPort());
	m_camDirActor->SetMapper(camDirMapper);
	m_camDirActor->GetProperty()->SetOpacity(0.2);
	m_camDirActor->GetProperty()->SetColor(0, 255, 0);

	cam->AddObserver(vtkCommand::ModifiedEvent, this);
	m_timer.start();
}

void iAFrustumActor::Execute(vtkObject*, unsigned long, void*)
{
	const int UpdateIntervalMS = 40;
	if (m_timer.elapsed() < UpdateIntervalMS)
	{	// rate-limit the update to 25 fps
		return;
	}
	iAVec3d pos(m_cam->GetPosition());
	iAVec3d dir(m_cam->GetDirectionOfProjection());
	dir.normalize();
	// move a little from camera pos towards focal point
	iAVec3d dirVecPos(pos + dir * m_size * 0.5);
	//double const* up = m_cam->GetViewUp();
	//double const* ori = m_cam->GetOrientation();
	
	m_camPosSource->SetCenter(pos.data());

	//m_camDirActor->SetPosition(dirVecPos[0], dirVecPos[1], dirVecPos[2]);
	//m_camDirActor->SetOrientation(ori[2], ori[1], ori[0]);

	iAVec3d initDir(1, 0, 0);
	auto axisVec = crossProduct(initDir, dir);
	auto angle = std::acos(dotProduct(initDir, dir));
	vtkNew<vtkTransform> t;
	//t->PostMultiply();
	t->PreMultiply();
	t->RotateWXYZ(angle, axisVec.data());
	t->Translate(dirVecPos.data());
	m_camDirActor->SetUserTransform(t);
	
	m_timer.start();

	emit updateRequired();
}

void iAFrustumActor::show()
{
	if (m_visible)
	{
		return;
	}
	m_ren->AddActor(m_camPosActor);
	m_ren->AddActor(m_camDirActor);
	m_visible = true;
}

void iAFrustumActor::hide()
{
	if (!m_visible)
	{
		return;
	}
	m_ren->RemoveActor(m_camPosActor);
	m_ren->RemoveActor(m_camDirActor);
	m_visible = false;
}

