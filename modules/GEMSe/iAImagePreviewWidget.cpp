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
#include "iAImagePreviewWidget.h"

#include "iAGEMSeConstants.h"

#include <iAChannelData.h>
#include <iAChannelSlicerData.h>
#include <iAColorTheme.h>
#include <iAConnector.h>
#include <iAConsole.h>
#include <iASlicer.h>
#include <iASlicerSettings.h>

#include <vtkDiscretizableColorTransferFunction.h>
#include <vtkImageActor.h>
#include <vtkImageData.h>
#include <vtkImageMapper3D.h>
#include <vtkTransform.h>

#include <QHBoxLayout>
#include <QSizePolicy>

namespace
{
	int GetMiddleSliceNumber(vtkImageData* imgData, iASlicerMode mode)
	{
		if (!imgData)
		{
			return 0;
		}
		int extent[6];
		imgData->GetExtent(extent);
		int baseIdx = (mode == iASlicerMode::XY) ? 5 : ((mode == iASlicerMode::XZ) ? 3 : 1);
		return (extent[baseIdx] - extent[baseIdx-1]) / 2;
	}
}

const int iAImagePreviewWidget::SliceNumberNotSet = -1;

iAImagePreviewWidget::iAImagePreviewWidget(QString const & title, QWidget* parent, bool isLabel, vtkCamera* commonCamera,
	iASlicerMode mode, int labelCount, bool magicLens):
	QWidget(parent),
	m_isLabelImage(isLabel),
	m_conn(0),
	m_empty(true),
	m_enableInteractions(commonCamera),
	m_title(title),
	m_commonCamera(commonCamera),
	m_labelCount(labelCount),
	m_sliceNumber(SliceNumberNotSet),
	m_mode(mode),
	m_aspectRatio(1.0),
	m_colorTheme(nullptr),
	m_slicerTransform(vtkTransform::New())
{
	m_slicer = new iASlicer(this, mode, false, magicLens, m_slicerTransform);
	setLayout(new QHBoxLayout);
	layout()->setSpacing(0);
	layout()->addWidget(m_slicer);
}

iAImagePreviewWidget::~iAImagePreviewWidget()
{
	delete m_slicer;
	delete m_conn;
}

void iAImagePreviewWidget::InitializeSlicer()
{
	BuildCTF();
	
	m_slicer->setup(iASingleSlicerSettings());
	m_slicer->addChannel(0, iAChannelData(m_imageData, m_ctf), true);
	m_slicer->setBackground(SLICER_BACKGROUND_COLOR[0], SLICER_BACKGROUND_COLOR[1], SLICER_BACKGROUND_COLOR[2]);
	
	// TODO: disable interaction in slicer
	// adapt initial zoom
	if (m_sliceNumber == SliceNumberNotSet)
	{
		m_sliceNumber = GetMiddleSliceNumber(m_imageData, m_slicer->mode());
	}
	m_slicer->setSliceNumber(m_sliceNumber);
	if (m_commonCamera)
	{
		m_slicer->setCamera(m_commonCamera, false);
	}
	m_slicer->update();

	if (!m_enableInteractions)
	{
		m_slicer->disableInteractor();
	}
	
	connect( m_slicer, SIGNAL(clicked(int, int, int)), this, SLOT(SlicerClicked(int, int, int)));
	connect( m_slicer, SIGNAL(rightClicked(int, int, int)), this, SLOT(SlicerRightClicked(int, int, int)));
	connect( m_slicer, SIGNAL(oslicerPos(int, int, int, int)), this, SLOT(SlicerHovered(int, int, int, int)));
	connect( m_slicer, SIGNAL(userInteraction()), this, SIGNAL(Updated()));
}

void iAImagePreviewWidget::SlicerClicked(int x, int y, int z)
{
	emit Clicked();
}

void iAImagePreviewWidget::SlicerRightClicked(int x, int y, int z)
{
	emit RightClicked();
}

void iAImagePreviewWidget::SlicerHovered(int x, int y, int z, int mode)
{
	emit MouseHover();
}

void iAImagePreviewWidget::SetSliceNumber(int sliceNr)
{
	m_sliceNumber = sliceNr;
	if (m_slicerTransform)
	{
		m_slicer->setSliceNumber(sliceNr);
	}
}

int iAImagePreviewWidget::GetSliceNumber() const
{
	return m_sliceNumber;
}

iASlicer* iAImagePreviewWidget::GetSlicer()
{
	return m_slicer;
}


bool iAImagePreviewWidget::Empty() const
{
	return m_empty;
}

void iAImagePreviewWidget::SetSlicerMode(iASlicerMode mode, int sliceNr, vtkCamera* camera)
{
	m_mode = mode;
	m_sliceNumber = sliceNr;
	if (m_slicerTransform)
	{
		m_slicer->setMode(mode);
		m_slicer->setSliceNumber(m_sliceNumber);
		m_slicer->setCamera(camera, false);
	}
}

iASlicerMode iAImagePreviewWidget::GetSlicerMode() const
{
	return m_slicer->mode();
}

vtkCamera* iAImagePreviewWidget::GetCamera()
{
	return m_slicer->getCamera();
}

void iAImagePreviewWidget::SetCamera(vtkCamera* camera)
{
	m_slicer->setCamera(camera, false);
	m_slicer->update();
}

