/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
#include "iARenderer.h"

#include "defines.h"
#include "iAConsole.h"
#include "iAChannelData.h"
#include "iAMovieHelper.h"
#include "iARenderObserver.h"
#include "iARenderSettings.h"
#include "iASlicerMode.h"
#include "mdichild.h"

#include <vtkActor.h>
#include <vtkAnnotatedCubeActor.h>
#include <vtkAppendFilter.h>
#include <vtkAreaPicker.h>
#include <vtkAxesActor.h>
#include <vtkCallbackCommand.h>
#include <vtkCamera.h>
#include <vtkCellArray.h>
#include <vtkCellLocator.h>
#include <vtkCubeSource.h>
#include <vtkDataSetMapper.h>
#include <vtkExtractSelectedFrustum.h>
#include <vtkGenericMovieWriter.h>
#include <vtkGenericRenderWindowInteractor.h>
#include <vtkImageData.h>
#include <vtkImageCast.h>
#include <vtkInteractorStyleRubberBandPick.h>
#include <vtkInteractorStyleSwitch.h>
#include <vtkLineSource.h>
#include <vtkLogoRepresentation.h>
#include <vtkLogoWidget.h>
#include <vtkObjectFactory.h>
#include <vtkOpenGLRenderer.h>
#include <vtkOrientationMarkerWidget.h>
#include <vtkPicker.h>
#include <vtkPlane.h>
#include <vtkPlaneSource.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkQImageToImageSource.h>
#include <vtkRendererCollection.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSphereSource.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>
#include <vtkTransform.h>
#include <vtkUnstructuredGrid.h>
#include <vtkVersion.h>
#include <vtkWindowToImageFilter.h>

#include <QApplication>
#include <QColor>
#include <QDateTime>
#include <QImage>
#include <QLocale>
#include <QApplication>
#include <QImage>

#define VTKISRBP_ORIENT 0
#define VTKISRBP_SELECT 1

static void GetCellCenter(vtkUnstructuredGrid* imageData, const unsigned int cellId,
	double center[3], double spacing[3]);

class MouseInteractorStyle : public vtkInteractorStyleRubberBandPick
{
public:
	static MouseInteractorStyle* New();
	
	virtual void OnChar()
	{
		switch (this->Interactor->GetKeyCode())
		{
		case 's':
		case 'S':
		{
			if (this->CurrentMode == VTKISRBP_ORIENT)
			{
				this->CurrentMode = VTKISRBP_SELECT;
			}
			else
			{
				this->CurrentMode = VTKISRBP_ORIENT;
			}
			break;
		}
		case 'p':
		case 'P':
		{
			vtkRenderWindowInteractor *rwi = this->Interactor;
			int *eventPos = rwi->GetEventPosition();
			FindPokedRenderer(eventPos[0], eventPos[1]);
			StartPosition[0] = eventPos[0];
			StartPosition[1] = eventPos[1];
			EndPosition[0] = eventPos[0];
			EndPosition[1] = eventPos[1];
			Pick();
			break;
		}
		default:
			Superclass::OnChar();
		}
	}
};
vtkStandardNewMacro(MouseInteractorStyle);

void KeyPressCallbackFunction(vtkObject* caller, long unsigned int vtkNotUsed(eventId),
	void* clientData, void* vtkNotUsed(callData))
{
	iARenderer *ren = static_cast<iARenderer*>(clientData);
	if( ren->interactor()->GetKeyCode() == 's' ||
		ren->interactor()->GetKeyCode() == 'S')
	{
		ren->txtActor()->SetVisibility(!ren->txtActor()->GetVisibility());
		ren->update();
	}
}

void PickCallbackFunction(vtkObject* caller, long unsigned int vtkNotUsed(eventId),
	void* clientData, void* vtkNotUsed(callData))
{
	vtkAreaPicker *areaPicker = static_cast<vtkAreaPicker*>(caller);
	iARenderer *ren = static_cast<iARenderer*>(clientData);
	ren->renderWindow()->GetRenderers()->GetFirstRenderer()->RemoveActor(ren->selectedActor());

	auto extractSelection = vtkSmartPointer<vtkExtractSelectedFrustum>::New();
	extractSelection->SetInputData(0, ren->getRenderObserver()->GetImageData());
	extractSelection->PreserveTopologyOff();
	extractSelection->SetFrustum(areaPicker->GetFrustum());
	extractSelection->Update();

	if (!extractSelection->GetOutput()->GetNumberOfElements(vtkUnstructuredGrid::CELL))
	{
		ren->emitNoSelectedCells();
		return;
	}
	
	if (ren->interactor()->GetControlKey() &&
		!ren->interactor()->GetShiftKey())
	{
		// Adds cells to selection
		auto append = vtkSmartPointer<vtkAppendFilter>::New();
		append->AddInputData(ren->finalSelection());
		append->AddInputData(extractSelection->GetOutput());
		append->Update();
		ren->finalSelection()->ShallowCopy(append->GetOutput());
	}
	else if (ren->interactor()->GetControlKey() &&
		ren->interactor()->GetShiftKey())
	{
		// Removes cells from selection 
		auto newfinalSel = vtkSmartPointer<vtkUnstructuredGrid>::New();
		newfinalSel->Allocate(1, 1);
		newfinalSel->SetPoints(ren->finalSelection()->GetPoints());
		auto currSel = vtkSmartPointer<vtkUnstructuredGrid>::New();
		currSel->ShallowCopy(extractSelection->GetOutput());
		double f_Cell[DIM] = { 0,0,0 }, c_Cell[DIM] = { 0,0,0 };
		double* spacing = ren->getRenderObserver()->GetImageData()->GetSpacing();

		for (vtkIdType i = 0; i < ren->finalSelection()->GetNumberOfCells(); ++i)
		{
			bool addCell = true;
			GetCellCenter(ren->finalSelection(), i, f_Cell, spacing);
			for (vtkIdType j = 0; j < currSel->GetNumberOfCells(); ++j)
			{
				GetCellCenter(currSel, j, c_Cell, spacing);
				if (f_Cell[0] == c_Cell[0] &&
					f_Cell[1] == c_Cell[1] &&
					f_Cell[2] == c_Cell[2])
				{
					addCell = false;
					break;
				}
			}
			if (addCell)
				newfinalSel->InsertNextCell(ren->finalSelection()->GetCell(i)->GetCellType(),
					ren->finalSelection()->GetCell(i)->GetPointIds());
		}		
		ren->finalSelection()->ShallowCopy(newfinalSel);
	}
	else
	{
		// New selection
		ren->finalSelection()->ShallowCopy(extractSelection->GetOutput());
	}
	ren->selectedMapper()->Update();
	ren->renderWindow()->GetRenderers()->GetFirstRenderer()->AddActor(ren->selectedActor());
	ren->emitSelectedCells(ren->finalSelection());
}

