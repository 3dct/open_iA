/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
#include "iASpatialView.h"

#include "iAColors.h"
#include "iAImageWidget.h"

#include "iAChannelVisualizationData.h"
#include "iAConsole.h"
#include "iAChannelID.h"
#include "iASlicer.h"
#include "iASlicerData.h"
#include "iASlicerMode.h"

//#include <QVTKOpenGLWidget.h"
#include <vtkPiecewiseFunction.h>
#include <vtkLookupTable.h>
#include <vtkImageData.h>

#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>


struct ImageData
{
	ImageData() {}
	ImageData(QString const & c, vtkImagePointer img):
		caption(c), image(img) {}
	QString caption;
	vtkImagePointer image;
};

struct ImageGUIElements
{
	ImageGUIElements() : imageWidget(nullptr), container(nullptr),
		m_selectionChannelInitialized(false) {}
	void DeleteAll()
	{
		delete container;
	}
	iAImageWidget* imageWidget;
	QWidget* container;
	bool m_selectionChannelInitialized;
};


iASpatialView::iASpatialView(): QWidget(),
	m_slice(0),
	newImgID(0)
{
	m_sliceControl = new QSpinBox();
	m_sliceControl->setMaximum(0);
	connect(m_sliceControl, SIGNAL(valueChanged(int)), this, SLOT(SliceChanged(int)));

	auto sliceButtonBar = new QToolBar();			// same order as in iASlicerMode!
	static const char* const slicerModeButtonLabels[] = { "YZ", "XY", "XZ" };
	for (int i = 0; i < 3; ++i)
	{
		slicerModeButton.push_back(new QToolButton());
		slicerModeButton[i]->setText(slicerModeButtonLabels[i]);
		slicerModeButton[i]->setAutoExclusive(true);
		slicerModeButton[i]->setCheckable(true);
		connect(slicerModeButton[i], SIGNAL(clicked(bool)), this, SLOT(SlicerModeButtonClicked(bool)));
		sliceButtonBar->addWidget(slicerModeButton[i]);
	}
	m_curMode = iASlicerMode::XY;
	slicerModeButton[m_curMode]->setChecked(true);
	
	m_sliceBar = new QWidget();
	m_sliceBar->setLayout(new QHBoxLayout());
	m_sliceBar->layout()->setSpacing(0);
	m_sliceBar->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
	m_sliceBar->layout()->addWidget(sliceButtonBar);
	m_sliceBar->layout()->addWidget(m_sliceControl);

	m_contentWidget = new QWidget();
	m_contentWidget->setLayout(new QHBoxLayout());
	m_contentWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding);
	m_contentWidget->layout()->setSpacing(0);

	m_imageBar = new QWidget();
	m_imageBar->setLayout(new QHBoxLayout());

	setLayout(new QVBoxLayout());
	layout()->addWidget(m_contentWidget);

	m_settings = new QWidget();
	m_settings->setLayout(new QVBoxLayout);
	m_settings->layout()->setSpacing(0);
	m_settings->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
	m_settings->layout()->addWidget(m_sliceBar);
	m_settings->layout()->addWidget(m_imageBar);
	layout()->addWidget(m_settings);
}


void iASpatialView::SetDatasets(QSharedPointer<iAUncertaintyImages> imgs)
{
	for (int i = 0; i < iAUncertaintyImages::SourceCount; ++i)
	{
		AddImage(imgs->GetSourceName(i), imgs->GetEntropy(i));
	}
}


QToolButton* iASpatialView::AddImage(QString const & caption, vtkImagePointer img)
{
	auto * button = new QToolButton();
	button->setText(caption);
	button->setCheckable(true);
	button->setAutoExclusive(false);
	m_imageBar->layout()->addWidget(button);
	connect(button, SIGNAL( clicked() ), this, SLOT( ImageButtonClicked() ) );
	iAImageWidget* imgW = nullptr;
	m_images.insert(newImgID, ImageData(caption, img));
	button->setProperty("imageID", newImgID);
	++newImgID;
	if (newImgID == 1)
	{
		AddImageDisplay(0);
		button->setChecked(true);
	}
	return button;
}

void InitializeChannel(ImageGUIElements & gui, QSharedPointer<iAChannelVisualizationData> selectionData)
{
	iASlicer* slicer = gui.imageWidget->GetSlicer();
	iAChannelID id = static_cast<iAChannelID>(ch_Concentration0);
	selectionData->SetName("Scatterplot Selection");
	slicer->initializeChannel(id, selectionData.data());
	int sliceNr = slicer->GetSlicerData()->getSliceNumber();
	switch (slicer->GetMode())
	{
	case YZ: slicer->enableChannel(id, true, static_cast<double>(sliceNr) * selectionData->GetImage()->GetSpacing()[0], 0, 0); break;
	case XY: slicer->enableChannel(id, true, 0, 0, static_cast<double>(sliceNr) * selectionData->GetImage()->GetSpacing()[2]); break;
	case XZ: slicer->enableChannel(id, true, 0, static_cast<double>(sliceNr) * selectionData->GetImage()->GetSpacing()[1], 0); break;
	}
	gui.m_selectionChannelInitialized = true;
}