vtkImageData * iAImagePreviewWidget::GetImage() const
{
	return m_imageData;
	//return m_slicer->GetImageData();
}

void iAImagePreviewWidget::SetImage(vtkSmartPointer<vtkImageData> img, bool empty, bool isLabelImg)
{
	m_isLabelImage = isLabelImg;
	m_empty = empty;
	m_imageData = img;
	int extent[6];
	m_imageData->GetExtent(extent);
	double w = extent[1]-extent[0]+1;
	double h = extent[3]-extent[2]+1;
	double d = extent[5]-extent[4]+1;
	switch (m_slicer->mode())
	{
		case XZ: m_aspectRatio = d / w; break;
		case YZ: m_aspectRatio = d / h; break;
		default:
		case XY: m_aspectRatio = h / w; break;
	}
	if (!m_slicerTransform)
	{
		InitializeSlicer();
	}
	UpdateImage();
}

void iAImagePreviewWidget::SetImage(iAITKIO::ImagePointer const img, bool empty, bool isLabelImg)
{
	if (!img)
	{
		DEBUG_LOG("iAImagePreviewWidget::SetImage called with NULL image!\n");
		return;
	}
	if (!m_conn)
	{
		m_conn = new iAConnector;
	}
	m_conn->SetImage(img);
	SetImage(m_conn->GetVTKImage(), empty, isLabelImg);
}

void iAImagePreviewWidget::AddNoMapperChannel(vtkSmartPointer<vtkImageData> img)
{
	if (m_addChannelImgActor)
	{
		DEBUG_LOG("Failsafe Remove Actor required");
		m_slicer->removeImageActor(m_addChannelImgActor);
	}
	m_addChannelImgActor = vtkSmartPointer<vtkImageActor>::New();
	m_addChannelImgActor->GetMapper()->BorderOn();
	m_addChannelImgActor->SetInputData(img);
	m_slicer->addImageActor(m_addChannelImgActor);
	m_slicer->update();
}

void iAImagePreviewWidget::RemoveChannel()
{
	if (m_addChannelImgActor)
	{
		m_slicer->removeImageActor(m_addChannelImgActor);
		m_slicer->update();
	}
	m_addChannelImgActor = nullptr;
}

 bool iAImagePreviewWidget::BuildCTF()
{
	// TODO: cache/reuse ctf
	if (m_empty || !m_isLabelImage)
	{
		m_ctf = vtkSmartPointer<vtkColorTransferFunction>::New();
		m_ctf->RemoveAllPoints();
		m_ctf->AddRGBPoint (m_imageData->GetScalarRange()[0], 0.0, 0.0, 0.0 );
		m_ctf->AddRGBPoint (m_imageData->GetScalarRange()[1], 1.0, 1.0, 1.0 );
		m_ctf->Build();
	}
	else
	{
		if (!m_colorTheme)
			return false;
		vtkSmartPointer<vtkDiscretizableColorTransferFunction> ctf = vtkSmartPointer<vtkDiscretizableColorTransferFunction>::New();
		assert(ctf);
		ctf->RemoveAllPoints();
		ctf->DiscretizeOn();
		ctf->SetNumberOfValues(m_labelCount+1);
		for (int i=0; i<m_labelCount; ++i)
		{
			QColor c(m_colorTheme->GetColor(i));
			ctf->AddRGBPoint(i,
				c.red()   / 255.0,
				c.green() / 255.0,
				c.blue()  / 255.0);
		}
		ctf->AddRGBPoint(m_labelCount,
			DefaultColors::DifferenceColor.red()   / 255.0,
			DefaultColors::DifferenceColor.green() / 255.0,
			DefaultColors::DifferenceColor.blue()  / 255.0
		); // for label representative -> highlights differences!
		ctf->Build();
		m_ctf = ctf;
	}
	return true;
}

void iAImagePreviewWidget::UpdateImage()
{
	if (!BuildCTF())
		return;
	m_slicer->updateChannel(0, iAChannelData(m_imageData, m_ctf));
	UpdateView();
}

void iAImagePreviewWidget::UpdateView()
{
	m_slicer->update();
}


QSize iAImagePreviewWidget::sizeHint() const
{
	return QSize(ExamplePreviewWidth, ExamplePreviewWidth * m_aspectRatio);
}


#include <QResizeEvent>

namespace
{
	const int Tolerance = 0;
}

void iAImagePreviewWidget::resizeEvent(QResizeEvent * event)
{
	/*
	QSize newSize = event->size();
	int myNewWidth  = newSize.height() / m_aspectRatio;
	int myNewHeight = newSize.width() * m_aspectRatio;
	if (newSize.width() > myNewWidth + Tolerance)
	{
		resize(myNewWidth, newSize.height());
	}
	else if (newSize.height() > myNewHeight + Tolerance)
	{
		resize(newSize.width(), myNewHeight);
	}
	*/
}


void iAImagePreviewWidget::SetColorTheme(iAColorTheme const * colorTheme)
{
	m_colorTheme = colorTheme;
	if (m_imageData)
	{
		UpdateImage();
	}
}


double iAImagePreviewWidget::GetAspectRatio() const
{
	return m_aspectRatio;
}

vtkSmartPointer<vtkColorTransferFunction> iAImagePreviewWidget::GetCTF()
{
	return m_ctf;
}
