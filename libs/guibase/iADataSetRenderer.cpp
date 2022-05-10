#include "iADataSetRenderer.h"

#include "iADataSet.h"
#include "iARenderer.h"

#include <vtkActor.h>
#include <vtkGlyph3DMapper.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkOpenGLRenderer.h>
#include <vtkSphereSource.h>

#include <QColor>

namespace
{
	QString PointRadius = "Point Radius";
	QString PointColor = "Point Color";
	QString LineColor = "Line Color";
	QString LineWidth = "Line Width";
}

iADataSetRenderer::iADataSetRenderer(iARenderer* renderer):
	m_renderer(renderer)
{}

iAAttributes const& iADataSetRenderer::attributes() const
{
	return m_attributes;
}

void iADataSetRenderer::setAttributes(QMap<QString, QVariant> values)
{
}

void iADataSetRenderer::addAttribute(
	QString const& name, iAValueType valueType, QVariant defaultValue, double min, double max)
{
	m_attributes.push_back(iAAttributeDescriptor::createParam(name, valueType, defaultValue, min, max));
}

class iAGraphRenderer : public iADataSetRenderer
{
public:
	iAGraphRenderer(iARenderer* renderer, iAGraphData* data):
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
	void show() override
	{
		m_renderer->renderer()->AddActor(m_lineActor);
		m_renderer->renderer()->AddActor(m_pointActor);
	}
	void hide() override
	{
		m_renderer->renderer()->RemoveActor(m_pointActor);
		m_renderer->renderer()->RemoveActor(m_lineActor);
	}

	void setAttributes(QMap<QString, QVariant> values) override
	{
		m_sphereSource->SetRadius(values[PointRadius].toDouble());
		QColor pointColor(values[PointColor].toString());
		m_pointActor->GetProperty()->SetColor(pointColor.redF(), pointColor.greenF(), pointColor.blueF());
		m_sphereSource->Update();
		QColor lineColor(values[LineColor].toString());
		m_lineActor->GetProperty()->SetColor(lineColor.redF(), lineColor.greenF(), lineColor.blueF());
		m_lineActor->GetProperty()->SetLineWidth(values[LineWidth].toFloat());
	}

private:
	vtkSmartPointer<vtkActor> m_lineActor, m_pointActor;
	vtkSmartPointer<vtkSphereSource> m_sphereSource;
};


std::shared_ptr<iADataSetRenderer> createDataRenderer(iADataSet* dataset, iARenderer* renderer)
{
	switch(dataset->type())
	{
	case iADataSetType::dstGraph:
		return std::make_shared<iAGraphRenderer>(renderer, dynamic_cast<iAGraphData*>(dataset));
	default:
		LOG(lvlWarn, QString("Requested renderer for unknown type %1!")
			.arg(dataset->type()));
		return {};
	}
}