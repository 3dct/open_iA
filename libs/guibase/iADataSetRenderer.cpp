#include "iADataSetRenderer.h"

#include "iAAABB.h"
#include "iADataSet.h"
#include "iARenderer.h"

#include <vtkActor.h>
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
	iAOutlineImpl(iAAABB const& box, iARenderer* renderer): m_renderer(renderer)
	{
		m_cubeSource->SetBounds(
			box.topLeft().x(), box.bottomRight().x(),
			box.topLeft().y(), box.bottomRight().y(),
			box.topLeft().z(), box.bottomRight().z()
		);
		m_mapper->SetInputConnection(m_cubeSource->GetOutputPort());
		m_actor->GetProperty()->SetColor(0, 0, 0);
		m_actor->GetProperty()->SetRepresentationToWireframe();
		m_actor->PickableOff();
		m_actor->SetMapper(m_mapper);
		renderer->renderer()->AddActor(m_actor);
	}

	void setVisible(bool visible)
	{
		if (visible)
		{
			m_renderer->renderer()->AddActor(m_actor);
		}
		else
		{
			m_renderer->renderer()->RemoveActor(m_actor);
		}
	}

private:
	vtkNew<vtkCubeSource> m_cubeSource;
	vtkNew<vtkPolyDataMapper> m_mapper;
	vtkNew<vtkActor> m_actor;
	iARenderer* m_renderer;
};


iADataSetRenderer::iADataSetRenderer(iARenderer* renderer):
	m_renderer(renderer),
	m_visible(false)
{}

iAAttributes const& iADataSetRenderer::attributes() const
{
	return m_attributes;
}

void iADataSetRenderer::setAttributes(QMap<QString, QVariant> const & values)
{
	m_attribValues = values;
	applyAttributes();
}

QMap<QString, QVariant> const& iADataSetRenderer::attributeValues() const
{
	return m_attribValues;
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
		m_outline = std::make_shared<iAOutlineImpl>(bounds(), m_renderer);
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
}

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
	iAGraphRenderer(iARenderer* renderer, iAGraphData* data) :
		iADataSetRenderer(renderer),
		m_lineActor(vtkSmartPointer<vtkActor>::New()),
		m_pointActor(vtkSmartPointer<vtkActor>::New())
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
		m_pointActor->GetProperty()->SetColor(1.0, 0.0, 0.0);

		addAttribute(PointRadius, iAValueType::Continuous, 5, 0.001, 100000000);
		addAttribute(PointColor, iAValueType::Color, "#FF0000");
		addAttribute(LineColor, iAValueType::Color, "#00FF00");
		addAttribute(LineWidth, iAValueType::Continuous, 1.0, 0.1, 100);
	}
	void showDataSet() override
	{
		m_renderer->renderer()->AddActor(m_lineActor);
		m_renderer->renderer()->AddActor(m_pointActor);
	}
	void hideDataSet() override
	{
		m_renderer->renderer()->RemoveActor(m_pointActor);
		m_renderer->renderer()->RemoveActor(m_lineActor);
	}

	void applyAttributes() override
	{
		m_sphereSource->SetRadius(attributeValues()[PointRadius].toDouble());
		QColor pointColor(attributeValues()[PointColor].toString());
		m_pointActor->GetProperty()->SetColor(pointColor.redF(), pointColor.greenF(), pointColor.blueF());
		m_sphereSource->Update();
		QColor lineColor(attributeValues()[LineColor].toString());
		m_lineActor->GetProperty()->SetColor(lineColor.redF(), lineColor.greenF(), lineColor.blueF());
		m_lineActor->GetProperty()->SetLineWidth(attributeValues()[LineWidth].toFloat());
	}

private:
	iAAABB bounds() override
	{
		iAAABB result(m_pointActor->GetBounds());
		result.merge(iAAABB(m_lineActor->GetBounds()));
		return result;
	}
	vtkSmartPointer<vtkActor> m_lineActor, m_pointActor;
	vtkSmartPointer<vtkSphereSource> m_sphereSource;
};



