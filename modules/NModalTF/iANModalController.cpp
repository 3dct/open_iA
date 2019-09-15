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
#include "iANModalController.h"

#include "iALabellingAttachment.h"
#include "dlg_labels.h"

#include "iAModality.h"
#include "iAModalityList.h"
#include "iAModalityTransfer.h"
#include "iARenderer.h"
#include "iAVolumeRenderer.h"
#include "iASlicer.h"
#include "iAChannelSlicerData.h"
#include "iAChannelData.h"
#include "mdichild.h"

#include <vtkImageData.h>
#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>
#include <vtkImageAppendComponents.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>
#include <vtkSmartVolumeMapper.h>
#include <vtkRenderer.h>
#include <vtkCamera.h>
#include <vtkImageReslice.h>
#include <vtkSmartPointer.h>

iANModalController::iANModalController(MdiChild *mdiChild) :
	m_mdiChild(mdiChild)
{
	QObject* obj = m_mdiChild->findChild<QObject*>("labels");
	if (obj)
	{
		m_dlg_labels = static_cast<dlg_labels*>(obj);
		m_dlg_labels->removeSlicer(m_mdiChild->slicer(iASlicerMode::XY));
		m_dlg_labels->removeSlicer(m_mdiChild->slicer(iASlicerMode::XZ));
		m_dlg_labels->removeSlicer(m_mdiChild->slicer(iASlicerMode::YZ));
	}
	else
	{
		m_dlg_labels = new dlg_labels(mdiChild, false);
		mdiChild->splitDockWidget(mdiChild->logDockWidget(), m_dlg_labels, Qt::Vertical);
	}
	m_dlg_labels->setSeedsTracking(true);

	connect(mdiChild, SIGNAL(histogramAvailable()), this, SLOT(onHistogramAvailable()));
}

void iANModalController::initialize() {
	assert(!m_initialized);
	_initialize();
	emit(allSlicersInitialized());
}

void iANModalController::reinitialize() {
	assert(m_initialized);
	_initialize();
	emit(allSlicersReinitialized());
}

void iANModalController::_initialize() {

	// TODO: cherry pick modalities

	assert(countModalities() < 4); // VTK limit. TODO: don't hard-code

	for (auto slicer : m_slicers) {
		m_dlg_labels->removeSlicer(slicer);
	}

	m_slicers.clear();
	m_mapOverlayImageId2modality.clear();
	for (int i = 0; i < m_modalities.size(); i++) {
		auto modality = m_modalities[i];

		auto slicer = _initializeSlicer(modality);
		int id = m_dlg_labels->addSlicer(
			slicer,
			"noname",
			modality->image()->GetExtent(),
			modality->image()->GetSpacing(),
			m_slicerChannel_label);
		m_slicers.append(slicer);
		m_mapOverlayImageId2modality.insert(id, modality);
	}

	if (countModalities() > 0) {
		_initializeMainSlicers();
		_initializeCombinedVol();
	}

	m_initialized = true;
}

