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
#include "iADataSetRenderer.h"

#include "iAAABB.h"
#include "iADataSet.h"
#include "iADataForDisplay.h"
#ifndef _NDEBUG
#include "iAMathUtility.h"    // for dblApproxEqual
#endif

#include "iAMainWindow.h"

#include <vtkActor.h>
#include <vtkCallbackCommand.h>
#include <vtkCommand.h>
#include <vtkCubeSource.h>
#include <vtkOpenGLRenderer.h>
#include <vtkOutlineFilter.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkSphereSource.h>

#include <QColor>

class iAOutlineImpl
{
public:
	iAOutlineImpl(iAAABB const& box, vtkRenderer* renderer, QColor const & c): m_renderer(renderer)
	{
		setBounds(box);
		m_mapper->SetInputConnection(m_cubeSource->GetOutputPort());
		m_actor->GetProperty()->SetRepresentationToWireframe();
		m_actor->GetProperty()->SetShading(false);
		m_actor->GetProperty()->SetOpacity(1);
		//m_actor->GetProperty()->SetLineWidth(2);
		m_actor->GetProperty()->SetAmbient(1.0);
		m_actor->GetProperty()->SetDiffuse(0.0);
		m_actor->GetProperty()->SetSpecular(0.0);
		setColor(c);
		m_actor->SetPickable(false);
		m_actor->SetMapper(m_mapper);
		renderer->AddActor(m_actor);
	}
	void setVisible(bool visible)
	{
		if (visible)
		{
			m_renderer->AddActor(m_actor);
		}
		else
		{
			m_renderer->RemoveActor(m_actor);
		}
	}
	void setOrientationAndPosition(QVector<double> pos, QVector<double> ori)
	{
		assert(pos.size() == 3);
		assert(ori.size() == 3);
		m_actor->SetPosition(pos.data());
		m_actor->SetOrientation(ori.data());
	}
	void setBounds(iAAABB const& box)
	{
		m_cubeSource->SetBounds(
			box.minCorner().x(), box.maxCorner().x(),
			box.minCorner().y(), box.maxCorner().y(),
			box.minCorner().z(), box.maxCorner().z()
		);
	}
	void setColor(QColor const& c)
	{
		m_actor->GetProperty()->SetColor(c.redF(), c.greenF(), c.blueF());
	}
	QColor color() const
	{
		auto rgb = m_actor->GetProperty()->GetColor();
		return QColor(rgb[0], rgb[1], rgb[2]);
	}

private:
	vtkNew<vtkCubeSource> m_cubeSource;
	vtkNew<vtkPolyDataMapper> m_mapper;
	vtkNew<vtkActor> m_actor;
	vtkRenderer* m_renderer;
};


namespace
{
	const QString Position("Position");
	const QString Orientation("Orientation");
	const QString OutlineColor("Box Color");
	const QString Pickable("Pickable");
	const QColor OutlineDefaultColor(Qt::black);
	const QString Shading = "Shading";

	const QString AmbientLighting = "Ambient lighting";
	const QString DiffuseLighting = "Diffuse lighting";
	const QString SpecularLighting = "Specular lighting";
	const QString SpecularPower = "Specular power";

	template <class T>
	void applyLightingProperties(T* prop, QMap<QString, QVariant> const & values)
	{
		prop->SetAmbient(values[AmbientLighting].toDouble());
		prop->SetDiffuse(values[DiffuseLighting].toDouble());
		prop->SetSpecular(values[SpecularLighting].toDouble());
		prop->SetSpecularPower(values[SpecularPower].toDouble());
	}
}


#include "iAVolumeSettings.h"

iADataSetRenderer::iADataSetRenderer(vtkRenderer* renderer):
	m_renderer(renderer),
	m_visible(false)
{
	addAttribute(Position, iAValueType::Vector3, QVariant::fromValue(QVector<double>({0, 0, 0})));
	addAttribute(Orientation, iAValueType::Vector3, QVariant::fromValue(QVector<double>({0, 0, 0})));
	addAttribute(OutlineColor, iAValueType::Color, OutlineDefaultColor);
	addAttribute(Pickable, iAValueType::Boolean, true);

	auto volumeSettings = iAMainWindow::get()->defaultVolumeSettings();
	addAttribute(AmbientLighting, iAValueType::Continuous, volumeSettings.AmbientLighting);
	addAttribute(DiffuseLighting, iAValueType::Continuous, volumeSettings.DiffuseLighting);
	addAttribute(SpecularLighting, iAValueType::Continuous, volumeSettings.SpecularLighting);
	addAttribute(SpecularPower, iAValueType::Continuous, volumeSettings.SpecularPower);
}