// ---------- iAMeshRenderer ----------

namespace
{
	const QString PolyColor = "Color";
	const QString PolyOpacity = "Opacity";
}

class iAMeshRenderer: public iADataSetRenderer
{
public:
	iAMeshRenderer(iARenderer* renderer, iAPolyData * data):
		iADataSetRenderer(renderer),
		m_polyActor(vtkSmartPointer<vtkActor>::New())
	{
		vtkNew<vtkPolyDataMapper> mapper;
		mapper->SetInputData(data->poly());
		//m_polyMapper->SelectColorArray("Colors");
		mapper->SetScalarModeToUsePointFieldData();
		m_polyActor->SetPickable(false);
		m_polyActor->SetDragable(false);
		m_polyActor->SetMapper(mapper);

		addAttribute(PolyColor, iAValueType::Color, "#FFFFFF");
		addAttribute(PolyOpacity, iAValueType::Continuous, 1.0, 0.0, 1.0);
	}
	void showDataSet() override
	{
		m_renderer->renderer()->AddActor(m_polyActor);
	}
	void hideDataSet() override
	{
		m_renderer->renderer()->RemoveActor(m_polyActor);
	}
	void applyAttributes() override
	{
		QColor color(attributeValues()[PolyColor].toString());
		double opacity = attributeValues()[PolyOpacity].toDouble();
		m_polyActor->GetProperty()->SetColor(color.redF(), color.greenF(), color.blueF());
		m_polyActor->GetProperty()->SetOpacity(opacity);
	}
	iAAABB bounds() override
	{
		return iAAABB(m_polyActor->GetBounds());
	}

private:
	vtkSmartPointer<vtkActor> m_polyActor;
};

// ---------- iAVolRenderer ----------
#include "iAModalityTransfer.h"
#include "iAToolsVTK.h"
#include "iAVolumeSettings.h"

#include <vtkImageData.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>
#include <vtkSmartVolumeMapper.h>

namespace
{
	const QString LinearInterpolation = "Linear interpolation";
	const QString Shading = "Shading";
	const QString SampleDistance = "Sample distance";
	const QString AmbientLighting = "Ambient lighting";
	const QString DiffuseLighting= "Diffuse lighting";
	const QString SpecularLighting = "Specular lighting";
	const QString SpecularPower = "Specular power";
	const QString ScalarOpacityUnitDistance = "Scalar Opacity Unit Distance";
	const QString RendererType = "Renderer type";
}

