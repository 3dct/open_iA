/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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
#include "iADataSetRendererImpl.h"

#include "iADataSet.h"
#include "iADataSetRenderer.h"
#include "iAMainWindow.h"

#include "iAAABB.h"
#ifndef NDEBUG
#include "iAMathUtility.h"    // for dblApproxEqual
#endif

// ---------- iAGraphRenderer ----------

#include <vtkActor.h>
#include <vtkCallbackCommand.h>
#include <vtkGlyph3DMapper.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkSphereSource.h>

#include <QColor>
#include <QVector>

namespace
{
	const QString PointRadius = "Point Radius";
	const QString PointColor = "Point Color";
	const QString LineColor = "Line Color";
	const QString LineWidth = "Line Width";

}

iAGraphRenderer::iAGraphRenderer(vtkRenderer* renderer, iAGraphData const * data) :
	iADataSetRenderer(renderer, true),
	m_lineActor(vtkSmartPointer<vtkActor>::New()),
	m_pointActor(vtkSmartPointer<vtkActor>::New()),
	m_data(data)
{
	auto lineMapper = vtkPolyDataMapper::New();
	lineMapper->SetInputData(data->poly());
	m_lineActor->SetMapper(lineMapper);
	m_lineActor->GetProperty()->SetColor(0.0, 1.0, 0.0);

	// Glyph the points
	m_sphereSource = vtkSphereSource::New();
	m_sphereSource->SetPhiResolution(21);
	m_sphereSource->SetThetaResolution(21);
	m_sphereSource->SetRadius(5);
	vtkNew<vtkPoints> pointsPoints;
	pointsPoints->DeepCopy(data->poly()->GetPoints());
	vtkNew<vtkPolyData> glyphPoints;
	glyphPoints->SetPoints(pointsPoints);
	auto pointMapper = vtkGlyph3DMapper::New();
	pointMapper->SetInputData(glyphPoints);
	pointMapper->SetSourceConnection(m_sphereSource->GetOutputPort());
	m_pointActor->SetMapper(pointMapper);
	m_pointActor->SetPickable(false);
	m_pointActor->GetProperty()->SetColor(1.0, 0.0, 0.0);

	addAttribute(PointRadius, iAValueType::Continuous, 5, 0.001, 100000000);
	addAttribute(PointColor, iAValueType::Color, "#FF0000");
	addAttribute(LineColor, iAValueType::Color, "#00FF00");
	addAttribute(LineWidth, iAValueType::Continuous, 1.0, 0.1, 100);

	// adapt bounding box to changes in position/orientation of volume:
	// idea how to connect lambda to observer from https://gist.github.com/esmitt/7ca96193f2c320ba438e0453f9136c20
	vtkNew<vtkCallbackCommand> modifiedCallback;
	modifiedCallback->SetCallback(
		[](vtkObject* vtkNotUsed(caller), long unsigned int vtkNotUsed(eventId), void* clientData,
			void* vtkNotUsed(callData))
		{
			auto graphRenderer = reinterpret_cast<iAGraphRenderer*>(clientData);
			graphRenderer->updateOutlineTransform();
			graphRenderer->updatePointRendererPosOri();
		});
	modifiedCallback->SetClientData(this);
	m_lineActor->AddObserver(vtkCommand::ModifiedEvent, modifiedCallback);
	iAGraphRenderer::showDataSet();
}

void iAGraphRenderer::showDataSet()
{
	m_renderer->AddActor(m_lineActor);
	m_renderer->AddActor(m_pointActor);
}

void iAGraphRenderer::hideDataSet()
{
	m_renderer->RemoveActor(m_pointActor);
	m_renderer->RemoveActor(m_lineActor);
}

void iAGraphRenderer::updatePointRendererPosOri()
{
	m_pointActor->SetPosition(m_lineActor->GetPosition());
	m_pointActor->SetOrientation(m_lineActor->GetOrientation());
}

void iAGraphRenderer::applyAttributes(QVariantMap const& values)
{
	m_sphereSource->SetRadius(values[PointRadius].toDouble());
	QColor pointColor(values[PointColor].toString());
	m_pointActor->GetProperty()->SetColor(pointColor.redF(), pointColor.greenF(), pointColor.blueF());
	m_sphereSource->Update();
	QColor lineColor(values[LineColor].toString());
	m_lineActor->GetProperty()->SetColor(lineColor.redF(), lineColor.greenF(), lineColor.blueF());
	m_lineActor->GetProperty()->SetLineWidth(values[LineWidth].toFloat());

	QVector<double> pos = values[Position].value<QVector<double>>();
	QVector<double> ori = values[Orientation].value<QVector<double>>();
	assert(pos.size() == 3);
	assert(ori.size() == 3);
	m_pointActor->SetPosition(pos.data());
	m_pointActor->SetOrientation(ori.data());
	m_lineActor->SetPosition(pos.data());
	m_lineActor->SetOrientation(ori.data());

	m_lineActor->SetPickable(values[Pickable].toBool());
	//m_pointActor->SetPickable(values[Pickable].toBool()); // both move together same as bounds
}

iAAABB iAGraphRenderer::bounds()
{
	return iAAABB(m_data->poly()->GetBounds());
}

double const* iAGraphRenderer::orientation() const
{
#ifndef NDEBUG
	auto o1 = m_pointActor->GetOrientation(), o2 = m_lineActor->GetOrientation();
	assert(dblApproxEqual(o1[0], o2[0], 1e-6) && dblApproxEqual(o1[1], o2[1], 1e-6) && dblApproxEqual(o1[2], o2[2], 1e-6));
#endif
	return m_pointActor->GetOrientation();
}

