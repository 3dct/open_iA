#include "iADataSetRenderer.h"

#include "iADataSet.h"
#include "iARenderer.h"

#include <vtkActor.h>
#include <vtkGlyph3DMapper.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkOpenGLRenderer.h>
#include <vtkSphereSource.h>


iADataSetRenderer::iADataSetRenderer(iARenderer* renderer):
	m_renderer(renderer)
{}

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
		// TODO: move somewhere else, fix memory leaks...
		auto sphere = vtkSphereSource::New();
		sphere->SetPhiResolution(21);
		sphere->SetThetaResolution(21);
		sphere->SetRadius(5);
		vtkNew<vtkPoints> pointsPoints;
		pointsPoints->DeepCopy(data->poly()->GetPoints());
		vtkNew<vtkPolyData> glyphPoints;
		glyphPoints->SetPoints(pointsPoints);
		auto pointMapper = vtkGlyph3DMapper::New();
		pointMapper->SetInputData(glyphPoints);
		pointMapper->SetSourceConnection(sphere->GetOutputPort());
		m_pointActor->SetMapper(pointMapper);
		m_pointActor->GetProperty()->SetColor(1.0, 0.0, 0.0);
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

private:
	vtkSmartPointer<vtkActor> m_lineActor, m_pointActor;
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