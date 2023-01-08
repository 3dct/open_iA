/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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
#include "iANModalWidget.h"

#include "iALabelsDlg.h"
#include "iALabellingObjects.h"
#include "iANModalController.h"
#include "iANModalLabelsWidget.h"
#include "iANModalPreprocessor.h"

#include <iAChartWithFunctionsWidget.h>
#include <iADataSet.h>
#include <iAMdiChild.h>
#include <iASlicer.h>
#include <iASlicerMode.h>
#include <iATransferFunctionOwner.h>

#include <vtkImageData.h>
#include <vtkSmartPointer.h>

#include <QGridLayout>
#include <QScrollArea>
#include <QSlider>

iANModalWidget::iANModalWidget(iAMdiChild* mdiChild)
{
	m_mdiChild = mdiChild;
	m_c = new iANModalController(mdiChild);

	// Main
	QHBoxLayout* layoutMain = new QHBoxLayout(this);

	// Slicers
	QWidget* widgetSlicersGrid = new QWidget();
	m_layoutSlicersGrid = new QGridLayout(widgetSlicersGrid);

	QScrollArea* scrollArea = new QScrollArea();
	scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	//scrollArea->setWidget(widgetSlicersGrid);

	// Labels
	m_labelsWidget = new iANModalLabelsWidget();

	// Wrap up!
	layoutMain->addWidget(m_labelsWidget, 0);
	//layoutMain->addWidget(scrollArea, 1);
	layoutMain->addWidget(widgetSlicersGrid, 1);

	connect(m_labelsWidget, &iANModalLabelsWidget::labelOpacityChanged, this, &iANModalWidget::onLabelOpacityChanged);
	//connect(m_labelsWidget, &iANModalLabelsWidget::labelRemoverStateChanged, this, &iANModalWidget::onLabelRemoverStateChanged);

	connect(m_c, &iANModalController::allSlicersInitialized, this, &iANModalWidget::onAllSlicersInitialized);
	connect(m_c, &iANModalController::allSlicersReinitialized, this, &iANModalWidget::onAllSlicersReinitialized);
	connect(m_c, &iANModalController::histogramInitialized, this, &iANModalWidget::onHistogramInitialized);

	//connect(m_mdiChild->dataDockWidget(), &dlg_modalities::modalitiesChanged, this, &iANModalWidget::onModalitiesChanged);

	connect(m_c->m_dlg_labels, &iALabelsDlg::seedsAdded, this, &iANModalWidget::onSeedsAdded);
	connect(m_c->m_dlg_labels, &iALabelsDlg::seedsRemoved, this, &iANModalWidget::onSeedsRemoved);
	connect(m_c->m_dlg_labels, &iALabelsDlg::allSeedsRemoved, this, &iANModalWidget::onAllSeedsRemoved);
	connect(m_c->m_dlg_labels, &iALabelsDlg::labelAdded, this, &iANModalWidget::onLabelAdded);
	connect(m_c->m_dlg_labels, &iALabelsDlg::labelRemoved, this, &iANModalWidget::onLabelRemoved);
	connect(m_c->m_dlg_labels, &iALabelsDlg::labelsColorChanged, this, &iANModalWidget::onLabelsColorChanged);

	auto list = m_mdiChild->dataSets();
	QList<iAImageData*> dataSets;
	for (int i = 0; i < list.size(); i++)
	{
		auto imgDS = dynamic_cast<iAImageData*>(list[i].get());
		dataSets.append(imgDS);
	}

	m_preprocessor = QSharedPointer<iANModalPreprocessor>(new iANModalPreprocessor(mdiChild));
	auto output = m_preprocessor->preprocess(dataSets);

	if (!output.valid)
	{
		// TODO do not proceed
	}

	m_c->setDataSets(output.modalities);

	m_c->initialize();

	if (output.maskMode == iANModalPreprocessor::MaskMode::HIDE_ON_RENDER)
	{
		m_c->setMask(output.mask);
	}
}

void iANModalWidget::onAllSlicersInitialized()
{
	for (int i = 0; i < m_c->m_slicers.size(); i++)
	{
		auto slicer = m_c->m_slicers[i];
		auto modality = m_c->m_modalities[i];

		int column = i;

		constexpr iASlicerMode modes[iASlicerMode::SlicerCount] = {
			iASlicerMode::XY, iASlicerMode::XZ, iASlicerMode::YZ};
		for (iASlicerMode mode : modes)
		{
			auto mainSlider = m_mdiChild->slicerScrollBar(mode);
			connect(mainSlider, &QAbstractSlider::sliderPressed, [slicer, mainSlider, mode]() {
				slicer->setMode(mode);
				slicer->setSliceNumber(mainSlider->value());
			});
			connect(mainSlider, &QAbstractSlider::valueChanged, slicer, &iASlicer::setSliceNumber);
		}

		m_layoutSlicersGrid->addWidget(slicer, 0, column);
	}
}

