/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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
#include "iALabel3D.h"

#include <vtkCellArray.h>
#include <vtkFloatArray.h>
#include <vtkFollower.h>
#include <vtkImageData.h>
#include <vtkLineSource.h>
#include <vtkMath.h>
#include <vtkPointData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkQuad.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkTexture.h>
#include <vtkTubeFilter.h>
#include <vtkVersion.h>


#include <vtkQImageToImageSource.h>
#include <QPixmap>
#include <vtkVectorText.h>
#include <vtkAppendPolyData.h>
#include <vtkImageDataGeometryFilter.h>
#include <vtkCaptionActor2D.h>
#include <vtkTextProperty.h>
#include <vtkLegendBoxActor.h>
#include <vtkSphereSource.h>

//appearance settings
#define LABEL_SCALE 1.0//26.0
#define LABEL_TUBE_RAD 0.3
#define LABEL_DISPLACEMENT 30.0

const int NUM_PTS = 4;
const float TUPLES[NUM_PTS][3] =
{
	{0.0, 1.0, 0.0},
	{1.0, 1.0, 0.0},
	{1.0, 0.0, 0.0},
	{0.0, 0.0, 0.0}
};

iALabel3D::iALabel3D(bool showLine) :
	m_showLine(showLine),
	m_scale(LABEL_SCALE),
	m_tubeRadius(LABEL_TUBE_RAD),
	m_displacement(LABEL_DISPLACEMENT)
{
	m_labeledPnt[0] = m_labeledPnt[1] = m_labeledPnt[2] = 0;
	m_centerPnt[0] = m_centerPnt[1] = m_centerPnt[2] = 0;
	// create VTK image
	m_imageData = vtkImageData::New();

	// create VTK quad representation
	vtkSmartPointer<vtkFloatArray> texCoords =	vtkSmartPointer<vtkFloatArray>::New();
	texCoords->SetNumberOfComponents(3);
	texCoords->SetName("TextureCoordinates");
	for(int i=0; i<NUM_PTS; ++i)
		texCoords->InsertNextTuple(TUPLES[i]);
	m_pnts = vtkPoints::New();
	m_quad = vtkQuad::New();
	for(int i=0; i<NUM_PTS; ++i)
		m_quad->GetPointIds()->SetId(i,i);
	m_quadCell = vtkCellArray::New();
	m_quadCell->InsertNextCell(m_quad);
	m_polyData = vtkPolyData::New();
	m_polyData->SetPoints(m_pnts);
	m_polyData->SetPolys(m_quadCell);
	m_polyData->GetPointData()->SetTCoords(texCoords);

	vtkSmartPointer<vtkVectorText> textSource = vtkSmartPointer<vtkVectorText>::New();
	textSource->SetText("Hello");

	//// apply the texture
	//m_texture = vtkTexture::New();
	//m_texture->SetInputData(m_imageData);//m_texture->SetInputConnection(labelImgSrc->GetOutputPort());
	//m_texture->InterpolateOn();

	//QImage* img = new QImage();
	//img->load("C:/test.png");

	//img->save("C:/testcopy.png");

	////QString url = "C:/test.png";
	////QPixmap img(url);

	//vtkSmartPointer<vtkQImageToImageSource> qImageToVtk = vtkSmartPointer<vtkQImageToImageSource>::New();
	//qImageToVtk->SetQImage(img);
	//qImageToVtk->Update();

	//vtkSmartPointer<vtkTexture> texture = vtkSmartPointer<vtkTexture>::New();
	//texture->SetInputConnection(qImageToVtk->GetOutputPort());
	//texture->Update();

	// VTK mappers and actors
	m_mapper = vtkPolyDataMapper::New();
	m_mapper->SetInputConnection(textSource->GetOutputPort());
	m_mapper->Update();
	follower = vtkFollower::New();//follower = vtkActor::New();
	follower->SetMapper(m_mapper);
	follower->GetProperty()->SetColor(0, 0, 0);
	follower->SetScale(0.3, 0.3, 0.3);
	//follower->SetTexture(m_texture);
	m_lineSource = vtkLineSource::New();
	m_lineTubeFilter = vtkTubeFilter::New();
	m_lineTubeFilter->SetInputConnection(m_lineSource->GetOutputPort());
	m_lineMapper = vtkPolyDataMapper::New();
	m_lineMapper->SetInputConnection(m_lineTubeFilter->GetOutputPort());//m_lineMapper->SetInputConnection(m_lineSource->GetOutputPort());
	m_lineActor = vtkActor::New();
	m_lineActor->SetMapper(m_lineMapper);
	// appearance settings
	//follower->GetProperty()->SetAmbientColor(1.0, 1.0, 1.0);
	//follower->GetProperty()->SetAmbient(1.0);
	//follower->SetScale(m_scale);
	m_lineTubeFilter->CappingOn();
	m_lineTubeFilter->SetRadius(m_tubeRadius);
	m_lineTubeFilter->SetNumberOfSides(8);
	m_lineActor->GetProperty()->SetColor(0, 0, 0);
	m_lineActor->GetProperty()->SetLineWidth(4.0);
	m_lineActor->PickableOff();
	if(!m_showLine)
		m_lineActor->SetVisibility(false);

	// make the label up to date
	Update();
}