iADataSetRenderer::~iADataSetRenderer()
{}

void iADataSetRenderer::setAttributes(QMap<QString, QVariant> const & values)
{
	m_attribValues = values;
	applyAttributes(values);
	if (m_outline)
	{
		m_outline->setBounds(bounds());	// only potentially changes for volume currently; maybe use signals instead?
		m_outline->setColor(m_attribValues[OutlineColor].value<QColor>());
		updateOutlineTransform();
	}
}

void iADataSetRenderer::setPickable(bool pickable)
{
	m_attribValues[Pickable] = pickable;
	// TODO: maybe only apply pickable?
	applyAttributes(m_attribValues);
}

bool iADataSetRenderer::isPickable() const
{
	return m_attribValues[Pickable].toBool();
}

iAAttributes iADataSetRenderer::attributesWithValues() const
{
	iAAttributes result = combineAttributesWithValues(m_attributes, m_attribValues);
	// set position and orientation from current values:
	assert(result[0]->name() == Position);
	auto pos = position();
	result[0]->setDefaultValue(QVariant::fromValue(QVector<double>({pos[0], pos[1], pos[2]})));
	assert(result[1]->name() == Orientation);
	auto ori = orientation();
	result[1]->setDefaultValue(QVariant::fromValue(QVector<double>({ori[0], ori[1], ori[2]})));
	return result;
}

void iADataSetRenderer::setVisible(bool visible)
{
	m_visible = visible;
	if (m_visible)
	{
		showDataSet();
	}
	else
	{
		hideDataSet();
	}
}

bool iADataSetRenderer::isVisible() const
{
	return m_visible;
}

void iADataSetRenderer::setBoundsVisible(bool visible)
{
	if (!m_outline)
	{
		m_outline = std::make_shared<iAOutlineImpl>(bounds(), m_renderer,
			m_attribValues.contains(OutlineColor) ? m_attribValues[OutlineColor].value<QColor>() : OutlineDefaultColor);
		updateOutlineTransform();
	}
	m_outline->setVisible(visible);
}

void iADataSetRenderer::addAttribute(
	QString const& name, iAValueType valueType, QVariant defaultValue, double min, double max)
{
#ifndef _NDEBUG
	for (auto attr : m_attributes)
	{
		if (attr->name() == name)
		{
			LOG(lvlWarn, QString("iADataSetRenderer::addAttribute: Attribute with name %1 already exists!").arg(name));
		}
	}
#endif
	m_attributes.push_back(iAAttributeDescriptor::createParam(name, valueType, defaultValue, min, max));
	m_attribValues[name] = defaultValue;
}

void iADataSetRenderer::updateOutlineTransform()
{
	if (!m_outline)
	{
		return;
	}
	QVector<double> pos(3), ori(3);
	for (int i = 0; i < 3; ++i)
	{
		pos[i] = position()[i];
		ori[i] = orientation()[i];
	}
	m_outline->setOrientationAndPosition(pos, ori);
}

//QWidget* iADataSetRenderer::controlWidget()
//{
//	return nullptr;
//}

// ---------- iAGraphRenderer ----------

#include <vtkGlyph3DMapper.h>

namespace
{
	const QString PointRadius = "Point Radius";
	const QString PointColor = "Point Color";
	const QString LineColor = "Line Color";
	const QString LineWidth = "Line Width";
}

class iAGraphRenderer : public iADataSetRenderer
{
public:
	iAGraphRenderer(vtkRenderer* renderer, iAGraphData* data) :
		iADataSetRenderer(renderer),
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

