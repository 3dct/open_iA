// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iADataSetRendererImpl.h"

#include "iADataSet.h"
#include "iAMainWindow.h"
#include "iAValueTypeVectorHelpers.h"

#include "iAAABB.h"
#ifndef NDEBUG
#include "iAMathUtility.h"    // for dblApproxEqual
#endif

// ---------- iAGraphRenderer ----------

#include <vtkActor.h>
#include <vtkCallbackCommand.h>
#include <vtkGlyph3DMapper.h>
#include <vtkPolyDataMapper.h>
#include <vtkPointData.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkSphereSource.h>
#include <vtkTubeFilter.h>

#include <QColor>
#include <QVector>

namespace
{
	// graph renderer options:
	constexpr const char* PointRadiusVaryBy = "Vary Point Radius by";
	constexpr const char* VaryModeFixed = "Fixed";
	constexpr const char* StoredColors = "Stored Colors";
	constexpr const char* PointRadius = "Minimum Point Radius";
	constexpr const char* PointColorMode = "Point Colors";
	constexpr const char* PointColor = "Fixed Point Color";
	constexpr const char* PointPrefix = "Point ";
	constexpr const char* LineColorMode = "Line Colors";
	constexpr const char* LineColor = "Fixed Line Color";
	constexpr const char* LineWidth = "Minimum Line Width";
	constexpr const char* LineWidthVaryBy = "Vary Line Width by";
	constexpr const char* LinePrefix = "Line ";
	constexpr const char* ShadingInterpolation = "Shading Interpolation";
	constexpr const char* InterpolationFlat = "Flat";
	constexpr const char* InterpolationGouraud = "Gouraud";
	constexpr const char* InterpolationPhong = "Phong";
	constexpr const char* InterpolationPBR = "Physically Based Rendering";
	// surface renderer options:
	constexpr const char* PolyColor = "Color";
	constexpr const char* PolyOpacity = "Opacity";
	constexpr const char* PolyWireframe = "Wireframe";


	int string2VtkShadingInterpolation(QString const & type)
	{
		if (type == InterpolationFlat) { return VTK_FLAT; }
		else if (type == InterpolationGouraud) { return VTK_GOURAUD; }
		else if (type == InterpolationPBR) { return VTK_PBR; }
		else /* if (type == InterpolationPhong) */ { return VTK_PHONG; }
	}

	QStringList const & shadingInterpolationTypes()
	{
		static QStringList types = QStringList() << InterpolationFlat << InterpolationGouraud << InterpolationPhong << InterpolationPBR;
		return types;
	}

	void applyActorProperties(vtkActor* actor, QVariantMap const& values, QString const& prefix = "")
	{
		applyLightingProperties(actor->GetProperty(), values, prefix);
		auto pos = variantToVector<double>(values[iADataSetRenderer::Position]);
		assert(pos.size() == 3);
		if (pos.size() == 3)
		{
			actor->SetPosition(pos.data());
		}
		auto ori = variantToVector<double>(values[iADataSetRenderer::Orientation]);
		assert(ori.size() == 3);
		if (ori.size() == 3)
		{
			actor->SetOrientation(ori.data());
		}
		actor->GetProperty()->SetInterpolation(string2VtkShadingInterpolation(values[prefix + ShadingInterpolation].toString()));
		actor->GetProperty()->SetShading(values[prefix + iADataSetRenderer::Shading].toBool());
	}
}

constexpr const char GraphRendererName[] = "Default Settings/Dataset Renderer: Graph";

//! Encapsulates the specifics of the settings of a graph renderer.
//! Handles auto-registration of the settings with iASettingsManager (via deriving from iASettingsObject),
//! and thus avoids having to expose users of iAGraphRenderer to the settings auto-registration.
class iAguibase_API iAGraphRendererSettings : iASettingsObject<GraphRendererName, iAGraphRendererSettings>
{
public:
	static iAAttributes& defaultAttributes()
	{
		static iAAttributes attr;
		if (attr.isEmpty())
		{
			attr = cloneAttributes(iADataSetRenderer::defaultAttributes());
			addAttr(attr, PointRadius, iAValueType::Continuous, 5, 0.0000001, 100000000);
			addAttr(attr, PointColorMode, iAValueType::Categorical, QStringList() << VaryModeFixed << StoredColors);
			addAttr(attr, PointColor, iAValueType::Color, "#FF0000");
			addAttr(attr, QString(PointPrefix) + iADataSetRenderer::Shading, iAValueType::Boolean, false);
			addAttr(attr, QString(PointPrefix) + ShadingInterpolation, iAValueType::Categorical, shadingInterpolationTypes());
			addAttr(attr, QString(PointPrefix) + iADataSetRenderer::AmbientLighting, iAValueType::Continuous, 0.2);
			addAttr(attr, QString(PointPrefix) + iADataSetRenderer::DiffuseLighting, iAValueType::Continuous, 0.5);
			addAttr(attr, QString(PointPrefix) + iADataSetRenderer::SpecularLighting, iAValueType::Continuous, 0.7);
			addAttr(attr, QString(PointPrefix) + iADataSetRenderer::SpecularPower, iAValueType::Continuous, 10.0);
			addAttr(attr, LineWidth, iAValueType::Continuous, 1.0, 0.1, 100);
			addAttr(attr, LineColorMode, iAValueType::Categorical, QStringList() << VaryModeFixed << StoredColors);
			addAttr(attr, LineColor, iAValueType::Color, "#00FF00");
			addAttr(attr, QString(LinePrefix) + iADataSetRenderer::Shading, iAValueType::Boolean, false);
			addAttr(attr, QString(LinePrefix) + ShadingInterpolation, iAValueType::Categorical, shadingInterpolationTypes());
			addAttr(attr, QString(LinePrefix) + iADataSetRenderer::AmbientLighting, iAValueType::Continuous, 0.2);
			addAttr(attr, QString(LinePrefix) + iADataSetRenderer::DiffuseLighting, iAValueType::Continuous, 0.5);
			addAttr(attr, QString(LinePrefix) + iADataSetRenderer::SpecularLighting, iAValueType::Continuous, 0.7);
			addAttr(attr, QString(LinePrefix) + iADataSetRenderer::SpecularPower, iAValueType::Continuous, 10.0);
		}
		return attr;
	}
};