iALabel3D::~iALabel3D()
{
	m_texture->Delete();
	m_mapper->Delete();
	follower->Delete();
	m_imageData->Delete();
	m_lineSource->Delete();
	m_lineTubeFilter->Delete();
	m_lineMapper->Delete();
	m_lineActor->Delete();
	m_pnts->Delete();
	m_quad->Delete();
	m_quadCell->Delete();
	m_polyData->Delete();
}

void iALabel3D::SetupLabelQuad()
{
	double aspRatio = ((double)qImage.width()) / ((double)qImage.height());
	// Create a plane
	double hAR = 0.5*aspRatio;
	double pnts[NUM_PTS][3] =
	{
		{-hAR, -0.5, 0.0},
		{ hAR, -0.5, 0.0},
		{ hAR,  0.5, 0.0},
		{-hAR,  0.5, 0.0}
	};
	m_pnts->SetNumberOfPoints(NUM_PTS);
	for (int i=0; i<NUM_PTS; ++i)
		m_pnts->SetPoint(i, pnts[i]);
	m_pnts->Modified();
}

void iALabel3D::AttachActorsToRenderers( vtkRenderer * ren, vtkRenderer * labelRen, vtkCamera * cam ) const
{
	ren->AddActor(m_lineActor);
	follower->SetCamera(cam);
	labelRen->AddViewProp(follower);//ren->AddActor(follower);


	vtkSmartPointer<vtkCaptionActor2D> captionActor = vtkSmartPointer<vtkCaptionActor2D>::New();
	captionActor->SetCaption("Test");
	captionActor->SetAttachmentPoint(m_labeledPnt[0], m_labeledPnt[1], m_labeledPnt[2]);
	captionActor->BorderOff();
	captionActor->PickableOn();
	captionActor->DragableOn();
	captionActor->GetCaptionTextProperty()->BoldOff();
	captionActor->GetCaptionTextProperty()->ItalicOff();
	captionActor->GetCaptionTextProperty()->ShadowOff();
	captionActor->GetCaptionTextProperty()->SetBackgroundColor(0.0, 0.0, 0.0);
	captionActor->GetCaptionTextProperty()->SetBackgroundOpacity(0.3);
	captionActor->GetCaptionTextProperty()->SetColor(1.0, 0.0, 0.0);
#if (VTK_MAJOR_VERSION >= 8)
	captionActor->GetCaptionTextProperty()->FrameOff();
	captionActor->GetCaptionTextProperty()->UseTightBoundingBoxOn();
#endif
	ren->AddViewProp(captionActor);

	// Create the actor
	   vtkSmartPointer<vtkLegendBoxActor> actor = vtkSmartPointer<vtkLegendBoxActor>::New();
	   actor->SetNumberOfEntries(1);
	   actor->SetUseBackground(1);
	   actor->SetBackgroundColor(0.5, 0.5, 0.5);
	   actor->SetBackgroundOpacity(1.0);
	
	   actor->GetPositionCoordinate()->SetCoordinateSystemToView();
	actor->GetPositionCoordinate()->SetValue(-0.7, -0.8);
	
	   actor->GetPosition2Coordinate()->SetCoordinateSystemToView();
	   actor->GetPosition2Coordinate()->SetValue(0.7, 0.8);
	   double textColor[3] = { 1.0, 0.0, 0.0 };
	  vtkSmartPointer<vtkSphereSource> sphere = vtkSmartPointer<vtkSphereSource>::New();
	      sphere->SetRadius(10.0);
	      sphere->Update();
	actor->SetEntry(1, sphere->GetOutput(), "tetst", textColor);
	ren->AddViewProp(captionActor);

}