void GetCellCenter(vtkUnstructuredGrid* data, const unsigned int cellId, double center[DIM], double spacing[DIM])
{
	double pcoords[DIM] = { 0,0,0 };
	double *weights = new double[data->GetMaxCellSize()];
	vtkCell* cell = data->GetCell(cellId);
	int np = cell->GetPoints()->GetNumberOfPoints();
	int subId = cell->GetParametricCenter(pcoords);
	cell->EvaluateLocation(subId, pcoords, center, weights);
	for (int i = 0; i < DIM; ++i)
		center[i] = floor(center[i] / spacing[i]);
}

iARenderer::iARenderer(QObject *par)  :  QObject( par ),
	m_interactor(nullptr),
	m_renderObserver(nullptr),
	m_imageData(nullptr),
	m_renWin(vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New()),		// TODO: move out of here?
	m_cam(vtkSmartPointer<vtkCamera>::New()),
	m_cSource(vtkSmartPointer<vtkCubeSource>::New()),
	m_cMapper(vtkSmartPointer<vtkPolyDataMapper>::New()),
	m_cActor(vtkSmartPointer<vtkActor>::New()),
	m_logoRep(vtkSmartPointer<vtkLogoRepresentation>::New()),
	m_logoWidget(vtkSmartPointer<vtkLogoWidget>::New()),
	m_logoImage(vtkSmartPointer<vtkQImageToImageSource>::New()),
	m_labelRen(vtkSmartPointer<vtkOpenGLRenderer>::New()),
	m_ren(vtkSmartPointer<vtkOpenGLRenderer>::New()),
	m_polyMapper(vtkSmartPointer<vtkPolyDataMapper>::New()),
	m_polyActor(vtkSmartPointer<vtkActor>::New()),
	m_annotatedCubeActor(vtkSmartPointer<vtkAnnotatedCubeActor>::New()),
	m_axesActor(vtkSmartPointer<vtkAxesActor>::New()),
	m_moveableAxesActor(vtkSmartPointer<vtkAxesActor>::New()),
	m_orientationMarkerWidget(vtkSmartPointer<vtkOrientationMarkerWidget>::New()),
	m_plane1(vtkSmartPointer<vtkPlane>::New()),
	m_plane2(vtkSmartPointer<vtkPlane>::New()),
	m_plane3(vtkSmartPointer<vtkPlane>::New()),
	m_slicingCube(vtkSmartPointer<vtkCubeSource>::New()),
	m_sliceCubeMapper(vtkSmartPointer<vtkPolyDataMapper>::New()),
	m_sliceCubeActor(vtkSmartPointer<vtkActor>::New()),
	m_txtActor(vtkSmartPointer<vtkTextActor>::New()),
	m_cellLocator(vtkSmartPointer<vtkCellLocator>::New()),
	m_profileLineSource(vtkSmartPointer<vtkLineSource>::New()),
	m_profileLineMapper(vtkSmartPointer<vtkPolyDataMapper>::New()),
	m_profileLineActor(vtkSmartPointer<vtkActor>::New()),
	m_profileLineStartPointSource(vtkSmartPointer<vtkSphereSource>::New()),
	m_profileLineStartPointMapper(vtkSmartPointer<vtkPolyDataMapper>::New()),
	m_profileLineStartPointActor(vtkSmartPointer<vtkActor>::New()),
	m_profileLineEndPointSource(vtkSmartPointer<vtkSphereSource>::New()),
	m_profileLineEndPointMapper(vtkSmartPointer<vtkPolyDataMapper>::New()),
	m_profileLineEndPointActor(vtkSmartPointer<vtkActor>::New()),
	m_slicePlaneOpacity(0.8)
{
	m_renWin->AlphaBitPlanesOn();
	m_renWin->LineSmoothingOn();
	m_renWin->PointSmoothingOn();

	m_txtActor->SetInput("Selection mode");
	m_txtActor->GetTextProperty()->SetFontSize(24);
	m_txtActor->GetTextProperty()->SetColor(1.0, 0.0, 0.0);
	m_txtActor->GetTextProperty()->SetJustificationToLeft();
	m_txtActor->GetTextProperty()->SetVerticalJustificationToBottom();
	m_txtActor->VisibilityOff();
	m_txtActor->SetPickable(false);
	m_txtActor->SetDragable(false);
	for (int s = 0; s < 3; ++s)
	{
		m_slicePlaneSource[s] = vtkSmartPointer<vtkPlaneSource>::New();
		m_slicePlaneMapper[s] = vtkSmartPointer<vtkPolyDataMapper>::New();
		m_slicePlaneActor[s] = vtkSmartPointer<vtkActor>::New();
		m_slicePlaneActor[s]->GetProperty()->LightingOff();
		m_slicePlaneActor[s]->SetPickable(false);
		m_slicePlaneActor[s]->SetDragable(false);
	}

	MdiChild * mdi_parent = dynamic_cast<MdiChild*>(parent());
	if (mdi_parent)
	{
		connect(this, SIGNAL(msg(QString)), mdi_parent, SLOT(addMsg(QString)));
		connect(this, SIGNAL(progress(int)), mdi_parent, SLOT(updateProgressBar(int)));
	}
}