void iANModalWidget::onAllSlicersReinitialized()
{
	while (auto slicer = m_layoutSlicersGrid->takeAt(0))
	{
		delete slicer;
	}
	onAllSlicersInitialized();
}

void iANModalWidget::onHistogramInitialized(int index)
{
	auto histogram = m_c->m_histograms[index];

	connect(histogram, &iAChartWithFunctionsWidget::updateTFTable, m_c, &iANModalController::update);

	int column = index;
	m_layoutSlicersGrid->addWidget(histogram, 1, column);
}

void iANModalWidget::onSeedsAdded(const QList<iASeed>& seeds)
{
	QHash<int, QList<iANModalSeed>> nmSeeds;
	for (auto seed : seeds)
	{
		int id = seed.label->id;
		if (!nmSeeds.contains(id))
		{
			nmSeeds.insert(id, QList<iANModalSeed>());
		}
		//auto modality = m_c->m_mapOverlayImageId2modality.value(seed.overlayImageId);
		//double scalar = modality->image()->GetScalarComponentAsDouble(seed.x, seed.y, seed.z, 0);
		nmSeeds.find(id).value().append(iANModalSeed(seed.x, seed.y, seed.z, seed.overlayImageId));
	}
	for (auto ite = nmSeeds.constBegin(); ite != nmSeeds.constEnd(); ite++)
	{
		int id = ite.key();
		if (m_labels.contains(id))
		{
			auto label = m_labels.value(id);
			auto list = ite.value();
			m_c->addSeeds(list, label);
		}
	}
	m_c->update();
}

void iANModalWidget::onSeedsRemoved(const QList<iASeed>& seeds)
{
	QList<iANModalSeed> nmSeeds;
	for (auto seed : seeds)
	{
		//auto modality = m_c->m_mapOverlayImageId2modality.value(seed.overlayImageId);
		//double scalar = modality->image()->GetScalarComponentAsDouble(seed.x, seed.y, seed.z, 0);
		nmSeeds.append(iANModalSeed(seed.x, seed.y, seed.z, seed.overlayImageId));
	}
	m_c->removeSeeds(nmSeeds);
	m_c->update();
}

void iANModalWidget::onAllSeedsRemoved()
{
	m_c->removeAllSeeds();
}

void iANModalWidget::onLabelAdded(const iALabel& label)
{
	auto nmLabel = iANModalLabel(label.id, label.name, label.color, 1.0f);
	m_labels.insert(label.id, nmLabel);
	m_labelsWidget->updateTable(m_labels.values());
}

void iANModalWidget::onLabelRemoved(const iALabel& label)
{
	//auto nmLabel = iANModalLabel(label.id, label.name, label.color, 1.0f);
	//m_c->removeSeeds(nmLabel,);
	m_labels.remove(label.id);
	m_labelsWidget->updateTable(m_labels.values());
	m_c->update();
}

void iANModalWidget::onLabelsColorChanged(const QList<iALabel>& labels)
{
	QList<iANModalLabel> nmLabels;
	for (auto label : labels)
	{
		auto ite = m_labels.find(label.id);
		ite.value().color = label.color;

		int row = m_labelsWidget->row(label.id);
		if (row >= 0)
		{
			float opacity = m_labelsWidget->opacity(row);
			nmLabels.append(iANModalLabel(label.id, label.name, label.color, opacity));
		}
	}
	m_labelsWidget->updateTable(m_labels.values());
	m_c->updateLabels(nmLabels);
	m_c->update();
}

void iANModalWidget::onLabelOpacityChanged(int labelId)
{
	if (m_labels.contains(labelId))
	{
		iANModalLabel label = m_labels.value(labelId);
		int row = m_labelsWidget->row(labelId);
		float opacity = m_labelsWidget->opacity(row);
		if (label.opacity != opacity)
		{
			label.opacity = opacity;
			//m_labels.remove(labelId);
			m_labels.insert(labelId, label);
			m_c->updateLabel(label);
		}
		m_c->update();
	}
}

void iANModalWidget::onLabelRemoverStateChanged(int labelId)
{
	if (m_labels.contains(labelId))
	{
		iANModalLabel label = m_labels.value(labelId);
		m_labels.insert(labelId, label);
		m_c->updateLabel(label);
		m_c->update();
	}
}