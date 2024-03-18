// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAPlaneSliceTool.h"

#include <iAAABB.h>
#include <iALog.h>

// guibase
#include <iADataSetRenderer.h>
#include <iADataSetViewer.h>
#include <iADockWidgetWrapper.h>
#include <iAMdiChild.h>
#include <iAMainWindow.h>
#include <iAQVTKWidget.h>
#include <iARenderer.h>
#include <iASlicerMode.h>
#include <iAVolumeViewer.h>
#include <iATransferFunction.h>

#include <vtkCallbackCommand.h>
#include <vtkCamera.h>
#include <vtkColorTransferFunction.h>
#include <vtkConeSource.h>
#include <vtkImageData.h>
#include <vtkImageProperty.h>
#include <vtkImageResliceMapper.h>
#include <vtkImageSlice.h>
#include <vtkInteractorStyleImage.h>
#include <vtkPlaneWidget.h>
#include <vtkPlane.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRendererCollection.h>
#include <vtkSphereSource.h>

#include <QApplication>
#include <QMessageBox>

class iAvtkPlaneWidget : public vtkPlaneWidget
{
public:
	vtkTypeMacro(iAvtkPlaneWidget, vtkPlaneWidget);
	static iAvtkPlaneWidget* New();

	//! The "automatic" handle size from vtkPlaneWidget somehow doesn't work at all for us - it produces far too small handles for large planes.
	//! Also, it completely ignores the user-settable "HandleSize" and only uses the (internal) HandleSizeFactor for resizing
	//! Workaround: specify a fixed handle size in relation to the dataset size.
	void SizeHandles() override
	{		
		for (int i = 0; i < 4; i++)
		{
			this->HandleGeometry[i]->SetRadius(m_handleRadius);
		}
		// Set the height and radius of the cone
		this->ConeSource->SetHeight(2.0 * m_handleRadius);
		this->ConeSource->SetRadius(m_handleRadius);
		this->ConeSource2->SetHeight(2.0 * m_handleRadius);
		this->ConeSource2->SetRadius(m_handleRadius);
	}
	void setHandleRadius(double radius)
	{
		m_handleRadius = radius;
	}
private:
	double m_handleRadius;
};

vtkStandardNewMacro(iAvtkPlaneWidget);

namespace
{
	void updateSlice(vtkCamera* cam, vtkPlaneWidget* planeWidget)
	{
		//vtkNew<vtkPlane> p; tool->m_planeWidget->GetPlane(p); tool->m_reslicer->SetSlicePlane(p); -> leads to vtkImageSlice also rotating around
		// instead, we need to change the camera's parameters to adapt where we slice, instead of setting the slice plane in resliceMapper directly;
		// see https://vtkusers.public.kitware.narkive.com/N0oCzOND/setcutplane-not-working-in-vtkimagereslicemapper
		iAVec3d center(planeWidget->GetCenter());
		iAVec3d origin(planeWidget->GetOrigin());
		iAVec3d normal(planeWidget->GetNormal());
		iAVec3d p1(planeWidget->GetPoint1());
		iAVec3d p2(planeWidget->GetPoint2());
		auto distance = cam->GetDistance();
		auto position = center + distance * normal;
		auto viewUp = p1 - origin;
		cam->SetFocalPoint(center.data());
		cam->SetPosition(position.data());
		cam->SetViewUp(viewUp.data());
	}

	std::array<float, 3> quaternionToEulerAngles(std::array<float, 4> q)
	{
		double ayterm = 2 * (q[3] * q[1] - q[0] * q[2]);
		double a2[3] = {
			vtkMath::DegreesFromRadians(std::atan2(2 * (q[3] * q[0] + q[1] * q[2]), 1 - 2 * (q[0] * q[0] + q[1] * q[1]))),
			vtkMath::DegreesFromRadians(-vtkMath::Pi() / 2 + 2 * std::atan2(std::sqrt(1 + ayterm), std::sqrt(1 - ayterm))),
			vtkMath::DegreesFromRadians(std::atan2(2 * (q[3] * q[2] + q[0] * q[1]), 1 - 2 * (q[1] * q[1] + q[2] * q[2])))
		};
		for (int a = 0; a < 3; ++a)
		{   // round to nearest X degrees (for smoothing):
			const double RoundDegrees = 2;
			a2[a] = std::round(a2[a] / RoundDegrees) * RoundDegrees;
		}
	}

	std::array<float, 4> eulerAnglesToQuaternion(std::array<float, 3> a)
	{
		// from https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
		float cr = std::cos(a[0] * 0.5);
		float sr = std::sin(a[0] * 0.5);
		float cp = std::cos(a[1] * 0.5);
		float sp = std::sin(a[1] * 0.5);
		float cy = std::cos(a[2] * 0.5);
		float sy = std::sin(a[2] * 0.5);
		return std::array<float, 4>
		{
			cr * cp * cy + sr * sp * sy,
			sr * cp * cy - cr * sp * sy,
			cr * sp * cy + sr * cp * sy,
			cr * cp * sy - sr * sp * cy
		};
	}
	void logValues(vtkPlaneWidget* planeWidget)
	{
		double const * center = planeWidget->GetCenter();
		double const * normal = planeWidget->GetNormal();
		//vtkNew<vtkPlane> plane;
		//planeWidget->GetPlane(plane);
		//auto t = plane->GetTransform();
		//t->Get
		LOG(lvlInfo, QString("Plane: center=(%1, %2, %3), normal=(%4, %5, %6)")
			.arg(center[0]).arg(center[1]).arg(center[2])
			.arg(normal[0]).arg(normal[1]).arg(normal[2]));
	}
}