iARenderer::~iARenderer(void)
{
	m_ren->RemoveAllObservers();
	m_renWin->RemoveAllObservers();
	if (m_renderObserver) m_renderObserver->Delete();
}

void iARenderer::initialize( vtkImageData* ds, vtkPolyData* pd)
{
	m_imageData = ds;
	m_polyData = pd;
	m_cellLocator->SetDataSet(m_polyData);
	if(m_polyData)
		if( m_polyData->GetNumberOfCells() )
			m_cellLocator->BuildLocator();
	double spacing[3];	ds->GetSpacing(spacing);
	m_ren->SetLayer(0);
	m_ren->UseDepthPeelingOn();
#if (VTK_MAJOR_VERSION >= 8 && defined(VTK_OPENGL2_BACKEND) && QT_VERSION >= 0x050400 )
	m_ren->UseDepthPeelingForVolumesOn();
#endif
	m_labelRen->SetLayer(1);
	m_labelRen->InteractiveOff();
	m_labelRen->UseDepthPeelingOn();
	m_renWin->SetNumberOfLayers(5);
	m_renWin->AddRenderer(m_ren);
	m_renWin->AddRenderer(m_labelRen);
	m_interactor = m_renWin->GetInteractor();
	setPointPicker();	
	initObserver();

	QImage img;
	img.load(":/images/fhlogo.png");
	m_logoImage->SetQImage(&img);
	m_logoImage->Update();
	m_logoRep->SetImage(m_logoImage->GetOutput( ));
	m_logoWidget->SetInteractor(m_interactor);
	m_logoWidget->SetRepresentation(m_logoRep);
	m_logoWidget->SetResizable(false);
	m_logoWidget->SetSelectable(true);
	m_logoWidget->On();

	m_interactor->Initialize();

	// setup cube source
	updatePositionMarkerExtent();
	m_cMapper->SetInputConnection(m_cSource->GetOutputPort());
	m_cActor->SetMapper(m_cMapper);
	m_cActor->GetProperty()->SetColor(1,0,0);
	m_cActor->SetPickable(false);
	m_cActor->SetDragable(false);

	setupCutter();
	setupCube();
	setupAxes(spacing);
	setupOrientationMarker();
	setupRenderer();

	m_labelRen->SetActiveCamera(m_cam);
	m_ren->SetActiveCamera(m_cam);
	setCamPosition( 0,0,1, 1,1,1 ); // +Z

	m_profileLineMapper->SetInputConnection(m_profileLineSource->GetOutputPort());
	m_profileLineActor->SetMapper(m_profileLineMapper);
	m_profileLineActor->SetPickable(false);
	m_profileLineActor->SetDragable(false);
	m_profileLineStartPointMapper->SetInputConnection(m_profileLineStartPointSource->GetOutputPort());
	m_profileLineStartPointActor->SetMapper(m_profileLineStartPointMapper);
	m_profileLineStartPointActor->SetPickable(false);
	m_profileLineStartPointActor->SetDragable(false);
	m_profileLineEndPointMapper->SetInputConnection(m_profileLineEndPointSource->GetOutputPort());
	m_profileLineEndPointActor->SetMapper(m_profileLineEndPointMapper);
	m_profileLineEndPointActor->SetPickable(false);
	m_profileLineEndPointActor->SetDragable(false);
	m_profileLineActor->GetProperty()->SetColor(0.59, 0.73, 0.94);//ffa800//150, 186, 240
	m_profileLineActor->GetProperty()->SetLineWidth(2.0);
	m_profileLineActor->GetProperty()->SetLineStipplePattern(0x00ff);//0xf0f0
	m_profileLineActor->GetProperty()->SetLineStippleRepeatFactor(1);
	m_profileLineActor->GetProperty()->SetPointSize(2);
	m_profileLineActor->SetPickable(false);
	m_profileLineActor->SetDragable(false);
	m_profileLineStartPointSource->SetRadius(2 * spacing[0]);
	m_profileLineEndPointSource->SetRadius(2 * spacing[0]);
	m_profileLineStartPointActor->GetProperty()->SetColor(1.0, 0.65, 0.0);
	m_profileLineEndPointActor->GetProperty()->SetColor(0.0, 0.65, 1.0);

	//slicing cube settings for surface extraction
	m_sliceCubeMapper->SetInputConnection(m_slicingCube->GetOutputPort());
	m_sliceCubeActor->SetMapper(m_sliceCubeMapper);
	m_sliceCubeActor->GetProperty()->SetColor(1.0, 0, 0);
	m_sliceCubeActor->GetProperty()->SetRepresentationToWireframe();
	m_sliceCubeActor->GetProperty()->SetOpacity(1); 
	m_sliceCubeActor->GetProperty()->SetLineWidth(2.3);
	m_sliceCubeActor->GetProperty()->SetAmbient(1.0);
	m_sliceCubeActor->GetProperty()->SetDiffuse(0.0);
	m_sliceCubeActor->GetProperty()->SetSpecular(0.0);
	m_sliceCubeActor->SetPickable(false);
	m_sliceCubeActor->SetDragable(false);
	m_sliceCubeActor->SetVisibility(false);

	setArbitraryProfileOn(false);

	 updateSlicePlanes(m_imageData->GetSpacing());
	for (int s = 0; s < 3; ++s)
	{
		m_slicePlaneMapper[s]->SetInputConnection(m_slicePlaneSource[s]->GetOutputPort());
		m_slicePlaneActor[s]->SetMapper(m_slicePlaneMapper[s]);
		m_slicePlaneActor[s]->GetProperty()->SetColor( (s == 0) ? 1:0, (s == 1) ? 1 : 0, (s == 2) ? 1 : 0);
		m_slicePlaneActor[s]->GetProperty()->SetOpacity(1.0);
		m_slicePlaneMapper[s]->Update();
	}
}

