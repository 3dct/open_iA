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
#include "iAVolumeSettings.h"

#include "iAAABB.h"
#ifndef NDEBUG
#include "iAMathUtility.h"    // for dblApproxEqual
#endif
#include "iAValueTypeVectorHelpers.h"

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

	template <class T>
	void applyLightingProperties(T* prop, QVariantMap const& values)
	{
		prop->SetAmbient(values[iADataSetRenderer::AmbientLighting].toDouble());
		prop->SetDiffuse(values[iADataSetRenderer::DiffuseLighting].toDouble());
		prop->SetSpecular(values[iADataSetRenderer::SpecularLighting].toDouble());
		prop->SetSpecularPower(values[iADataSetRenderer::SpecularPower].toDouble());
	}
}

class iAGraphRenderer : public iADataSetRenderer
{
public:
	iAGraphRenderer(vtkRenderer* renderer, iAGraphData* data) :
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
	void showDataSet() override
	{
		m_renderer->AddActor(m_lineActor);
		m_renderer->AddActor(m_pointActor);
	}
	void hideDataSet() override
	{
		m_renderer->RemoveActor(m_pointActor);
		m_renderer->RemoveActor(m_lineActor);
	}
	void updatePointRendererPosOri()
	{
		m_pointActor->SetPosition(m_lineActor->GetPosition());
		m_pointActor->SetOrientation(m_lineActor->GetOrientation());
	}

	void applyAttributes(QVariantMap const& values) override
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

	iAAABB bounds() override
	{
		return iAAABB(m_data->poly()->GetBounds());
	}
	double const* orientation() const override
	{
#ifndef NDEBUG
		auto o1 = m_pointActor->GetOrientation(), o2 = m_lineActor->GetOrientation();
		assert(dblApproxEqual(o1[0], o2[0], 1e-6) && dblApproxEqual(o1[1], o2[1], 1e-6) && dblApproxEqual(o1[2], o2[2], 1e-6));
#endif
		return m_pointActor->GetOrientation();
	}
	double const* position() const override
	{
#ifndef NDEBUG
		auto p1 = m_pointActor->GetPosition(), p2 = m_lineActor->GetPosition();
		assert(dblApproxEqual(p1[0], p2[0], 1e-6) && dblApproxEqual(p1[1], p2[1], 1e-6) && dblApproxEqual(p1[2], p2[2], 1e-6));
#endif
		return m_lineActor->GetPosition();
	}
	void setPosition(double pos[3]) override
	{
		m_lineActor->SetPosition(pos);
		m_pointActor->SetPosition(pos);
	}
	void setOrientation(double ori[3]) override
	{
		m_lineActor->SetOrientation(ori);
		m_pointActor->SetOrientation(ori);
	}
	vtkProp3D* vtkProp() override
	{
		return m_lineActor;
	}

private:
	vtkSmartPointer<vtkActor> m_lineActor, m_pointActor;
	vtkSmartPointer<vtkSphereSource> m_sphereSource;
	iAGraphData* m_data;
};



// ---------- iAMeshRenderer ----------

namespace
{
	const QString PolyColor = "Color";
	const QString PolyOpacity = "Opacity";
	const QString PolyWireframe = "Wireframe";
}

class iAPolyActorRenderer : public iADataSetRenderer
{
public:
	iAPolyActorRenderer(vtkRenderer* renderer) :
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
	void showDataSet() override
	{
		m_renderer->AddActor(m_polyActor);
	}
	void hideDataSet() override
	{
		m_renderer->RemoveActor(m_polyActor);
	}
	void applyAttributes(QVariantMap const& values) override
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
	double const* orientation() const override
	{
		return m_polyActor->GetOrientation();
	}
	double const* position() const override
	{
		return m_polyActor->GetPosition();
	}
	void setPosition(double pos[3]) override
	{
		m_polyActor->SetPosition(pos);
	}
	void setOrientation(double ori[3]) override
	{
		m_polyActor->SetOrientation(ori);
	}
	vtkProp3D* vtkProp() override
	{
		return m_polyActor;
	}

protected:
	vtkSmartPointer<vtkActor> m_polyActor;
};

