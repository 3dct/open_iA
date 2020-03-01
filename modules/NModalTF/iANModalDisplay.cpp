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

#include "iANModalDisplay.h"

#include "mdichild.h"
#include "iASlicer.h"
#include "iASlicerMode.h"
#include "iAModality.h"
#include "iAChannelData.h"

#include <vtkImageData.h>
#include <vtkColorTransferFunction.h>
#include <vtkCamera.h>

#include <QDialog>
#include <QCheckBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QStatusBar>

iANModalDisplay::iANModalDisplay(QWidget *parent, MdiChild *mdiChild, QList<QSharedPointer<iAModality>> modalities, int maxSelection, int minSelection, int numOfRows) :
	m_mdiChild(mdiChild),
	m_maxSelection(maxSelection),
	m_minSelection(minSelection),
	m_modalities(modalities)
{
	setParent(parent); 

	assert(modalities.size() >= 0);

	numOfRows = numOfRows < 1 ? 1 : numOfRows;
	int numOfCols = ceil((float)modalities.size() / (float)numOfRows);

	auto layoutMain = new QVBoxLayout(this);

	auto widgetGrid = new QWidget(this);
	auto layoutGrid = new QGridLayout(widgetGrid);

	auto group = new QButtonGroup();
	group->setExclusive(isSingleSelection());

	m_slicerMode = iASlicerMode::XY;
	for (int i = 0; i < modalities.size(); i++) {
		auto mod = modalities[i];
		auto slicer = createSlicer(mod);
		m_slicers.append(slicer);

		int col = i % numOfCols;
		int row = floor(i / numOfCols);
		layoutGrid->addWidget(_createSlicerContainer(slicer, mod, group, isSingleSelection() && i == 0), row, col);
	}

	layoutMain->addWidget(widgetGrid);
}

iASlicer* iANModalDisplay::createSlicer(QSharedPointer<iAModality> mod) {
	int sliceNumber = m_mdiChild->slicer(m_slicerMode)->sliceNumber();
	// Hide everything except the slice itself
	auto slicer = new iASlicer(nullptr, m_slicerMode, /*bool decorations = */false);
	slicer->setup(m_mdiChild->slicerSettings().SingleSlicer);
	slicer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

	auto image = mod->image();

	vtkColorTransferFunction* colorTf = vtkColorTransferFunction::New();
	double range[2];
	image->GetScalarRange(range);
	double min = range[0];
	double max = range[1];
	colorTf->AddRGBPoint(min, 0.0, 0.0, 0.0);
	colorTf->AddRGBPoint(max, 1.0, 1.0, 1.0);
	slicer->addChannel(CHANNEL_MAIN, iAChannelData(mod->name(), image, colorTf), true);

	double* origin = image->GetOrigin();
	int* extent = image->GetExtent();
	double* spacing = image->GetSpacing();

	double xc = origin[0] + 0.5*(extent[0] + extent[1])*spacing[0];
	double yc = origin[1] + 0.5*(extent[2] + extent[3])*spacing[1];
	double xd = (extent[1] - extent[0] + 1)*spacing[0];
	double yd = (extent[3] - extent[2] + 1)*spacing[1];

	vtkCamera* camera = slicer->camera();
	double d = camera->GetDistance();
	camera->SetParallelScale(0.5 * yd);
	camera->SetFocalPoint(xc, yc, 0.0);
	camera->SetPosition(xc, yc, +d);

	slicer->setSliceNumber(sliceNumber);

	return slicer;
}

inline QWidget* iANModalDisplay::_createSlicerContainer(iASlicer* slicer, QSharedPointer<iAModality> mod, QButtonGroup* group, bool checked) {
	QWidget *widget = new QWidget(this);
	QHBoxLayout *layout = new QHBoxLayout(widget);

	widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	QAbstractButton *selectionButton;
	if (isSingleSelection()) {
		selectionButton = new QRadioButton(widget);
	} else {
		selectionButton = new QCheckBox(widget);
	}
	selectionButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
	connect(selectionButton, &QAbstractButton::toggled, this, [this, mod, selectionButton]() { setModalitySelected(mod, selectionButton); });

	selectionButton->setChecked(checked);

	group->addButton(selectionButton);

	slicer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	layout->setSpacing(0);

	layout->addWidget(selectionButton);
	layout->addWidget(slicer);

	return widget;
}

QList<QSharedPointer<iAModality>> iANModalDisplay::selectModalities(QWidget *widget, iANModalDisplay *display, int maxSelection,int minSelection, QWidget *dialogParent){
	// Set up dialog
	//QDialog *dialog = new QDialog(dialogParent);
	auto dialog = new SelectionDialog(display);
	dialog->setModal(true);

	// Set up dialog contents
	QVBoxLayout *layout = new QVBoxLayout(dialog);
	layout->addWidget(widget);

	// Execute dialog and output
	auto dialogCode = dialog->exec();
	if (dialogCode == QDialog::Rejected) {
		return QList<QSharedPointer<iAModality>>();
	}
	return display->selection();
}


void iANModalDisplay::setModalitySelected(QSharedPointer<iAModality> mod, QAbstractButton *button) {
	if (isSingleSelection()) {
		m_selectedModalities.clear();
		m_selectedModalities.append(mod);
		return;
	}

	if (button->isDown()) {
		m_selectedModalities.append(mod);
	} else {
		m_selectedModalities.removeOne(mod);
	}

	emit selectionChanged();
}

bool iANModalDisplay::isSelectionValid() {
	int len = selection().size();
	return len < m_minSelection || (m_maxSelection > 0 && len > m_maxSelection);
}

bool iANModalDisplay::validateSelection() {
	if (!isSelectionValid()) {
		// TODO
		QString msg = "TODO";
		emit selectionRejected(msg);
		return false;
	}
	return true;
}

iANModalDisplay::SelectionDialog::SelectionDialog(iANModalDisplay *display) :
	m_display(display)
{
	setParent(display);
}

void iANModalDisplay::SelectionDialog::done(int r) {
	if (r == QDialog::Accepted) {
		if (m_display->validateSelection()) {
			QDialog::done(r);
		}
	}
}