void iALabel3D::SetVisible( bool isVisible )
{
	follower->SetVisibility(isVisible);//follower->SetVisibility(0);
	if(m_showLine)
		m_lineActor->SetVisibility(isVisible);//m_lineActor->SetVisibility(0);
}

void iALabel3D::SetLabeledPoint( double labeledPnt[3], double centerPnt[3])
{
	size_t sz = sizeof(double)*3;
	memcpy(m_labeledPnt, labeledPnt, sz);
	memcpy(m_centerPnt,  centerPnt,  sz);

	UpdateLabelPositioning();
}

void iALabel3D::UpdateLabelPositioning()
{
	double fromCenter[3], displacedLabelPnt[3];

	vtkMath::Subtract(m_labeledPnt, m_centerPnt, fromCenter);
	vtkMath::Normalize(fromCenter);
	vtkMath::MultiplyScalar(fromCenter, m_displacement);
	vtkMath::Add(m_labeledPnt, fromCenter, displacedLabelPnt);

	follower->SetPosition(displacedLabelPnt);
	m_lineSource->SetPoint1(m_labeledPnt);
	m_lineSource->SetPoint2(displacedLabelPnt);
}

void iALabel3D::UpdateImageData()
{
	int w = qImage.width();
	int h = qImage.height();
	if(m_imageData->GetDimensions()[0] != w ||
		m_imageData->GetDimensions()[1] != h)
	{
		m_imageData->SetDimensions(w, h, 1);
		m_imageData->AllocateScalars(VTK_UNSIGNED_CHAR, 4);
	}
	if (w != 0 && h != 0)
	{
		memcpy(m_imageData->GetScalarPointer(), qImage.rgbSwapped().constBits(), w*h * 4);

		//qImage.save("C:/test.png");
		QImage* img = new QImage();
		img->load("C:/test.png");

		vtkSmartPointer<vtkQImageToImageSource> qImageToVtk = vtkSmartPointer<vtkQImageToImageSource>::New();
		qImageToVtk->SetQImage(img);
		qImageToVtk->Update();

		vtkSmartPointer<vtkImageDataGeometryFilter> imageDataGeometryFilter = vtkSmartPointer<vtkImageDataGeometryFilter>::New();
		imageDataGeometryFilter->SetInputConnection(qImageToVtk->GetOutputPort());
		imageDataGeometryFilter->Update();

		
		//m_mapper->SetInputConnection(imageDataGeometryFilter->GetOutputPort());
		//m_mapper->Update();
		
		//m_imageData->Modified();
		//m_texture->SetInputData(m_imageData);
		//m_texture->Update();
		//follower->SetTexture(texture);
	}
}

void iALabel3D::Update()
{
	SetupLabelQuad();
	UpdateImageData();
}

void iALabel3D::SetScale( double scale )
{
	m_scale = scale;
	follower->SetScale(m_scale);
}

void iALabel3D::SetTubeRadius( double tubeRadius )
{
	m_tubeRadius = tubeRadius;
	m_lineTubeFilter->SetRadius(m_tubeRadius);
}

void iALabel3D::SetDisplacement( double displacement )
{
	m_displacement = displacement;
	UpdateLabelPositioning();
}

void iALabel3D::SetShowLine( bool showLine )
{
	m_showLine = showLine;
	m_lineActor->SetVisibility(m_showLine);
}

void iALabel3D::DetachActorsToRenderers( vtkRenderer * ren, vtkRenderer * labelRen )
{
	ren->RemoveActor(m_lineActor);
	labelRen->RemoveActor(follower);
}
