// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iASpatialView.h"

#include "iAUncertaintyColors.h"
#include "iAImageWidget.h"

#include <iAChannelData.h>
#include <iALog.h>
#include <iASlicer.h>
#include <iASlicerMode.h>
#include <iATransferFunction.h>

#include <iAQFlowLayout.h>

#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>
#include <vtkLookupTable.h>
#include <vtkImageData.h>

#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>

ImageGUIElements::ImageGUIElements() : imageWidget(nullptr), container(nullptr),
	m_selectionChannelInitialized(false)
{}

void ImageGUIElements::DeleteAll()
{
	delete container;
}

ImageData::ImageData()
{}

ImageData::ImageData(QString const & c, vtkImagePointer img):
	caption(c), image(img)
{}

iASpatialView::iASpatialView(): QWidget(),
	m_slice(0),
	newImgID(0)
{
	m_sliceControl = new QSpinBox();
	m_sliceControl->setMaximum(0);
	connect(m_sliceControl, QOverload<int>::of(&QSpinBox::valueChanged), this, &iASpatialView::SliceChanged);

	auto sliceButtonBar = new QToolBar();			// same order as in iASlicerMode!
	static const char* const slicerModeButtonLabels[] = { "YZ", "XY", "XZ" };
	for (int i = 0; i < 3; ++i)
	{
		slicerModeButton.push_back(new QToolButton());
		slicerModeButton[i]->setText(slicerModeButtonLabels[i]);
		slicerModeButton[i]->setAutoExclusive(true);
		slicerModeButton[i]->setCheckable(true);
		connect(slicerModeButton[i], &QToolButton::clicked, this, &iASpatialView::SlicerModeButtonClicked);
		sliceButtonBar->addWidget(slicerModeButton[i]);
	}
	m_curMode = iASlicerMode::XY;
	slicerModeButton[m_curMode]->setChecked(true);

	m_sliceBar = new QWidget();
	m_sliceBar->setLayout(new QHBoxLayout());
	m_sliceBar->layout()->setSpacing(0);
	m_sliceBar->layout()->setContentsMargins(0, 4, 0, 0);

	m_sliceBar->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
	m_sliceBar->layout()->addWidget(sliceButtonBar);
	m_sliceBar->layout()->addWidget(m_sliceControl);

	m_contentWidget = new QWidget();
	m_contentWidget->setLayout(new QHBoxLayout());
	m_contentWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding);
	m_contentWidget->layout()->setSpacing(4);
	m_contentWidget->layout()->setContentsMargins(0, 0, 0, 0);

	m_imageBar = new QWidget();
	m_imageBar->setLayout(new iAQFlowLayout(0, 4, 4));

	setLayout(new QVBoxLayout());
	layout()->setSpacing(0);
	layout()->setContentsMargins(4, 4, 4, 4);
	layout()->addWidget(m_contentWidget);

	m_settings = new QWidget();
	m_settings->setLayout(new QVBoxLayout);
	m_settings->layout()->setSpacing(4);
	m_settings->layout()->setContentsMargins(0, 4, 0, 0);
	m_settings->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
	m_settings->layout()->addWidget(m_sliceBar);
	m_settings->layout()->addWidget(m_imageBar);
	layout()->addWidget(m_settings);
}


void iASpatialView::SetDatasets(std::shared_ptr<iAUncertaintyImages> imgs,
		vtkSmartPointer<vtkLookupTable> labelImgLut)
{
	m_labelImgLut = labelImgLut;
	double uncertaintyRange[2] = {0.0, 1.0};
	m_uncertaintyLut = defaultColorTF(uncertaintyRange);

	newImgID = 0;
	for (auto widget : m_imageBar->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly))
	{
		delete widget;
	}
	for (auto guiIdx : m_guiElements.keys())
	{
		RemoveImageDisplay(guiIdx);
	}
	m_images.clear();
	m_memberButtons.clear();
	for (int i = 0; i < iAUncertaintyImages::SourceCount; ++i)
	{
		AddImage(imgs->GetSourceName(i), imgs->GetEntropy(i));
	}
	if (imgs->HasReference())
		AddImage("Reference", imgs->GetReference());
}


QToolButton* iASpatialView::AddImage(QString const & caption, vtkImagePointer img)
{
	auto * button = new QToolButton();
	button->setText(caption);
	button->setCheckable(true);
	button->setAutoExclusive(false);
	m_imageBar->layout()->addWidget(button);
	connect(button, &QToolButton::clicked, this, &iASpatialView::ImageButtonClicked);
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

namespace
{
	void InitializeChannel(ImageGUIElements & gui, std::shared_ptr<iAChannelData> selectionData)
	{
		iASlicer* slicer = gui.imageWidget->GetSlicer();
		const uint SelectionChannelID = 0;
		selectionData->setName("Scatterplot Selection");
		slicer->addChannel(SelectionChannelID, *selectionData.get(), true);
		gui.m_selectionChannelInitialized = true;
	}
}

void iASpatialView::AddImageDisplay(int idx)
{
	if (m_guiElements.contains(idx))
	{
		LOG(lvlWarn, QString("Image %1 already shown!").arg(idx));
		return;
	}
	ImageGUIElements gui;
	gui.container = new QWidget();
	gui.container->setLayout(new QVBoxLayout());
	gui.container->layout()->setSpacing(4);
	gui.container->layout()->setContentsMargins(0, 0, 0, 0);

	vtkScalarsToColors* colors = m_labelImgLut;
	if (m_images[idx].caption.contains("Uncertainty"))
	{
		colors = m_uncertaintyLut;
	}
	gui.imageWidget = new iAImageWidget(m_images[idx].image, colors);
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


void iASpatialView::SlicerModeButtonClicked(bool /*checked*/)
{
	auto modeIdx = slicerModeButton.indexOf(qobject_cast<QToolButton*>(sender()));
	if (m_curMode == modeIdx)
	{
		return;
	}
	m_curMode = static_cast<iASlicerMode>(modeIdx);
	for (int id : m_guiElements.keys())
	{
		m_guiElements[id].imageWidget->SetMode(m_curMode);
		m_sliceControl->setMaximum(m_guiElements[id].imageWidget->GetSliceCount() - 1);
	}
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
		iAUncertaintyColors::SelectedPixel.red() / 255.0,
		iAUncertaintyColors::SelectedPixel.green() / 255.0,
		iAUncertaintyColors::SelectedPixel.blue() / 255.0);
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


void iASpatialView::SetupSelection(vtkImagePointer selectionImg)
{
	m_ctf = BuildLabelOverlayLUT();
	m_otf = BuildLabelOverlayOTF();
	m_selectionData = std::make_shared<iAChannelData>();
	m_selectionData->setData(selectionImg, m_ctf, m_otf);
}


void iASpatialView::UpdateSelection()
{
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
		LOG(lvlError, "Image was null!");
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