void iASpatialView::AddImageDisplay(int idx)
{
	if (m_guiElements.contains(idx))
	{
		DEBUG_LOG(QString("Image %1 already shown!").arg(idx));
		return;
	}
	ImageGUIElements gui;
	gui.container = new QWidget();
	gui.container->setLayout(new QVBoxLayout());
	gui.imageWidget = new iAImageWidget(m_images[idx].image);
	auto label = new QLabel(m_images[idx].caption);
	label->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
	label->setAlignment(Qt::AlignHCenter);
	gui.container->layout()->addWidget(label);
	gui.container->layout()->addWidget(gui.imageWidget);
	m_contentWidget->layout()->addWidget(gui.container);
	m_sliceControl->setMaximum(gui.imageWidget->GetSliceCount()-1);
	m_guiElements.insert(idx, gui);
	if (m_selectionData && !gui.m_selectionChannelInitialized)
	{
		InitializeChannel(gui, m_selectionData);
	}
	gui.imageWidget->SetSlice(m_slice);
}


void iASpatialView::RemoveImageDisplay(int idx)
{
	m_guiElements[idx].DeleteAll();
	m_guiElements.remove(idx);
}


void iASpatialView::StyleChanged()
{
	for (int id: m_guiElements.keys())
	{
		m_guiElements[id].imageWidget->StyleChanged();
	}
}


void iASpatialView::SlicerModeButtonClicked(bool checked)
{
	int modeIdx = slicerModeButton.indexOf(qobject_cast<QToolButton*>(sender()));
	if (m_curMode == modeIdx)
	{
		return;
	}
	for (int id : m_guiElements.keys())
	{
		m_guiElements[id].imageWidget->SetMode(modeIdx);
		m_sliceControl->setMaximum(m_guiElements[id].imageWidget->GetSliceCount() - 1);
	}
	m_curMode = modeIdx;
}


void iASpatialView::ImageButtonClicked()
{
	QToolButton* button = qobject_cast<QToolButton*>(QObject::sender());
	int id = button->property("imageID").toInt();
	if (m_guiElements.contains(id))
	{	// remove image widget:
		RemoveImageDisplay(id);
	}
	else
	{
		AddImageDisplay(id);
	}
}


void iASpatialView::SliceChanged(int slice)
{
	m_slice = slice;
	for (int id : m_guiElements.keys())
	{
		m_guiElements[id].imageWidget->SetSlice(slice);
	}
}


vtkSmartPointer<vtkLookupTable> BuildLabelOverlayLUT()
{
	auto result = vtkSmartPointer<vtkLookupTable>::New();
	result->SetNumberOfTableValues(2);
	result->SetRange(0, 1);                  // alpha value here is not used!
	result->SetTableValue(0.0, 0.0, 0.0, 0.0);
	result->SetTableValue(1.0,
		Uncertainty::SelectionColor.red() / 255.0,
		Uncertainty::SelectionColor.green() / 255.0,
		Uncertainty::SelectionColor.blue() / 255.0);
	result->Build();
	return result;
}


vtkSmartPointer<vtkPiecewiseFunction> BuildLabelOverlayOTF()
{
	auto result = vtkSmartPointer<vtkPiecewiseFunction>::New();
	result->AddPoint(0.0, 0.0);
	result->AddPoint(1.0, 0.5);
	return result;
}


void iASpatialView::ShowSelection(vtkImagePointer selectionImg)
{
	if (!m_selectionData)
	{
		m_ctf = BuildLabelOverlayLUT();
		m_otf = BuildLabelOverlayOTF();
		m_selectionData = QSharedPointer<iAChannelVisualizationData>(new iAChannelVisualizationData);
		ResetChannel(m_selectionData.data(), selectionImg, m_ctf, m_otf);
	}
	for (int guiID : m_guiElements.keys())
	{
		iASlicer* slicer = m_guiElements[guiID].imageWidget->GetSlicer();
		if (!m_guiElements[guiID].m_selectionChannelInitialized)
		{
			InitializeChannel(m_guiElements[guiID], m_selectionData);
		}
		slicer->update();
	}
}


void iASpatialView::AddMemberImage(QString const & caption, vtkImagePointer img, bool keep)
{
	if (!img)
	{
		DEBUG_LOG("Image was null!");
		return;
	}
	if (!keep)
	{
		for (auto memberButton : m_memberButtons)
		{
			int idx = memberButton->property("imageID").toInt();
			RemoveImageDisplay(idx);
			m_images.remove(idx);
			delete memberButton;
		}
		m_memberButtons.clear();
	}
	auto memberButton = AddImage(caption, img);
	int idx = memberButton->property("imageID").toInt();
	memberButton->setChecked(true);
	AddImageDisplay(idx);
	m_memberButtons.push_back(memberButton);
}


void iASpatialView::ToggleSettings()
{
	m_settings->setVisible(!m_settings->isVisible());
}