class iAVolRenderer: public iADataSetRenderer
{
public:
	iAVolRenderer(iARenderer* renderer, iAImageData* data) :
		iADataSetRenderer(renderer),
		m_volume(vtkSmartPointer<vtkVolume>::New()),
		m_volProp(vtkSmartPointer<vtkVolumeProperty>::New()),
		m_volMapper(vtkSmartPointer<vtkSmartVolumeMapper>::New()),
		m_transfer(std::make_shared<iAModalityTransfer>(data->image()->GetScalarRange()))
	{
		m_volMapper->SetBlendModeToComposite();
		m_volume->SetMapper(m_volMapper);
		m_volume->SetProperty(m_volProp);
		m_volume->SetVisibility(true);
		m_volMapper->SetInputData(data->image());
		if (data->image()->GetNumberOfScalarComponents() > 1)
		{
			m_volMapper->SetBlendModeToComposite();
			m_volProp->IndependentComponentsOff();
			m_volProp->SetScalarOpacity(0, m_transfer->opacityTF());
		}
		else
		{
			//if (m_volSettings.ScalarOpacityUnitDistance < 0)
			{
				//m_volSettings.ScalarOpacityUnitDistance = data->image()->GetSpacing()[0];
				m_volProp->SetScalarOpacityUnitDistance(data->image()->GetSpacing()[0]);
			}
			m_volProp->SetColor(0, m_transfer->colorTF());
			m_volProp->SetScalarOpacity(0, m_transfer->opacityTF());
		}
		m_volProp->Modified();

		iAVolumeSettings volumeSettings;
		addAttribute(LinearInterpolation, iAValueType::Boolean, volumeSettings.LinearInterpolation);
		addAttribute(Shading, iAValueType::Boolean, volumeSettings.Shading);
		addAttribute(SampleDistance, iAValueType::Continuous, volumeSettings.SampleDistance);
		addAttribute(AmbientLighting, iAValueType::Continuous, volumeSettings.AmbientLighting);
		addAttribute(DiffuseLighting, iAValueType::Continuous, volumeSettings.DiffuseLighting);
		addAttribute(SpecularLighting, iAValueType::Continuous, volumeSettings.SpecularLighting);
		addAttribute(SpecularPower, iAValueType::Continuous, volumeSettings.SpecularPower);
		addAttribute(ScalarOpacityUnitDistance, iAValueType::Continuous, volumeSettings.ScalarOpacityUnitDistance);
		QStringList renderTypes = RenderModeMap().values();
		selectOption(renderTypes, renderTypes[volumeSettings.RenderMode]);
		addAttribute(RendererType, iAValueType::Categorical, renderTypes);
	}
	void showDataSet() override
	{
		m_renderer->renderer()->AddVolume(m_volume);
	}
	void hideDataSet() override
	{
		m_renderer->renderer()->RemoveVolume(m_volume);
	}
	void applyAttributes() override
	{
		m_volProp->SetAmbient(attributeValues()[AmbientLighting].toDouble());
		m_volProp->SetDiffuse(attributeValues()[DiffuseLighting].toDouble());
		m_volProp->SetSpecular(attributeValues()[SpecularLighting].toDouble());
		m_volProp->SetSpecularPower(attributeValues()[SpecularPower].toDouble());
		m_volProp->SetInterpolationType(attributeValues()[LinearInterpolation].toInt());
		m_volProp->SetShade(attributeValues()[Shading].toBool());
		if (attributeValues()[ScalarOpacityUnitDistance].toDouble() > 0)
		{
			m_volProp->SetScalarOpacityUnitDistance(attributeValues()[ScalarOpacityUnitDistance].toDouble());
		}
		/*
		else
		{
			m_volSettings.ScalarOpacityUnitDistance = m_volProp->GetScalarOpacityUnitDistance();
		}
		*/
		m_volMapper->SetRequestedRenderMode(attributeValues()[RendererType].toInt());
		m_volMapper->SetSampleDistance(attributeValues()[SampleDistance].toDouble());
		
		// maybe provide as option: (see also AutoAdjustSampleDistances, InteractiveUpdateRate)
		m_volMapper->InteractiveAdjustSampleDistancesOff();
	}
	/*
	void setMovable(bool movable) override
	{
		m_volume->SetPickable(movable);
		m_volume->SetDragable(movable);
	}
	*/
	iAAABB bounds() override
	{
		return iAAABB(m_volume->GetBounds());
	}
	

private:
	//iAVolumeSettings m_volSettings;
	std::shared_ptr<iAModalityTransfer> m_transfer;

	vtkSmartPointer<vtkVolume> m_volume;
	vtkSmartPointer<vtkVolumeProperty> m_volProp;
	vtkSmartPointer<vtkSmartVolumeMapper> m_volMapper;
};

// ---------- Factory method ----------

std::shared_ptr<iADataSetRenderer> createDataRenderer(iADataSet* dataset, iARenderer* renderer)
{
	switch(dataset->type())
	{
	case iADataSetType::dstVolume:
		return std::make_shared<iAVolRenderer>(renderer, dynamic_cast<iAImageData*>(dataset));
	case iADataSetType::dstGraph:
		return std::make_shared<iAGraphRenderer>(renderer, dynamic_cast<iAGraphData*>(dataset));
	case iADataSetType::dstMesh:
		return std::make_shared<iAMeshRenderer>(renderer, dynamic_cast<iAPolyData*>(dataset));

	default:
		LOG(lvlWarn, QString("Requested renderer for unknown type %1!")
			.arg(dataset->type()));
		return {};
	}
}