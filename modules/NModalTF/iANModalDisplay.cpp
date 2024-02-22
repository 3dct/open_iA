// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "iANModalDisplay.h"

#include <iAChannelData.h>
#include <iAImageData.h>
#include <iAMdiChild.h>
#include <iASlicerImpl.h>
#include <iASlicerMode.h>

#include <vtkCamera.h>
#include <vtkColorTransferFunction.h>
#include <vtkImageData.h>

#include <QButtonGroup>
#include <QCheckBox>
#include <QDialog>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QStatusBar>
#include <QVBoxLayout>

iANModalDisplay::iANModalDisplay(QWidget* parent, iAMdiChild* mdiChild,
	const QList<std::shared_ptr<iAImageData>>& dataSets, int maxSelection, int minSelection, int numOfRows) :
	m_dataSets(dataSets), m_maxSelection(maxSelection), m_minSelection(minSelection), m_mdiChild(mdiChild)
{
	setParent(parent);

	assert(dataSets.size() >= 0);

	numOfRows = numOfRows < 1 ? 1 : numOfRows;
	int numOfCols = std::ceil((float)dataSets.size() / (float)numOfRows);

	auto layoutMain = new QVBoxLayout(this);

	auto widgetGrid = new QWidget(this);
	auto layoutGrid = new QGridLayout(widgetGrid);

	auto group = new QButtonGroup();
	group->setExclusive(isSingleSelection());

	m_slicerMode = iASlicerMode::XY;
	for (int i = 0; i < dataSets.size(); i++)
	{
		auto ds = dataSets[i];
		auto slicer = createSlicer(ds);
		m_slicers.append(slicer);

		int col = i % numOfCols;
		int row = std::floor(i / numOfCols);
		layoutGrid->addWidget(createSlicerContainer(slicer, ds, group /*, isSingleSelection() && i == 0*/), row, col);
	}

	layoutMain->addWidget(widgetGrid);
}

iASlicer* iANModalDisplay::createSlicer(std::shared_ptr<iAImageData> dataSet)
{
	int sliceNumber = m_mdiChild->slicer(m_slicerMode)->sliceNumber();
	// Hide everything except the slice itself
	auto slicer = new iASlicerImpl(nullptr, m_slicerMode, /*bool decorations = */ false);
	slicer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

	auto image = dataSet->vtkImage();

	vtkColorTransferFunction* colorTf = vtkColorTransferFunction::New();
	double range[2];
	image->GetScalarRange(range);
	double min = range[0];
	double max = range[1];
	colorTf->AddRGBPoint(min, 0.0, 0.0, 0.0);
	colorTf->AddRGBPoint(max, 1.0, 1.0, 1.0);
	auto channelData = iAChannelData(dataSet->name(), image, colorTf);
	slicer->addChannel(MAIN_CHANNEL_ID, channelData, true);

	double* origin = image->GetOrigin();
	int* extent = image->GetExtent();
	double* spacing = image->GetSpacing();

	double xc = origin[0] + 0.5 * (extent[0] + extent[1]) * spacing[0];
	double yc = origin[1] + 0.5 * (extent[2] + extent[3]) * spacing[1];
	//double xd = (extent[1] - extent[0] + 1)*spacing[0];
	double yd = (extent[3] - extent[2] + 1) * spacing[1];

	vtkCamera* camera = slicer->camera();
	double d = camera->GetDistance();
	camera->SetParallelScale(0.5 * yd);
	camera->SetFocalPoint(xc, yc, 0.0);
	camera->SetPosition(xc, yc, +d);

	slicer->setSliceNumber(sliceNumber);

	return slicer;
}

inline QWidget* iANModalDisplay::createSlicerContainer(
	iASlicer* slicer, std::shared_ptr<iAImageData> mod, QButtonGroup* group /*, bool checked*/)
{
	//Q_UNUSED(checked);
	QWidget* widget = new QWidget(this);
	QHBoxLayout* layout = new QHBoxLayout(widget);

	widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	QAbstractButton* selectionButton;
	if (isSingleSelection())
	{
		selectionButton = new QRadioButton(widget);
	}
	else
	{
		selectionButton = new QCheckBox(widget);
	}
	selectionButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
	connect(selectionButton, &QAbstractButton::toggled, this,
		[this, mod, selectionButton]() { setModalitySelected(mod, selectionButton); });

	//selectionButton->setChecked(checked);

	group->addButton(selectionButton);

	slicer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	layout->setSpacing(0);

	layout->addWidget(selectionButton);
	layout->addWidget(slicer);

	return widget;
}