iAPlaneSliceTool::iAPlaneSliceTool(iAMainWindow* mainWnd, iAMdiChild* child) :
	iATool(mainWnd, child),
	m_sliceWidget(new iAQVTKWidget(child)),
	m_dw(new iADockWidgetWrapper(m_sliceWidget, "Arbitrary Slice", "ArbitrarySliceViewer")),
	m_planeWidget(vtkSmartPointer<iAvtkPlaneWidget>::New()),
	m_reslicer(vtkSmartPointer<vtkImageResliceMapper>::New()),
	m_imageSlice(vtkSmartPointer<vtkImageSlice>::New())
{
	auto ds = child->firstImageDataSetIdx();
	if (ds == iAMdiChild::NoDataSet)
	{
		QMessageBox::warning(mainWnd, "Arbitrary slicing tool", "No image dataset loaded!");
		return;
	}
	child->splitDockWidget(child->slicerDockWidget(iASlicerMode::XY), m_dw, Qt::Horizontal);

	//m_planeWidget->SetDefaultRenderer(child->renderer()->renderer());
	
	auto bounds = child->dataSetViewer(ds)->renderer()->bounds();
	auto lengths = bounds.maxCorner() - bounds.minCorner();
	auto maxSideLen = std::max(std::max(lengths.x(), lengths.y()), lengths.z());
	auto handleSize = maxSideLen / 40.0;
	auto objCenter = (bounds.maxCorner() - bounds.minCorner()) / 2;
	LOG(lvlInfo, QString("handle size: %1").arg(handleSize));
	m_planeWidget->SetOrigin(bounds.minCorner().x(), bounds.minCorner().y(), objCenter.z());
	m_planeWidget->SetPoint1(bounds.maxCorner().x(), bounds.minCorner().y(), objCenter.z());
	m_planeWidget->SetPoint2(bounds.minCorner().x(), bounds.maxCorner().y(), objCenter.z());
	m_planeWidget->SetInteractor(child->renderer()->interactor());
	m_planeWidget->SetRepresentationToSurface();
	m_planeWidget->On();
	
	// set to middle of object in z direction (i.e. xy slice default position):
	// set handle size to a 20th of the longest dataset size:
	//m_planeWidget->GetHandleProperty()->
	m_planeWidget->setHandleRadius(handleSize);
	m_planeWidget->SizeHandles();

	logValues(m_planeWidget);

	m_reslicer->SetInputData(child->firstImageData());
	m_reslicer->SetSliceFacesCamera(true);
	m_reslicer->SetSliceAtFocalPoint(true);

	auto imgProp = m_imageSlice->GetProperty();
	auto tf = dynamic_cast<iAVolumeViewer*>(m_child->dataSetViewer(ds))->transfer();
	imgProp->SetLookupTable(tf->colorTF());
	m_imageSlice->SetMapper(m_reslicer);
	m_imageSlice->SetProperty(imgProp);
	vtkNew<vtkRenderer> ren;
	ren->AddViewProp(m_imageSlice);
	ren->ResetCamera();
	auto bgc = QApplication::palette().color(QPalette::Window);
	ren->SetBackground(bgc.redF(), bgc.greenF(), bgc.blueF());

	m_sliceWidget->renderWindow()->AddRenderer(ren);
	vtkNew<vtkInteractorStyleImage> style;
	m_sliceWidget->renderWindow()->GetInteractor()->SetInteractorStyle(style);
	//m_planeWidget->SetHandleSize(0.1); // no effect, plane widget automatically sets handle sizes

	updateSlice(m_sliceWidget->renderWindow()->GetRenderers()->GetFirstRenderer()->GetActiveCamera(), m_planeWidget);
	child->updateRenderer();

	vtkNew<vtkCallbackCommand> modifiedCallback;
	modifiedCallback->SetCallback(
		[](vtkObject* vtkNotUsed(caller), long unsigned int vtkNotUsed(eventId), void* clientData,
			void* vtkNotUsed(callData))
		{
			auto tool = reinterpret_cast<iAPlaneSliceTool*>(clientData);
			auto cam = tool->m_sliceWidget->renderWindow()->GetRenderers()->GetFirstRenderer()->GetActiveCamera();
			updateSlice(cam, tool->m_planeWidget);
			tool->m_sliceWidget->interactor()->Render();
			logValues(tool->m_planeWidget);
		});
	modifiedCallback->SetClientData(this);
	m_planeWidget->AddObserver(vtkCommand::InteractionEvent, modifiedCallback);
}

iAPlaneSliceTool::~iAPlaneSliceTool()
{
	delete m_dw;
}

quint64 iAPlaneSliceTool::addSnapshot(iASnapshotInfo info)
{
	auto id = m_nextSnapshotID++;
	m_snapshots[id] = info;
	return id;
}

void iAPlaneSliceTool::removeSnapshot(quint64 id)
{
	m_snapshots.erase(id);
}

void iAPlaneSliceTool::clearSnapshots()
{
	m_snapshots.clear();
}

void iAPlaneSliceTool::moveSlice(quint64 id, iAMoveAxis axis, float value)
{
	auto& s = m_snapshots[id];
	// apply shift
}


const QString iAPlaneSliceTool::Name("Arbitrary Slice Plane");