iAGraphRenderer::iAGraphRenderer(vtkRenderer* renderer, iAGraphData const * data, QVariantMap const& overrideValues) :
	iADataSetRenderer(renderer),
	m_lineActor(vtkSmartPointer<vtkActor>::New()),
	m_pointActor(vtkSmartPointer<vtkActor>::New()),
	m_tubeFilter(vtkSmartPointer<vtkTubeFilter>::New()),
	m_lineMapper(vtkSmartPointer<vtkPolyDataMapper>::New()),
	m_data(data)
{
	m_tubeFilter->SidesShareVerticesOff();
	m_tubeFilter->SetInputData(data->poly());
	m_lineMapper->SetInputConnection(m_tubeFilter->GetOutputPort());
	m_lineMapper->SetScalarModeToUseCellData();
	m_lineActor->SetMapper(m_lineMapper);
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
	glyphPoints->GetPointData()->ShallowCopy(data->poly()->GetPointData());
	m_glyphMapper = vtkGlyph3DMapper::New();
	m_glyphMapper->SetInputData(glyphPoints);
	m_glyphMapper->SetSourceConnection(m_sphereSource->GetOutputPort());
	m_pointActor->SetMapper(m_glyphMapper);
	m_pointActor->SetPickable(false);
	m_pointActor->GetProperty()->SetColor(1.0, 0.0, 0.0);

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
	setDefaultAttributes(defaultAttributes(), overrideValues);
}

iAAttributes const& iAGraphRenderer::attributes() const
{
	static iAAttributes attr;
	if (attr.isEmpty())
	{
		attr = cloneAttributes(iAGraphRenderer::defaultAttributes());
		addAttr(attr, PointRadiusVaryBy, iAValueType::Categorical,
			QStringList() << (QString("!") + VaryModeFixed) << m_data->vertexValueNames());
		addAttr(attr, LineWidthVaryBy, iAValueType::Categorical,
			QStringList() << (QString("!") + VaryModeFixed) << m_data->edgeValueNames());
	}
	return attr;
}

iAAttributes& iAGraphRenderer::defaultAttributes()
{
	static iAAttributes& attr = iAGraphRendererSettings::defaultAttributes();
	return attr;
}

iAGraphRenderer::~iAGraphRenderer()
{
	if (isVisible())
	{
		hideDataSet();
	}
}

void iAGraphRenderer::showDataSet()
{
	m_renderer->AddActor(m_lineActor);
	m_renderer->AddActor(m_pointActor);
}