	void applyAttributes(QMap<QString, QVariant> const& values) override
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
		auto o1 = m_pointActor->GetOrientation(), o2 = m_lineActor->GetOrientation();
		assert(dblApproxEqual(o1[0], o2[0], 1e-6) && dblApproxEqual(o1[1], o2[1], 1e-6) && dblApproxEqual(o1[2], o2[2], 1e-6));
		return m_pointActor->GetOrientation();
	}
	double const* position() const override
	{
		auto p1 = m_pointActor->GetPosition(), p2 = m_lineActor->GetPosition();
		assert(dblApproxEqual(p1[0], p2[0], 1e-6) && dblApproxEqual(p1[1], p2[1], 1e-6) && dblApproxEqual(p1[2], p2[2], 1e-6));
		return m_lineActor->GetPosition();
	}
	void setPosition(double pos[3]) override
	{
		m_lineActor->SetPosition(pos);
		m_pointActor->SetPosition(pos);
	}
	void setOrientation(double ori[3])
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
		iADataSetRenderer(renderer),
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
	}
	void showDataSet() override
	{
		m_renderer->AddActor(m_polyActor);
	}
	void hideDataSet() override
	{
		m_renderer->RemoveActor(m_polyActor);
	}
	void applyAttributes(QMap<QString, QVariant> const& values) override
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
	void setOrientation(double ori[3])
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
	iAPolyDataRenderer(vtkRenderer* renderer, iAPolyData * data):
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
#include "iAModalityTransfer.h"
#include "iAToolsVTK.h"
#include "iAVolumeDataForDisplay.h"

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