void iARenderer::reInitialize( vtkImageData* ds, vtkPolyData* pd)
{
	m_imageData = ds;
	m_polyData = pd;
	updatePositionMarkerExtent();
	if (m_polyData)
	{
		m_cellLocator->SetDataSet(m_polyData );
		if (m_polyData->GetNumberOfCells())
			m_cellLocator->BuildLocator();
	}
	m_polyMapper->SetInputData(m_polyData);
	m_renderObserver->ReInitialize(m_ren, m_labelRen, m_interactor, m_pointPicker,
		m_moveableAxesTransform, ds,
		m_plane1, m_plane2, m_plane3, m_cellLocator );
	m_interactor->ReInitialize();
	emit reInitialized();
	update();
}

void iARenderer::setAreaPicker()
{
	auto areaPicker = vtkSmartPointer<vtkAreaPicker>::New();
	m_interactor->SetPicker(areaPicker);
	auto style = vtkSmartPointer<MouseInteractorStyle>::New();
	m_interactor->SetInteractorStyle(style);

	auto keyPressCallback = vtkSmartPointer<vtkCallbackCommand>::New();
	keyPressCallback->SetCallback(KeyPressCallbackFunction);
	keyPressCallback->SetClientData(this);
	m_interactor->AddObserver(vtkCommand::KeyReleaseEvent, keyPressCallback, 1.0);

	auto pickCallback = vtkSmartPointer<vtkCallbackCommand>::New();
	pickCallback->SetCallback(PickCallbackFunction);
	pickCallback->SetClientData(this);
	areaPicker->AddObserver(vtkCommand::EndPickEvent, pickCallback, 1.0);

	m_finalSelection = vtkSmartPointer<vtkUnstructuredGrid>::New();
	m_selectedMapper = vtkSmartPointer<vtkDataSetMapper>::New();
	m_selectedMapper->SetScalarModeToUseCellData();
	m_selectedMapper->SetInputData(m_finalSelection);
	m_selectedActor = vtkSmartPointer<vtkActor>::New();
	QColor c(255, 0, 0);
	m_selectedActor->SetMapper(m_selectedMapper);
	m_selectedActor->GetProperty()->SetColor(c.redF(), c.greenF(), c.blueF());
	m_selectedActor->GetProperty()->SetRepresentationToWireframe();
	m_selectedActor->GetProperty()->EdgeVisibilityOn();
	m_selectedActor->GetProperty()->SetEdgeColor(c.redF(), c.greenF(), c.blueF());
	m_selectedActor->GetProperty()->SetLineWidth(3);
}

void iARenderer::setPointPicker()
{
	m_pointPicker = vtkSmartPointer<vtkPicker>::New();
	m_pointPicker->SetTolerance(0.00005);//spacing[0]/150);
	m_interactor->SetPicker(m_pointPicker);
	setDefaultInteractor();
}

void iARenderer::setDefaultInteractor()
{
	m_interactor->SetInteractorStyle(vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New());
}

void iARenderer::setupCutter()
{
	m_plane1->SetNormal(1, 0, 0);
	m_plane2->SetNormal(0, 1, 0);
	m_plane3->SetNormal(0, 0, 1);
}