class iAPolyDataRenderer : public iAPolyActorRenderer
{
public:
	iAPolyDataRenderer(vtkRenderer* renderer, iAPolyData* data) :
		iAPolyActorRenderer(renderer),
		m_data(data)
	{
		dynamic_cast<vtkPolyDataMapper*>(m_polyActor->GetMapper())->SetInputData(data->poly());
		//m_polyMapper->SelectColorArray("Colors");
	}
	iAAABB bounds() override
	{
		return iAAABB(m_data->poly()->GetBounds());
	}
private:
	iAPolyData* m_data;
};

#include "iAGeometricObject.h"

class iAGeometricObjectRenderer : public iAPolyActorRenderer
{
public:
	iAGeometricObjectRenderer(vtkRenderer* renderer, iAGeometricObject* data) :
		iAPolyActorRenderer(renderer),
		m_data(data)
	{
		m_polyActor->GetMapper()->SetInputConnection(m_data->source()->GetOutputPort());
		//m_polyMapper->SelectColorArray("Colors");
	}
	iAAABB bounds() override
	{
		return iAAABB(m_data->bounds());
	}
private:
	iAGeometricObject* m_data;
};

// ---------- iAVolRenderer ----------
//#include "iAChartWithFunctionsWidget.h"
#include "iAPreferences.h"
#include "iATransferFunction.h"

#include <vtkImageData.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>
#include <vtkSmartVolumeMapper.h>

namespace
{
	const QString LinearInterpolation = "Linear interpolation";
	const QString ScalarOpacityUnitDistance = "Scalar Opacity Unit Distance";
	const QString RendererType = "Renderer type";
	const QString Spacing = "Spacing";
	const QString InteractiveAdjustSampleDistance = "Interactively Adjust Sample Distances";
	const QString AutoAdjustSampleDistance = "Auto-Adjust Sample Distances";
	const QString SampleDistance = "Sample distance";
	const QString InteractiveUpdateRate = "Interactive Update Rate";
	const QString FinalColorLevel = "Final Color Level";
	const QString FinalColorWindow = "Final Color Window";
	// VTK 9.2
	//const QString GlobalIlluminationReach = "Global Illumination Reach";
	//const QString VolumetricScatteringBlending = "VolumetricScatteringBlending";
}

// ---------- Factory method ----------

std::shared_ptr<iADataSetRenderer> createDataRenderer(iADataSet* dataSet, iADataForDisplay* dataForDisplay, vtkRenderer* renderer)
{
	auto img = dynamic_cast<iAImageData*>(dataSet);
	if (img)
	{
		auto volDataForDisplay = dynamic_cast<iAImageDataForDisplay*>(dataForDisplay);
		if (!volDataForDisplay)
		{
			LOG(lvlWarn, QString("Required additional data for displaying volume couldn't be created!"));
			return {};
		}
		return std::make_shared<iAVolRenderer>(renderer, img, volDataForDisplay);
	}
	auto graph = dynamic_cast<iAGraphData*>(dataSet);
	if (graph)
	{
		return std::make_shared<iAGraphRenderer>(renderer, graph);
	}
	auto mesh = dynamic_cast<iAPolyData*>(dataSet);
	if (mesh)
	{
		return std::make_shared<iAPolyDataRenderer>(renderer, mesh);
	}
	auto geometricObject = dynamic_cast<iAGeometricObject*>(dataSet);
	if (geometricObject)
	{
		return std::make_shared<iAGeometricObjectRenderer>(renderer, geometricObject);
	}

	LOG(lvlWarn, QString("Requested renderer for unknown dataset type!"));
	return {};
}