QList<std::shared_ptr<iAImageData>> iANModalDisplay::selectDataSets(
	iANModalDisplay* display, QWidget* footer, QWidget* dialogParent)
{
	// Set up dialog
	//QDialog *dialog = new QDialog(dialogParent);
	auto dialog = new SelectionDialog(display, dialogParent);
	dialog->setModal(true);

	if (!footer)
	{
		footer = createOkCancelFooter(dialog);
	}

	// Set up dialog contents
	QVBoxLayout* layout = new QVBoxLayout(dialog);
	layout->addWidget(display, 1);
	layout->addWidget(footer, 0);

	// Execute dialog and output
	auto dialogCode = dialog->exec();
	if (dialogCode == QDialog::Rejected)
	{
		return QList<std::shared_ptr<iAImageData>>();
	}
	return display->selection();
}

iANModalDisplay::Footer* iANModalDisplay::createFooter(
	QDialog* dialog, const QList<QString>& acceptText, const QList<QString>& rejectText)
{
	auto footerWigdet = new Footer(dialog);
	auto footerLabel = new QLabel(footerWigdet);
	auto footerLayout = new QHBoxLayout(footerWigdet);
	{
		footerLayout->addWidget(footerLabel);
		footerLayout->setStretchFactor(footerLabel, 1);

		for (QString text : acceptText)
		{
			auto button = new QPushButton(text);
			QObject::connect(button, &QPushButton::clicked, dialog,
				[footerWigdet, text] { footerWigdet->m_textOfButtonClicked = text; });
			QObject::connect(button, &QPushButton::clicked, dialog, &QDialog::accept);
			footerLayout->addWidget(button);
			footerLayout->setStretchFactor(button, 0);
		}

		for (QString text : rejectText)
		{
			auto button = new QPushButton(text);
			QObject::connect(button, &QPushButton::clicked, dialog,
				[footerWigdet, text] { footerWigdet->m_textOfButtonClicked = text; });
			QObject::connect(button, &QPushButton::clicked, dialog, &QDialog::reject);
			footerLayout->addWidget(button);
			footerLayout->setStretchFactor(button, 0);
		}

		footerWigdet->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
	}

	return footerWigdet;
}

void iANModalDisplay::setModalitySelected(std::shared_ptr<iAImageData> ds, QAbstractButton* button)
{
	if (isSingleSelection())
	{
		m_selectedDataSets.clear();
		m_selectedDataSets.append(ds);
	}
	else
	{
		if (button->isDown())
		{
			m_selectedDataSets.append(ds);
		}
		else
		{
			m_selectedDataSets.removeOne(ds);
		}
	}

	emit selectionChanged();
}

bool iANModalDisplay::isSelectionValid()
{
	int len = selection().size();
	return len < m_minSelection || (m_maxSelection > 0 && len > m_maxSelection);
}

bool iANModalDisplay::validateSelection()
{
	if (!isSelectionValid())
	{
		// TODO
		QString msg = "TODO";
		emit selectionRejected(msg);
		return false;
	}
	return true;
}

iANModalDisplay::SelectionDialog::SelectionDialog(iANModalDisplay* display, QWidget* parent) :
	QDialog(parent), m_display(display)
{
	display->setParent(this);
}

void iANModalDisplay::SelectionDialog::done(int r)
{
	if (r == QDialog::Accepted)
	{
		if (m_display->validateSelection())
		{
			QDialog::done(r);
		}
	}
}

uint iANModalDisplay::createChannel()
{
	uint channel = m_nextChannelId;
	m_nextChannelId++;
	return channel;
}

void iANModalDisplay::setChannelData(uint channelId, iAChannelData channelData)
{
	for (auto slicer : m_slicers)
	{
		if (slicer->hasChannel(channelId))
		{
			slicer->updateChannel(channelId, channelData);
		}
		else
		{
			slicer->addChannel(channelId, channelData, true);
		}
		slicer->update();
	}
}