void iARenderer::setupCube()
{
	m_annotatedCubeActor->SetPickable(1);
	m_annotatedCubeActor->SetXPlusFaceText("+X");
	m_annotatedCubeActor->SetXMinusFaceText("-X");
	m_annotatedCubeActor->SetYPlusFaceText("+Y");
	m_annotatedCubeActor->SetYMinusFaceText("-Y");
	m_annotatedCubeActor->SetZPlusFaceText("+Z");
	m_annotatedCubeActor->SetZMinusFaceText("-Z");
	m_annotatedCubeActor->SetXFaceTextRotation(0);
	m_annotatedCubeActor->SetYFaceTextRotation(0);
	m_annotatedCubeActor->SetZFaceTextRotation(90);
	m_annotatedCubeActor->SetFaceTextScale(0.45);
	m_annotatedCubeActor->GetCubeProperty()->SetColor(0.7, 0.78, 1);
	m_annotatedCubeActor->GetTextEdgesProperty()->SetDiffuse(0);
	m_annotatedCubeActor->GetTextEdgesProperty()->SetAmbient(0);
	m_annotatedCubeActor->GetXPlusFaceProperty()->SetColor(1, 0, 0);
	m_annotatedCubeActor->GetXPlusFaceProperty()->SetInterpolationToFlat();
	m_annotatedCubeActor->GetXMinusFaceProperty()->SetColor(1, 0, 0);
	m_annotatedCubeActor->GetXMinusFaceProperty()->SetInterpolationToFlat();
	m_annotatedCubeActor->GetYPlusFaceProperty()->SetColor(0, 1, 0);
	m_annotatedCubeActor->GetYPlusFaceProperty()->SetInterpolationToFlat();
	m_annotatedCubeActor->GetYMinusFaceProperty()->SetColor(0, 1, 0);
	m_annotatedCubeActor->GetYMinusFaceProperty()->SetInterpolationToFlat();
	m_annotatedCubeActor->GetZPlusFaceProperty()->SetColor(0, 0, 1);
	m_annotatedCubeActor->GetZPlusFaceProperty()->SetInterpolationToFlat();
	m_annotatedCubeActor->GetZMinusFaceProperty()->SetColor(0, 0, 1);
	m_annotatedCubeActor->GetZMinusFaceProperty()->SetInterpolationToFlat();
}

void iARenderer::setupAxes(double spacing[3])
{
	m_axesActor->AxisLabelsOff();
	m_axesActor->SetShaftTypeToCylinder();
	m_axesActor->SetTotalLength(15, 15, 15);

	vtkTransform *transform = vtkTransform::New();
	transform->Scale(spacing[0]*3, spacing[1]*3, spacing[2]*3);

	m_axesActor->SetUserTransform(transform);
	transform->Delete();

	m_moveableAxesActor->AxisLabelsOff();
	m_moveableAxesActor->SetShaftTypeToCylinder();
	m_moveableAxesActor->SetTotalLength(15, 15, 15);
	m_moveableAxesActor->SetPickable(false);
	m_moveableAxesActor->SetDragable(false);

	m_moveableAxesTransform->Scale(spacing[0]*3, spacing[1]*3, spacing[2]*3);

	m_moveableAxesActor->SetUserTransform(m_moveableAxesTransform);
}

void iARenderer::setupOrientationMarker()
{
	m_orientationMarkerWidget->SetOrientationMarker(m_annotatedCubeActor);
	m_orientationMarkerWidget->SetViewport(0.0, 0.0, 0.2, 0.2);
	m_orientationMarkerWidget->SetInteractor(m_interactor);
	m_orientationMarkerWidget->SetEnabled( 1 );
	m_orientationMarkerWidget->InteractiveOff();
}

void iARenderer::setupRenderer()
{
	m_polyMapper->SetInputData(m_polyData);
	m_polyMapper->SelectColorArray("Colors");
	m_polyMapper->SetScalarModeToUsePointFieldData();
	m_polyActor->SetMapper(m_polyMapper);

	m_ren->GradientBackgroundOn();
	m_ren->AddActor2D(m_txtActor);
	m_ren->AddActor(m_polyActor);
	m_ren->AddActor(m_cActor);
	m_ren->AddActor(m_axesActor);
	m_ren->AddActor(m_moveableAxesActor);
	m_ren->AddActor(m_profileLineActor);
	m_ren->AddActor(m_profileLineStartPointActor);
	m_ren->AddActor(m_profileLineEndPointActor);
	m_ren->AddActor(m_sliceCubeActor); 
	emit onSetupRenderer();
}

void iARenderer::update()
{
	if (m_polyData)
		m_polyMapper->Update();
	m_ren->Render();
	m_renWin->Render();
	m_renWin->GetInteractor()->Render();
}

void iARenderer::showHelpers(bool show)
{
	m_orientationMarkerWidget->SetEnabled(show);
	m_axesActor->SetVisibility(show);
	m_moveableAxesActor->SetVisibility(show);
	m_logoWidget->SetEnabled(show);
}

void iARenderer::showRPosition(bool s)
{
	m_cActor->SetVisibility(s);
}