double const* iAGraphRenderer::position() const
{
#ifndef NDEBUG
	auto p1 = m_pointActor->GetPosition(), p2 = m_lineActor->GetPosition();
	assert(dblApproxEqual(p1[0], p2[0], 1e-6) && dblApproxEqual(p1[1], p2[1], 1e-6) && dblApproxEqual(p1[2], p2[2], 1e-6));
#endif
	return m_lineActor->GetPosition();
}

void iAGraphRenderer::setPosition(double pos[3])
{
	m_lineActor->SetPosition(pos);
	m_pointActor->SetPosition(pos);
}

void iAGraphRenderer::setOrientation(double ori[3])
{
	m_lineActor->SetOrientation(ori);
	m_pointActor->SetOrientation(ori);
}

vtkProp3D* iAGraphRenderer::vtkProp()
{
	return m_lineActor;
}



// ---------- iAMeshRenderer ----------

namespace
{
	const QString PolyColor = "Color";
	const QString PolyOpacity = "Opacity";
	const QString PolyWireframe = "Wireframe";
}

iAPolyActorRenderer::iAPolyActorRenderer(vtkRenderer* renderer) :
	iADataSetRenderer(renderer, true),
	m_polyActor(vtkSmartPointer<vtkActor>::New())
{
	vtkNew<vtkPolyDataMapper> mapper;
	mapper->SetScalarModeToUsePointFieldData();
	m_polyActor->SetMapper(mapper);

	// adapt bounding box to changes in position/orientation of volume:
	vtkNew<vtkCallbackCommand> modifiedCallback;
	modifiedCallback->SetCallback(
		[](vtkObject* vtkNotUsed(caller), long unsigned int vtkNotUsed(eventId), void* clientData,
			void* vtkNotUsed(callData))
		{
			reinterpret_cast<iAPolyActorRenderer*>(clientData)->updateOutlineTransform();
		});
	modifiedCallback->SetClientData(this);
	m_polyActor->AddObserver(vtkCommand::ModifiedEvent, modifiedCallback);

	addAttribute(Shading, iAValueType::Boolean, true);
	addAttribute(PolyColor, iAValueType::Color, "#FFFFFF");
	addAttribute(PolyOpacity, iAValueType::Continuous, 1.0, 0.0, 1.0);
	addAttribute(PolyWireframe, iAValueType::Boolean, false);
	iAPolyActorRenderer::showDataSet();
}

void iAPolyActorRenderer::showDataSet()
{
	m_renderer->AddActor(m_polyActor);
}

void iAPolyActorRenderer::hideDataSet()
{
	m_renderer->RemoveActor(m_polyActor);
}

void iAPolyActorRenderer::applyAttributes(QVariantMap const& values)
{
	applyLightingProperties(m_polyActor->GetProperty(), values);
	m_polyActor->GetProperty()->SetShading(values[Shading].toBool());

	QColor color(values[PolyColor].toString());
	double opacity = values[PolyOpacity].toDouble();
	m_polyActor->GetProperty()->SetColor(color.redF(), color.greenF(), color.blueF());
	m_polyActor->GetProperty()->SetOpacity(opacity);
	if (values[PolyWireframe].toBool())
	{
		m_polyActor->GetProperty()->SetRepresentationToWireframe();
	}
	else
	{
		m_polyActor->GetProperty()->SetRepresentationToSurface();
	}

	QVector<double> pos = values[Position].value<QVector<double>>();
	QVector<double> ori = values[Orientation].value<QVector<double>>();
	assert(pos.size() == 3);
	assert(ori.size() == 3);
	m_polyActor->SetPosition(pos.data());
	m_polyActor->SetOrientation(ori.data());
	m_polyActor->SetPickable(values[Pickable].toBool());
}

double const* iAPolyActorRenderer::orientation() const
{
	return m_polyActor->GetOrientation();
}

double const* iAPolyActorRenderer::position() const
{
	return m_polyActor->GetPosition();
}

void iAPolyActorRenderer::setPosition(double pos[3])
{
	m_polyActor->SetPosition(pos);
}

void iAPolyActorRenderer::setOrientation(double ori[3])
{
	m_polyActor->SetOrientation(ori);
}

vtkProp3D* iAPolyActorRenderer::vtkProp()
{
	return m_polyActor;
}

iAPolyDataRenderer::iAPolyDataRenderer(vtkRenderer* renderer, iAPolyData const * data) :
	iAPolyActorRenderer(renderer),
	m_data(data)
{
	dynamic_cast<vtkPolyDataMapper*>(m_polyActor->GetMapper())->SetInputData(data->poly());
	//m_polyMapper->SelectColorArray("Colors");
}

iAAABB iAPolyDataRenderer::bounds()
{
	return iAAABB(m_data->poly()->GetBounds());
}


#include "iAGeometricObject.h"

iAGeometricObjectRenderer::iAGeometricObjectRenderer(vtkRenderer* renderer, iAGeometricObject const * data) :
	iAPolyActorRenderer(renderer),
	m_data(data)
{
	m_polyActor->GetMapper()->SetInputConnection(m_data->source()->GetOutputPort());
	//m_polyMapper->SelectColorArray("Colors");
}
iAAABB iAGeometricObjectRenderer::bounds()
{
	return iAAABB(m_data->bounds());
}