void iAGraphRenderer::hideDataSet()
{
	if (!m_renderer)
	{
		return;
	}
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
	m_glyphMapper->SetScaleMode(values[PointRadiusVaryBy].toString() == VaryModeFixed
								? vtkGlyph3DMapper::NO_DATA_SCALING
								: vtkGlyph3DMapper::SCALE_BY_MAGNITUDE);
	if (values[PointRadiusVaryBy].toString() != VaryModeFixed)
	{
		m_glyphMapper->SetScaleArray(values[PointRadiusVaryBy].toString().toStdString().c_str());
	}
	if (values[PointColorMode].toString() == VaryModeFixed)
	{
		QColor pointColor = variantToColor(values[PointColor]);
		m_pointActor->GetProperty()->SetColor(pointColor.redF(), pointColor.greenF(), pointColor.blueF());
	}
	else
	{
		m_glyphMapper->SelectColorArray("VertexColors");
	}
	m_sphereSource->Update();
	m_glyphMapper->SetColorMode((values[PointColorMode].toString() == VaryModeFixed)
		? VTK_COLOR_MODE_DEFAULT
		: VTK_COLOR_BY_SCALAR);

	m_lineMapper->SetScalarMode(values[LineColorMode].toString() == VaryModeFixed
		? VTK_SCALAR_MODE_USE_CELL_DATA
		: VTK_SCALAR_MODE_DEFAULT);
	if (values[LineColorMode].toString() == VaryModeFixed)
	{
		QColor lineColor = variantToColor(values[LineColor]);
		m_lineActor->GetProperty()->SetColor(lineColor.redF(), lineColor.greenF(), lineColor.blueF());
	}
	else
	{
		m_lineMapper->SelectColorArray("EdgeColors");
	}
	//m_lineActor->GetProperty()->SetLineWidth(values[LineWidth].toFloat());
	
	if (values[LineWidthVaryBy].toString() != VaryModeFixed)
	{
		m_data->poly()->GetPointData()->SetActiveScalars(values[LineWidthVaryBy].toString().toStdString().c_str());
	}
	m_tubeFilter->SetVaryRadius(values[LineWidthVaryBy].toString() == VaryModeFixed
		? VTK_VARY_RADIUS_OFF
		: VTK_VARY_RADIUS_BY_SCALAR);
	m_tubeFilter->SetRadius(values[LineWidth].toDouble());
	
	applyActorProperties(m_pointActor, values, PointPrefix);
	applyActorProperties(m_lineActor, values, LinePrefix);
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

//! Encapsulates the specifics of the settings of a surface mesh renderer.
//! Handles auto-registration of the settings with iASettingsManager (via deriving from iASettingsObject),
//! and thus avoids having to expose users of iAPolyDataRenderer/iAGeometricObjectRenderer to the settings auto-registration.
constexpr const char SurfaceRendererName[] = "Default Settings/Dataset Renderer: Surface";
class iAguibase_API iAPolyActorRendererSettings : iASettingsObject<SurfaceRendererName, iAPolyActorRendererSettings>
{
public:
	static iAAttributes& defaultAttributes()
	{
		static iAAttributes attr;
		if (attr.isEmpty())
		{
			attr = cloneAttributes(iADataSetRenderer::defaultAttributes());
			addAttr(attr, iAPolyActorRenderer::Shading, iAValueType::Boolean, true);
			addAttr(attr, ShadingInterpolation, iAValueType::Categorical, shadingInterpolationTypes());
			addAttr(attr, PolyColor, iAValueType::Color, "#FFFFFF");
			addAttr(attr, PolyOpacity, iAValueType::Continuous, 1.0, 0.0, 1.0);
			addAttr(attr, PolyWireframe, iAValueType::Boolean, false);
		}
		return attr;
	}
};

iAPolyActorRenderer::iAPolyActorRenderer(vtkRenderer* renderer, QVariantMap const & overrideValues) :
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
	setDefaultAttributes(defaultAttributes(), overrideValues);
}

iAAttributes const& iAPolyActorRenderer::attributes() const
{
	return defaultAttributes();
}

iAAttributes& iAPolyActorRenderer::defaultAttributes()
{
	static iAAttributes attr = iAPolyActorRendererSettings::defaultAttributes();
	return attr;
}

iAPolyActorRenderer::~iAPolyActorRenderer()
{
	if (isVisible())
	{
		hideDataSet();
	}
}

void iAPolyActorRenderer::showDataSet()
{
	m_renderer->AddActor(m_polyActor);
}

void iAPolyActorRenderer::hideDataSet()
{
	if (!m_renderer)
	{
		return;
	}
	m_renderer->RemoveActor(m_polyActor);
}

void iAPolyActorRenderer::applyAttributes(QVariantMap const& values)
{
	applyActorProperties(m_polyActor, values);

	QColor color = variantToColor(values[PolyColor]);
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

vtkPolyDataMapper* iAPolyActorRenderer::mapper()
{
	return dynamic_cast<vtkPolyDataMapper*>(m_polyActor->GetMapper());
}

vtkActor* iAPolyActorRenderer::actor()
{
	return m_polyActor;
}

iAPolyDataRenderer::iAPolyDataRenderer(vtkRenderer* renderer, iAPolyData const * data, QVariantMap const& overrideValues) :
	iAPolyActorRenderer(renderer, overrideValues),
	m_data(data)
{
	mapper()->SetInputData(data->poly());
	//m_polyMapper->SelectColorArray("Colors");
}

iAAABB iAPolyDataRenderer::bounds()
{
	return iAAABB(m_data->poly()->GetBounds());
}


#include "iAGeometricObject.h"

iAGeometricObjectRenderer::iAGeometricObjectRenderer(vtkRenderer* renderer, iAGeometricObject const * data, QVariantMap const& overrideValues) :
	iAPolyActorRenderer(renderer, overrideValues),
	m_data(data)
{
	mapper()->SetInputConnection(m_data->source()->GetOutputPort());
	//m_polyMapper->SelectColorArray("Colors");
}
iAAABB iAGeometricObjectRenderer::bounds()
{
	return iAAABB(m_data->bounds());
}