class iAVolRenderer: public iADataSetRenderer
{
public:
	iAVolRenderer(vtkRenderer* renderer, iAImageData* data, iAVolumeDataForDisplay* volDataForDisplay) :
		iADataSetRenderer(renderer),
		m_volume(vtkSmartPointer<vtkVolume>::New()),
		m_volProp(vtkSmartPointer<vtkVolumeProperty>::New()),
		m_volMapper(vtkSmartPointer<vtkSmartVolumeMapper>::New()),
		m_image(data)//,
		//m_histogram(new iAChartWithFunctionsWidget(nullptr))
	{
		assert(volDataForDisplay);
		m_volMapper->SetBlendModeToComposite();
		m_volume->SetMapper(m_volMapper);
		m_volume->SetProperty(m_volProp);
		m_volume->SetVisibility(true);
		m_volMapper->SetInputData(data->image());
		if (data->image()->GetNumberOfScalarComponents() > 1)
		{
			m_volMapper->SetBlendModeToComposite();
			m_volProp->IndependentComponentsOff();
		}
		else
		{
			m_volProp->SetColor(0, volDataForDisplay->transfer()->colorTF());
		}
		m_volProp->SetScalarOpacity(0, volDataForDisplay->transfer()->opacityTF());
		m_volProp->Modified();

		// properties specific to volumes:
		auto volumeSettings = iAMainWindow::get()->defaultVolumeSettings();
		addAttribute(LinearInterpolation, iAValueType::Boolean, volumeSettings.LinearInterpolation);
		addAttribute(Shading, iAValueType::Boolean, volumeSettings.Shading);
		addAttribute(ScalarOpacityUnitDistance, iAValueType::Continuous, volumeSettings.ScalarOpacityUnitDistance);

		// mapper properties:
		QStringList renderTypes = RenderModeMap().values();
		selectOption(renderTypes, renderTypes[volumeSettings.RenderMode]);
		addAttribute(RendererType, iAValueType::Categorical, renderTypes);
		addAttribute(InteractiveAdjustSampleDistance, iAValueType::Boolean, false);
		addAttribute(AutoAdjustSampleDistance, iAValueType::Boolean, false);
		addAttribute(SampleDistance, iAValueType::Continuous, volumeSettings.SampleDistance);
		addAttribute(InteractiveUpdateRate, iAValueType::Continuous, 1.0);
		addAttribute(FinalColorLevel, iAValueType::Continuous, 0.5);
		addAttribute(FinalColorWindow, iAValueType::Continuous, 1.0);
		// -> VTK 9.2 ?
		//addAttribute(GlobalIlluminationReach, iAValueType::Continuous, 0.0, 0.0, 1.0);
		//addAttribute(VolumetricScatteringBlending, iAValueType::Continuous, -1.0, 0.0, 2.0);

		// volume properties:
		auto spc = data->image()->GetSpacing();
		QVector<double> spacing({spc[0], spc[1], spc[2]});
		addAttribute(Spacing, iAValueType::Vector3, QVariant::fromValue(spacing));

		applyAttributes(m_attribValues);  // addAttribute adds default values; apply them now!

		// adapt bounding box to changes in position/orientation of volume:
		vtkNew<vtkCallbackCommand> modifiedCallback;
		modifiedCallback->SetCallback(
			[](vtkObject* vtkNotUsed(caller), long unsigned int vtkNotUsed(eventId), void* clientData,
				void* vtkNotUsed(callData))
			{
				reinterpret_cast<iAVolRenderer*>(clientData)->updateOutlineTransform();
			});
		modifiedCallback->SetClientData(this);
		m_volume->AddObserver(vtkCommand::ModifiedEvent, modifiedCallback);

		//m_histogram->setTransferFunction(m_transfer.get());
	}
	void showDataSet() override
	{
		m_renderer->AddVolume(m_volume);
	}
	void hideDataSet() override
	{
		m_renderer->RemoveVolume(m_volume);
	}
	void applyAttributes(QMap<QString, QVariant> const& values) override
	{
		applyLightingProperties(m_volProp.Get(), values);
		m_volProp->SetInterpolationType(values[LinearInterpolation].toInt());
		m_volProp->SetShade(values[Shading].toBool());
		if (values[ScalarOpacityUnitDistance].toDouble() > 0)
		{
			m_volProp->SetScalarOpacityUnitDistance(values[ScalarOpacityUnitDistance].toDouble());
		}
		/*
		else
		{
			m_volSettings.ScalarOpacityUnitDistance = m_volProp->GetScalarOpacityUnitDistance();
		}
		*/
		m_volMapper->SetRequestedRenderMode(values[RendererType].toInt());
		m_volMapper->SetInteractiveAdjustSampleDistances(values[InteractiveAdjustSampleDistance].toBool());
		m_volMapper->SetAutoAdjustSampleDistances(values[AutoAdjustSampleDistance].toBool());
		m_volMapper->SetSampleDistance(values[SampleDistance].toDouble());
		m_volMapper->SetInteractiveUpdateRate(values[InteractiveUpdateRate].toDouble());
		m_volMapper->SetFinalColorLevel(values[FinalColorLevel].toDouble());
		m_volMapper->SetFinalColorWindow(values[FinalColorWindow].toDouble());
		// VTK 9.2:
		//m_volMapper->SetGlobalIlluminationReach
		//m_volMapper->SetVolumetricScatteringBlending

		QVector<double> pos = values[Position].value<QVector<double>>();
		QVector<double> ori = values[Orientation].value<QVector<double>>();
		assert(pos.size() == 3);
		assert(ori.size() == 3);
		m_volume->SetPosition(pos.data());
		m_volume->SetOrientation(ori.data());
		m_volume->SetPickable(values[Pickable].toBool());
		
		QVector<double> spc = values[Spacing].value<QVector<double>>();
		assert(spc.size() == 3);
		m_image->image()->SetSpacing(spc.data());
	}
	/*
	void setMovable(bool movable) override
	{
		m_volume->SetPickable(movable);
		m_volume->SetDragable(movable);
	}
	*/
	//QWidget* controlWidget() override
	//{
	//
	//}
	iAAABB bounds() override
	{
		return iAAABB(m_image->image()->GetBounds());
	}
	double const* orientation() const override
	{
		return m_volume->GetOrientation();
	}
	double const* position() const override
	{
		return m_volume->GetPosition();
	}
	void setPosition(double pos[3]) override
	{
		m_volume->SetPosition(pos);
	}
	void setOrientation(double ori[3])
	{
		m_volume->SetOrientation(ori);
	}
	vtkProp3D* vtkProp() override
	{
		return m_volume;
	}

private:
	//iAVolumeSettings m_volSettings;

	vtkSmartPointer<vtkVolume> m_volume;
	vtkSmartPointer<vtkVolumeProperty> m_volProp;
	vtkSmartPointer<vtkSmartVolumeMapper> m_volMapper;
	iAImageData* m_image;
	//iAChartWithFunctionsWidget* m_histogram;
};

// ---------- Factory method ----------

std::shared_ptr<iADataSetRenderer> createDataRenderer(iADataSet* dataSet, iADataForDisplay* dataForDisplay, vtkRenderer* renderer)
{
	auto img = dynamic_cast<iAImageData*>(dataSet);
	if (img)
	{
		auto volDataForDisplay = dynamic_cast<iAVolumeDataForDisplay*>(dataForDisplay);
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