void iARenderer::showSlicePlanes(bool show)
{
	for (int s = 0; s < 3; ++s)
	{
		if (show)
			m_ren->AddActor(m_slicePlaneActor[s]);
		else
			m_ren->RemoveActor(m_slicePlaneActor[s]);
		m_slicePlaneActor[s]->GetProperty()->SetOpacity(m_slicePlaneOpacity);
	}
}

void iARenderer::setPlaneNormals( vtkTransform *tr )
{
	double normal[4], temp[4];

	normal[0] = 1; normal[1] = 0; normal[2] = 0; normal[3] = 1;
	tr->GetMatrix()->MultiplyPoint(normal, temp);
	m_plane1->SetNormal( temp[0], temp[1], temp[2] );

	normal[0] = 0; normal[1] = 1; normal[2] = 0; normal[3] = 1;
	tr->GetMatrix()->MultiplyPoint(normal, temp);
	m_plane2->SetNormal( temp[0], temp[1], temp[2] );

	normal[0] = 0; normal[1] = 0; normal[2] = 1; normal[3] = 1;
	tr->GetMatrix()->MultiplyPoint(normal, temp);
	m_plane3->SetNormal( temp[0], temp[1], temp[2] );

	m_renWin->Render();
	m_ren->Render();
};

void iARenderer::setCubeCenter( int x, int y, int z )
{
	if (m_interactor->GetEnabled()) {
		m_cSource->SetCenter( x * m_imageData->GetSpacing()[0],
			y * m_imageData->GetSpacing()[1],
			z * m_imageData->GetSpacing()[2] );
		update();
	}
};

void iARenderer::setCamPosition( int uvx, int uvy, int uvz, int px, int py, int pz )
{
	m_cam->SetViewUp ( uvx, uvy, uvz );
	m_cam->SetPosition ( px, py, pz );
	m_cam->SetFocalPoint( 0,0,0 );
	m_ren->ResetCamera();
	update();
}


void iARenderer::camPosition( double * camOptions )
{
	double pS = m_cam->GetParallelScale();
	double a[3] = {0};
	double b[3] = {0};
	double c[3] = {0};
	m_cam->GetViewUp(a);
	m_cam->GetPosition(b);
	m_cam->GetFocalPoint(c);

	camOptions[0] = a[0];
	camOptions[1] = a[1];
	camOptions[2] = a[2];
	camOptions[3] = b[0];
	camOptions[4] = b[1];
	camOptions[5] = b[2];
	camOptions[6] = c[0];
	camOptions[7] = c[1];
	camOptions[8] = c[2];
	camOptions[9] = pS;

	m_ren->ResetCamera();
	update();
}

void iARenderer::setCamPosition( double * camOptions, bool rsParallelProjection )
{
	m_cam->SetViewUp ( camOptions[0], camOptions[1], camOptions[2] );
	m_cam->SetPosition ( camOptions[3], camOptions[4], camOptions[5] );
	m_cam->SetFocalPoint( camOptions[6], camOptions[7], camOptions[8] );

	if(rsParallelProjection)
		m_cam->SetParallelScale( camOptions[9] );

	update();
}

void iARenderer::setCamera(vtkCamera* c)
{
	m_cam = c;
	m_labelRen->SetActiveCamera(m_cam);
	m_ren->SetActiveCamera(m_cam);
	emit onSetCamera();
}

vtkCamera* iARenderer::camera()
{
	return m_cam;
}

void iARenderer::setStatExt(int s)
{
	m_ext = s;
	updatePositionMarkerExtent();
}

void iARenderer::updatePositionMarkerExtent()
{
	if (!m_imageData)
		return;
	double const * spacing = m_imageData->GetSpacing();
	m_cSource->SetXLength(m_ext * spacing[0]);
	m_cSource->SetYLength(m_ext * spacing[1]);
	m_cSource->SetZLength(m_ext * spacing[2]);
}

void iARenderer::setSlicePlaneOpacity(float opc)
{
	if ((opc > 1.0) || (opc < 0.0f))
	{
		DEBUG_LOG(QString("Invalid slice plane opacity %1").arg(opc));
		return;
	}

	m_slicePlaneOpacity = opc;
}