inline iASlicer* iANModalController::_initializeSlicer(QSharedPointer<iAModality> modality) {
	auto slicerMode = iASlicerMode::XY;
	int sliceNumber = m_mdiChild->slicer(slicerMode)->sliceNumber();
	// Hide everything except the slice itself
	auto slicer = new iASlicer(nullptr, slicerMode, /*bool decorations = */false);
	slicer->setup(m_mdiChild->slicerSettings().SingleSlicer);
	slicer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

	auto image = modality->image();

	vtkColorTransferFunction* colorTf = vtkColorTransferFunction::New();
	double range[2];
	image->GetScalarRange(range);
	double min = range[0];
	double max = range[1];
	colorTf->AddRGBPoint(min, 0.0, 0.0, 0.0);
	colorTf->AddRGBPoint(max, 1.0, 1.0, 1.0);
	slicer->addChannel(m_slicerChannel_main, iAChannelData(modality->name(), image, colorTf), true);

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

inline void iANModalController::_initializeMainSlicers() {
	iASlicer* slicerArray[] = {
		m_mdiChild->slicer(iASlicerMode::YZ),
		m_mdiChild->slicer(iASlicerMode::XY),
		m_mdiChild->slicer(iASlicerMode::XZ)
	};

	for (uint channelId : m_channelIds) {
		m_mdiChild->removeChannel(channelId);
	}
	m_channelIds.clear();

	for (int modalityIndex = 0; modalityIndex < countModalities(); modalityIndex++) {
		uint channelId = m_mdiChild->createChannel();
		m_channelIds.append(channelId);

		auto modality = m_modalities[modalityIndex];

		auto chData = m_mdiChild->channelData(channelId);
		vtkImageData* imageData = modality->image();
		vtkColorTransferFunction* ctf = modality->transfer()->colorTF();
		vtkPiecewiseFunction* otf = modality->transfer()->opacityTF();
		chData->setData(imageData, ctf, otf);

		m_mdiChild->initChannelRenderer(channelId, false, true);
	}

	for (int mainSlicerIndex = 0; mainSlicerIndex < iASlicerMode::SlicerCount; mainSlicerIndex++) {

		auto reslicer = slicerArray[mainSlicerIndex]->channel(m_channelIds[0])->reslicer();

		int const* dims = reslicer->GetOutput()->GetDimensions();
		// should be double const once VTK supports it:
		double* spc = reslicer->GetOutput()->GetSpacing();

		//data->GetImageActor()->SetOpacity(0.0);
		//data->SetManualBackground(1.0, 1.0, 1.0);
		//data->SetManualBackground(0.0, 0.0, 0.0);

		auto imgOut = vtkSmartPointer<vtkImageData>::New();
		imgOut->SetDimensions(dims);
		imgOut->SetSpacing(spc);
		imgOut->AllocateScalars(VTK_UNSIGNED_CHAR, 4);
		m_slicerImages[mainSlicerIndex] = imgOut;
	}
}

void iANModalController::onHistogramAvailable() {
	if (m_initialized && countModalities() > 0) {
		_initializeCombinedVol();
	}
}

inline void iANModalController::_initializeCombinedVol() {
	auto appendFilter = vtkSmartPointer<vtkImageAppendComponents>::New();
	appendFilter->SetInputData(m_modalities[0]->image());
	for (int i = 1; i < countModalities(); i++) {
		appendFilter->AddInputData(m_modalities[i]->image());
	}
	appendFilter->Update();

	m_combinedVol = vtkSmartPointer<vtkVolume>::New();
	auto combinedVolProp = vtkSmartPointer<vtkVolumeProperty>::New();

	for (int i = 0; i < countModalities(); i++) {
		auto transfer = m_modalities[i]->transfer();
		combinedVolProp->SetColor(i, transfer->colorTF());
		combinedVolProp->SetScalarOpacity(i, transfer->opacityTF());
	}

	m_combinedVol->SetProperty(combinedVolProp);

	m_combinedVolMapper = vtkSmartPointer<vtkSmartVolumeMapper>::New();
	m_combinedVolMapper->SetBlendModeToComposite();
	m_combinedVolMapper->SetInputData(appendFilter->GetOutput());
	m_combinedVolMapper->Update();
	applyVolumeSettings();
	m_combinedVol->SetMapper(m_combinedVolMapper);
	m_combinedVol->Update();

	m_combinedVolRenderer = vtkSmartPointer<vtkRenderer>::New();
	m_combinedVolRenderer->SetActiveCamera(m_mdiChild->renderer()->camera());
	m_combinedVolRenderer->GetActiveCamera()->ParallelProjectionOn();
	m_combinedVolRenderer->SetLayer(1);
	m_combinedVolRenderer->AddVolume(m_combinedVol);
	//m_combinedVolRenderer->ResetCamera();

	for (int i = 0; i < countModalities(); ++i) {
		QSharedPointer<iAVolumeRenderer> renderer = m_modalities[i]->renderer();
		if (renderer->isRendered())
			renderer->remove();
	}
	m_mdiChild->renderer()->addRenderer(m_combinedVolRenderer);
}

inline void iANModalController::applyVolumeSettings() {
	auto vs = m_mdiChild->volumeSettings();
	auto volProp = m_combinedVol->GetProperty();
	volProp->SetAmbient(vs.AmbientLighting);
	volProp->SetDiffuse(vs.DiffuseLighting);
	volProp->SetSpecular(vs.SpecularLighting);
	volProp->SetSpecularPower(vs.SpecularPower);
	volProp->SetInterpolationType(vs.LinearInterpolation);
	volProp->SetShade(vs.Shading);
	if (vs.ScalarOpacityUnitDistance > 0)
		volProp->SetScalarOpacityUnitDistance(vs.ScalarOpacityUnitDistance);
	if (m_mdiChild->renderSettings().ShowSlicers)
	{
		m_combinedVolMapper->AddClippingPlane(m_mdiChild->renderer()->plane1());
		m_combinedVolMapper->AddClippingPlane(m_mdiChild->renderer()->plane2());
		m_combinedVolMapper->AddClippingPlane(m_mdiChild->renderer()->plane3());
	}
	else
	{
		m_combinedVolMapper->RemoveAllClippingPlanes();
	}
#ifdef VTK_OPENGL2_BACKEND
	m_combinedVolMapper->SetSampleDistance(vs.SampleDistance);
	m_combinedVolMapper->InteractiveAdjustSampleDistancesOff();
#endif
}

int iANModalController::countModalities() {
	// Cannot be larger than 4 because of VTK limit
	return m_modalities.size();
}

QList<QSharedPointer<iAModality>> iANModalController::cherryPickModalities(QList<QSharedPointer<iAModality>>modalities) {

	// TODO: pick "best" combination of modalities

	QList<QSharedPointer<iAModality>> mods;
	for (auto modality : modalities) {
		mods.append(modality);
	}
	return mods;
}

bool iANModalController::_checkModalities(QList<QSharedPointer<iAModality>> modalities) {
	if (modalities.size() < 1 || modalities.size() > 4) { // Bad: '4' is hard-coded. TODO: improve
		return false;
	}

	for (int i = 1; i < modalities.size(); i++) {
		if (!_matchModalities(modalities[0], modalities[i])) {
			return false;
		}
	}

	return true;
}

bool iANModalController::_matchModalities(QSharedPointer<iAModality> m1, QSharedPointer<iAModality> m2) {
	auto image1 = m1->image();
	const int *extent1 = image1->GetExtent();
	const double *spacing1 = image1->GetSpacing();
	const double *origin1 = image1->GetOrigin();

	auto image2 = m2->image();
	const int *extent2 = image2->GetExtent();
	const double *spacing2 = image2->GetSpacing();
	const double *origin2 = image2->GetOrigin();

	return true;
}

bool iANModalController::setModalities(QList<QSharedPointer<iAModality>> modalities) {
	if (!_checkModalities(modalities)) {
		return false;
	}

	for (auto modality : m_modalities) {
		double range[2];
		modality->image()->GetScalarRange(range);
		double min = range[0];
		double max = range[1];

		auto tf = modality->transfer();

		tf->colorTF()->RemoveAllPoints();
		tf->colorTF()->AddRGBPoint(min, 0.0, 0.0, 0.0);
		tf->colorTF()->AddRGBPoint(max, 0.0, 0.0, 0.0);

		tf->opacityTF()->RemoveAllPoints();
		tf->opacityTF()->AddPoint(min, 0.0);
		tf->opacityTF()->AddPoint(max, 0.0);
	}
	resetTf();

	m_modalities = modalities;
	return true;
}

void iANModalController::resetTf() {

}

void iANModalController::updateLabel(iANModalLabel label) {
	auto list = QList<iANModalLabel>();
	list.append(label);
	updateLabels(list);
}

void iANModalController::updateLabels(QList<iANModalLabel> labelsList) {
	auto labels = QVector<iANModalLabel>(m_maxLabelId + 1);
	auto used = QVector<bool>(m_maxLabelId + 1);
	used.fill(false);
	for (auto label : labelsList) {
		int id = label.id;
		assert(id >= 0);
		if (id <= m_maxLabelId) {
			labels[id] = label;
			used[id] = true;
		}
	}
	for (auto seed : m_seeds) {
		if (used[seed.labelId]) {
			auto modality = m_mapOverlayImageId2modality.value(seed.overlayImageId);
			auto colorTf = modality->transfer()->colorTF();
			auto opacityTf = modality->transfer()->opacityTF();

			auto label = labels[seed.labelId];
			auto c = label.remover ? NMODAL_COLOR_REMOVER : label.color;
			auto o = label.remover ? NMODAL_OPACITY_REMOVER : label.opacity;

			colorTf->RemovePoint(seed.scalar);
			colorTf->AddRGBPoint(seed.scalar, c.redF(), c.greenF(), c.blueF());

			opacityTf->RemovePoint(seed.scalar);
			opacityTf->AddPoint(seed.scalar, o);
		}
	}
}

void iANModalController::addSeeds(QList<iANModalSeed> seeds, iANModalLabel label) {
	for (auto seed : seeds) {
		auto modality = m_mapOverlayImageId2modality.value(seed.overlayImageId);
		auto colorTf = modality->transfer()->colorTF();
		auto opacityTf = modality->transfer()->opacityTF();
		
		double scalar;
		auto ite = m_seeds.constFind(seed);
		if (ite != m_seeds.constEnd()) {
			colorTf->RemovePoint(seed.scalar);
			opacityTf->RemovePoint(seed.scalar);
			scalar = ite->scalar;
		} else {
			scalar = modality->image()->GetScalarComponentAsDouble(seed.x, seed.y, seed.z, 0);
		}

		seed.labelId = label.id;
		seed.scalar = scalar;

		auto c = label.remover ? NMODAL_COLOR_REMOVER : label.color;
		auto o = label.remover ? NMODAL_OPACITY_REMOVER : label.opacity;
		colorTf->AddRGBPoint(seed.scalar, c.redF(), c.greenF(), c.blueF());
		opacityTf->AddPoint(seed.scalar, o);

		m_seeds.insert(seed);
	}
	if (label.id > m_maxLabelId) {
		m_maxLabelId = label.id;
	}
}

void iANModalController::removeSeeds(QList<iANModalSeed> seeds) {
	for (auto seed : seeds) {
		if (m_seeds.remove(seed)) {
			auto modality = m_mapOverlayImageId2modality.value(seed.overlayImageId);
			auto colorTf = modality->transfer()->colorTF();
			auto opacityTf = modality->transfer()->opacityTF();
			//double scalar = modality->image()->GetScalarComponentAsDouble(seed.x, seed.y, seed.z, 0);
			colorTf->RemovePoint(seed.scalar);
			opacityTf->RemovePoint(seed.scalar);
		}
	}
}

void iANModalController::removeSeeds(int labelId) {
	for (auto seed : m_seeds) {
		if (seed.labelId == labelId) {
			auto modality = m_mapOverlayImageId2modality.value(seed.overlayImageId);
			auto colorTf = modality->transfer()->colorTF();
			auto opacityTf = modality->transfer()->opacityTF();
			colorTf->RemovePoint(seed.scalar);
			opacityTf->RemovePoint(seed.scalar);
			m_seeds.remove(seed);
		}
	}
}

void iANModalController::update() {
	m_mdiChild->renderer()->update();
	for (int i = 0; i < 3; ++i)
		m_mdiChild->slicer(i)->update();
}