void iARenderer::saveMovie( const QString& fileName, int mode, int qual /*= 2*/ )
{
	auto movieWriter = GetMovieWriter(fileName, qual);

	if (movieWriter.GetPointer() == nullptr)
		return;

	// save current state and disable interaction:
	m_interactor->Disable();
	auto oldCam = vtkSmartPointer<vtkCamera>::New();
	oldCam->DeepCopy(m_cam);

	// set up movie export pipeline:
	auto windowToImage = vtkSmartPointer<vtkWindowToImageFilter>::New();
	int* rws = m_renWin->GetSize();
	if (rws[0] % 2 != 0) rws[0]++;
	if (rws[1] % 2 != 0) rws[1]++;
	m_renWin->SetSize(rws);
	m_renWin->Render();
	windowToImage->SetInput(m_renWin);
	windowToImage->ReadFrontBufferOff();
	movieWriter->SetInputConnection(windowToImage->GetOutputPort());
	movieWriter->Start();

	emit msg(tr("Movie export started, output file name: %1").arg(fileName));

	int numRenderings = 360;//TODO
	auto rot = vtkSmartPointer<vtkTransform>::New();
	m_cam->SetFocalPoint( 0,0,0 );
	double view[3];
	double point[3];
	if (mode == 0)
	{ // YZ
		double _view[3]  = { 0, 0, -1 };
		double _point[3] = { 1, 0,  0 };
		for (int ind=0; ind<3; ind++)
		{
			view[ind] = _view[ind];
			point[ind] = _point[ind];
		}
		rot->RotateZ(360/numRenderings);
	}
	else if (mode == 1)
	{ // XY
		double _view[3]  = { 0, 0, -1 };
		double _point[3] = { 0, 1,  0 };
		for (int ind=0; ind<3; ind++)
		{
			view[ind] = _view[ind];
			point[ind] = _point[ind];
		}
		rot->RotateX(360/numRenderings);
	}
	else if (mode == 2)
	{ // XZ
		double _view[3]  = { 0, 1, 0 };
		double _point[3] = { 0, 0, 1 };
		for (int ind=0; ind<3; ind++)
		{
			view[ind] = _view[ind];
			point[ind] = _point[ind];
		}
		rot->RotateY(360/numRenderings);
	}
	m_cam->SetViewUp ( view );
	m_cam->SetPosition ( point );
	for (int i =0; i < numRenderings; i++ )
	{
		m_ren->ResetCamera();
		m_renWin->Render();

		windowToImage->Modified();
		movieWriter->Write();
		if (movieWriter->GetError())
		{
			emit msg(movieWriter->GetStringFromErrorCode(movieWriter->GetErrorCode()));
			break;
		}
		emit progress( 100 * (i+1) / numRenderings);
		m_cam->ApplyTransform(rot);
	}

	m_cam->DeepCopy(oldCam);
	movieWriter->End();
	m_interactor->Enable();

	if (movieWriter->GetError())
		emit msg(tr("Movie export failed."));
	else
		emit msg(tr("Movie export completed."));
}

void iARenderer::mouseRightButtonReleasedSlot()
{
	if (!m_interactor)
		return;
	m_interactor->InvokeEvent(vtkCommand::RightButtonReleaseEvent);
}

void iARenderer::mouseLeftButtonReleasedSlot()
{
	if (!m_interactor)
		return;
	m_interactor->InvokeEvent(vtkCommand::LeftButtonReleaseEvent);
}

void iARenderer::setArbitraryProfile(int pointIndex, double * coords)
{
	if (pointIndex == 0)
	{
		m_profileLineSource->SetPoint1(coords);
		m_profileLineStartPointSource->SetCenter(coords);
		m_profileLineMapper->Update();
		m_profileLineStartPointMapper->Update();
	}
	else
	{
		m_profileLineSource->SetPoint2(coords);
		m_profileLineEndPointSource->SetCenter(coords);
		m_profileLineMapper->Update();
		m_profileLineEndPointMapper->Update();
	}
	update();
}

void iARenderer::setArbitraryProfileOn(bool isOn)
{
	m_profileLineActor->SetVisibility(isOn);
	m_profileLineStartPointActor->SetVisibility(isOn);
	m_profileLineEndPointActor->SetVisibility(isOn);
}

void iARenderer::initObserver()
{
	m_renderObserver = iARenderObserver::New(m_ren, m_labelRen, m_interactor, m_pointPicker,
		m_moveableAxesTransform, m_imageData,
		m_plane1, m_plane2, m_plane3, m_cellLocator);

	m_interactor->AddObserver(vtkCommand::KeyPressEvent, m_renderObserver, 0.0);
	m_interactor->AddObserver(vtkCommand::LeftButtonPressEvent, m_renderObserver);
	m_interactor->AddObserver(vtkCommand::LeftButtonReleaseEvent, m_renderObserver);
	m_interactor->AddObserver(vtkCommand::RightButtonPressEvent, m_renderObserver);
	m_interactor->AddObserver(vtkCommand::RightButtonReleaseEvent, m_renderObserver);
}

void iARenderer::setPolyData(vtkPolyData* pd)
{
	m_polyData = pd;
	if (m_polyData)
	{
		m_cellLocator->SetDataSet(m_polyData);
		if (m_polyData->GetNumberOfCells())
			m_cellLocator->BuildLocator();
	}
	m_polyMapper->SetInputData(m_polyData );
}

void iARenderer::addRenderer(vtkRenderer* renderer)
{
	m_renWin->AddRenderer(renderer);
}

void iARenderer::disableInteractor() { m_interactor->Disable(); }
void iARenderer::enableInteractor()  { m_interactor->ReInitialize(); }
vtkPlane* iARenderer::plane1() { return m_plane1; };
vtkPlane* iARenderer::plane2() { return m_plane2; };
vtkPlane* iARenderer::plane3() { return m_plane3; };
vtkOpenGLRenderer * iARenderer::renderer() { return m_ren; };
vtkRenderWindowInteractor* iARenderer::interactor() { return m_interactor; }
vtkRenderWindow* iARenderer::renderWindow() { return m_renWin; }
vtkOpenGLRenderer * iARenderer::labelRenderer(void) { return m_labelRen; }
vtkTextActor* iARenderer::txtActor() { return m_txtActor; }
vtkPolyData* iARenderer::polyData() { return m_polyData; }
vtkActor* iARenderer::polyActor() { return m_polyActor; };
vtkPolyDataMapper* iARenderer::polyMapper() const { return m_polyMapper; }
vtkActor* iARenderer::selectedActor() { return m_selectedActor; }
vtkUnstructuredGrid* iARenderer::finalSelection() { return m_finalSelection; }
vtkDataSetMapper* iARenderer::selectedMapper() { return m_selectedMapper; }
vtkTransform* iARenderer::coordinateSystemTransform() { m_moveableAxesTransform->Update(); return m_moveableAxesTransform; }
void iARenderer::setAxesTransform(vtkTransform *transform) { m_moveableAxesTransform = transform; }
vtkTransform * iARenderer::axesTransform(void) { return m_moveableAxesTransform; }
iARenderObserver * iARenderer::getRenderObserver() { return m_renderObserver; }

void iARenderer::setSlicingBounds(const int roi[6], const double * spacing)
{
	double xMin = roi[0] * spacing[0],
	       yMin = roi[1] * spacing[1],
	       zMin = roi[2] * spacing[2];
	double xMax = xMin + roi[3] * spacing[0],
	       yMax = yMin + roi[4] * spacing[1],
	       zMax = zMin + roi[5] * spacing[2];
	m_slicingCube->SetBounds(xMin, xMax, yMin, yMax, zMin, zMax);
	update();
}

void iARenderer::setCubeVisible(bool visible)
{
	m_sliceCubeActor->SetVisibility(visible);
}

void iARenderer::setSlicePlane(int planeID, double originX, double originY, double originZ)
{
	switch (planeID)
	{
		default:
		case iASlicerMode::YZ: m_plane1->SetOrigin(originX, originY, originZ); break;
		case iASlicerMode::XZ: m_plane2->SetOrigin(originX, originY, originZ); break;
		case iASlicerMode::XY: m_plane3->SetOrigin(originX, originY, originZ); break;
	}
	double center[3];
	m_slicePlaneSource[planeID]->GetCenter(center);
	center[planeID] = (planeID == 0) ? originX : ((planeID == 1) ? originY : originZ);
	m_slicePlaneSource[planeID]->SetCenter(center);
	m_slicePlaneMapper[planeID]->Update();
	update();
}

void iARenderer::applySettings(iARenderSettings const & settings)
{
#if (VTK_MAJOR_VERSION > 7 || (VTK_MAJOR_VERSION == 7 && VTK_MINOR_VERSION > 0))
	m_ren->SetUseFXAA(settings.UseFXAA);
#else
	DEBUG_LOG("FXAA Anti-Aliasing is not support with your VTK version");
#endif
	m_cam->SetParallelProjection(settings.ParallelProjection);
	QColor bgTop(settings.BackgroundTop);
	QColor bgBottom(settings.BackgroundBottom);
	
	setSlicePlaneOpacity(settings.PlaneOpacity);

	m_ren->SetBackground2(bgTop.redF(), bgTop.greenF(), bgTop.blueF());
	m_ren->SetBackground(bgBottom.redF(), bgBottom.greenF(), bgBottom.blueF());
	showHelpers(settings.ShowHelpers);
	showRPosition(settings.ShowRPosition);
	showSlicePlanes(settings.ShowSlicePlanes);
	//renWin->Render();
}

void iARenderer::emitSelectedCells(vtkUnstructuredGrid* selectedCells)
{
	double cell[DIM] = { 0,0,0 };
	double* spacing = getRenderObserver()->GetImageData()->GetSpacing();
	auto selCellPoints = vtkSmartPointer<vtkPoints>::New();
	for (vtkIdType i = 0; i < selectedCells->GetNumberOfCells(); ++i)
	{
		GetCellCenter(selectedCells, i, cell, spacing);
		selCellPoints->InsertNextPoint(cell);
	}
	emit cellsSelected(selCellPoints);
}

void iARenderer::emitNoSelectedCells()
{
	emit noCellsSelected();
}

void iARenderer::updateSlicePlanes(double const * newSpacing)
{
	if (!newSpacing)
	{
		DEBUG_LOG("Spacing is NULL");
		return;
	}
	double const * spc = newSpacing;

	double center[3], origin[3];
	const int * dim = imageData->GetDimensions();
	if (dim[0] == 0 || dim[1] == 0 || dim[2] == 0)
		return;
	for (int i = 0; i < 3; ++i)
	{
		center[i] = dim[i] * spc[i] / 2;
		origin[i] = 0;
	}
	for (int s = 0; s < 3; ++s)
	{
		m_slicePlaneSource[s]->SetOrigin(origin);
		double point1[3], point2[3];
		for (int j = 0; j < 3; ++j)
		{
			point1[j] = 0;
			point2[j] = 0;
		}
		int slicerXAxisIdx = mapSliceToGlobalAxis(s, iAAxisIndex::X);
		int slicerYAxisIdx = mapSliceToGlobalAxis(s, iAAxisIndex::Y);
		point1[slicerXAxisIdx] += 1.1 * dim[slicerXAxisIdx] * spc[slicerXAxisIdx];
		point2[slicerYAxisIdx] += 1.1 * dim[slicerYAxisIdx] * spc[slicerYAxisIdx];
		m_slicePlaneSource[s]->SetPoint1(point1);
		m_slicePlaneSource[s]->SetPoint2(point2);
		m_slicePlaneSource[s]->SetCenter(center);
	}
}