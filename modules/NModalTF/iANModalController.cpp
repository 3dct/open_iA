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
#include "iANModalController.h"

#include "iANModalBackgroundRemover.h"  // for BACKGROUND and FOREGROUND values
#include "iANModalTFManager.h"

#include "iALabelsDlg.h"
#include "iALabellingTool.h"

#include <iAChannelData.h>
#include <iAChannelSlicerData.h>
#include <iADataSet.h>
#include <iADataSetRenderer.h>
#include <iAImageDataForDisplay.h>
#include <iAMdiChild.h>
#include <iAPreferences.h>
#include <iARenderSettings.h>
#include <iARenderer.h>
#include <iASlicer.h>
#include <iASlicerImpl.h>
#include <iASlicerSettings.h>
#include <iAToolsVTK.h>
#include <iATransferFunction.h>
#include <iATypedCallHelper.h>
#include <iAVolumeSettings.h>

#include <iAChartWithFunctionsWidget.h>
#include <iAHistogramData.h>
#include <iAPlotTypes.h>

#include <iALog.h>
#include <iAPerformanceHelper.h>

#include <vtkCamera.h>
#include <vtkColorTransferFunction.h>
#include <vtkGPUVolumeRayCastMapper.h>
#include <vtkImageActor.h>
#include <vtkImageAppendComponents.h>
#include <vtkImageData.h>
#include <vtkImageReslice.h>
#include <vtkObjectFactory.h>   // for vtkStandardNewMacro
#include <vtkPiecewiseFunction.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkSmartVolumeMapper.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>

#include <array>

vtkStandardNewMacro(iANModalSmartVolumeMapper);

iANModalController::iANModalController(iAMdiChild* mdiChild) : m_mdiChild(mdiChild)
{
	// TODO NewIO:: use iATool!
	QObject* obj = m_mdiChild->findChild<QObject*>("labels");
	if (obj)
	{
		m_dlg_labels = static_cast<iALabelsDlg*>(obj);
		m_dlg_labels->removeSlicer(m_mdiChild->slicer(iASlicerMode::XY));
		m_dlg_labels->removeSlicer(m_mdiChild->slicer(iASlicerMode::XZ));
		m_dlg_labels->removeSlicer(m_mdiChild->slicer(iASlicerMode::YZ));
	}
	else
	{
		m_dlg_labels = new iALabelsDlg(mdiChild, false);
		mdiChild->splitDockWidget(mdiChild->renderDockWidget(), m_dlg_labels, Qt::Vertical);
	}
	connect(mdiChild, &iAMdiChild::dataSetRendered, this, &iANModalController::initializeHistogram);
}

void iANModalController::initialize()
{
	assert(!m_initialized);
	privateInitialize();
	emit(allSlicersInitialized());
}

void iANModalController::reinitialize()
{
	assert(m_initialized);
	privateInitialize();
	emit(allSlicersReinitialized());
}

void iANModalController::privateInitialize()
{
	assert(countDataSets() <= 4);  // VTK limit. TODO: don't hard-code

	for (auto slicer : m_slicers)
	{
		m_dlg_labels->removeSlicer(slicer);
	}

	m_slicers.clear();
	m_slicers.resize(m_dataSets.size());

	m_mapOverlayImageId2dataSet.clear();

	m_histograms.clear();
	m_histograms.resize(m_dataSets.size());

	for (int i = 0; i < m_dataSets.size(); i++)
	{
		auto dataSet = m_dataSets[i];

		auto slicer = initializeSlicer(dataSet);
		int id = m_dlg_labels->addSlicer(slicer, dataSet->name(), dataSet->vtkImage()->GetExtent(),
			dataSet->vtkImage()->GetSpacing(), m_slicerChannel_label);
		m_slicers[i] = slicer;
		m_mapOverlayImageId2dataSet.insert(id, dataSet);
		size_t dataSetIdx = m_mdiChild->dataSetIndex(dataSet.get());
		m_dataSetIdx2HistIdx.insert(dataSetIdx, i);
		if (dataSetIdx == iAMdiChild::NoDataSet)
		{
			LOG(lvlWarn, QString("Dataset %1 not found in child!").arg(dataSet->name()));
			continue;
		}
		auto viewer = dynamic_cast<iAImageDataForDisplay*>(m_mdiChild->dataSetViewer(dataSetIdx));
		if (!viewer)
		{
			LOG(lvlWarn, QString("No display data found for dataset %1!").arg(dataSet->name()));
			continue;
		}
		if (viewer->histogramData())
		{
			initializeHistogram(dataSetIdx);
		}  // no else required - histograms are automatically computed now, and connection to dataSetRendered should take care of it!
	}

	if (countDataSets() > 0)
	{
		initializeMainSlicers();
		initializeCombinedVol();
	}

	m_tracker.reinitialize(m_mdiChild);
	connect(&m_tracker, &iANModalSeedTracker::binClicked, this, &iANModalController::trackerBinClicked);

	m_initialized = true;
}

void iANModalController::initializeHistogram(size_t dataSetIdx)
{
	auto viewer = dynamic_cast<iAImageDataForDisplay*>(m_mdiChild->dataSetViewer(dataSetIdx));
	auto dataSetName = m_mdiChild->dataSet(dataSetIdx)->name();
	if (!viewer)
	{
		LOG(lvlWarn, QString("No display data found for dataset %1!").arg(dataSetName));
		return;
	}
	QSharedPointer<iAPlot> histogramPlot =
		QSharedPointer<iAPlot>(new iABarGraphPlot(viewer->histogramData(), QColor(70, 70, 70, 255)));

	auto histogram = new iAChartWithFunctionsWidget(m_mdiChild, dataSetName + " grey value", "Frequency");
	histogram->addPlot(histogramPlot);
	histogram->setTransferFunction(viewer->transfer());
	histogram->setMinimumSize(0, 100);

	if (!m_dataSetIdx2HistIdx.contains(dataSetIdx))
	{
		LOG(lvlFatal, QString("Dataset with index %1 is not contained in map to histogram indices, invalid state!").arg(dataSetIdx));
		return;
	}
	auto histIdx = m_dataSetIdx2HistIdx[dataSetIdx];
	m_histograms[histIdx] = histogram;
	emit histogramInitialized(histIdx);
}

inline iASlicer* iANModalController::initializeSlicer(std::shared_ptr<iAImageData> dataSet)
{
	auto slicerMode = iASlicerMode::XY;
	int sliceNumber = m_mdiChild->slicer(slicerMode)->sliceNumber();
	// Hide everything except the slice itself
	auto slicer = new iASlicerImpl(nullptr, slicerMode, /*bool decorations = */ false);
	slicer->setup(m_mdiChild->slicerSettings().SingleSlicer);
	slicer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

	auto image = dataSet->vtkImage();

	vtkColorTransferFunction* colorTf = vtkColorTransferFunction::New();
	double range[2];
	image->GetScalarRange(range);
	double min = range[0];
	double max = range[1];
	colorTf->AddRGBPoint(min, 0.0, 0.0, 0.0);
	colorTf->AddRGBPoint(max, 1.0, 1.0, 1.0);
	slicer->addChannel(m_slicerChannel_main, iAChannelData(dataSet->name(), image, colorTf), true);

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

inline void iANModalController::initializeMainSlicers()
{
	iASlicer* slicerArray[] = {
		m_mdiChild->slicer(iASlicerMode::YZ),
		m_mdiChild->slicer(iASlicerMode::XY),
		m_mdiChild->slicer(iASlicerMode::XZ)};

	for (uint channelId : m_channelIds)
	{
		m_mdiChild->removeChannel(channelId);
	}
	m_channelIds.clear();

	for (int dataSetIndex = 0; dataSetIndex < countDataSets(); dataSetIndex++)
	{
		uint channelId = m_mdiChild->createChannel();
		m_channelIds.append(channelId);

		auto dataSet = m_dataSets[dataSetIndex];
		auto dataSetIdx = m_mdiChild->dataSetIndex(dataSet.get());
		if (dataSetIdx == iAMdiChild::NoDataSet)
		{
			LOG(lvlWarn, QString("Dataset %1 not found in child!").arg(dataSet->name()));
			continue;
		}
		auto viewer = dynamic_cast<iAImageDataForDisplay*>(m_mdiChild->dataSetViewer(dataSetIdx));
		if (!viewer)
		{
			LOG(lvlWarn, QString("No display data found for dataset %1!").arg(dataSet->name()));
			continue;
		}
		auto chData = m_mdiChild->channelData(channelId);
		vtkImageData* imageData = dataSet->vtkImage();
		vtkColorTransferFunction* ctf = viewer->transfer()->colorTF();
		vtkPiecewiseFunction* otf = viewer->transfer()->opacityTF();
		chData->setData(imageData, ctf, otf);

		m_mdiChild->initChannelRenderer(channelId, false, true);
	}

	// Allocate a 2D image for each main slicer
	// This will be useful in the method _updateMainSlicers
	for (int mainSlicerIndex = 0; mainSlicerIndex < iASlicerMode::SlicerCount; mainSlicerIndex++)
	{
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
		m_sliceImages2D[mainSlicerIndex] = imgOut;
	}
}

inline void iANModalController::initializeCombinedVol()
{
	auto appendFilter = vtkSmartPointer<vtkImageAppendComponents>::New();
	appendFilter->SetInputData(m_dataSets[0]->vtkImage());
	for (int i = 1; i < countDataSets(); i++)
	{
		appendFilter->AddInputData(m_dataSets[i]->vtkImage());
	}
	appendFilter->Update();

	m_combinedVol = vtkSmartPointer<vtkVolume>::New();
	auto combinedVolProp = vtkSmartPointer<vtkVolumeProperty>::New();

	for (int i = 0; i < countDataSets(); i++)
	{
		auto dataSetIdx = m_mdiChild->dataSetIndex(m_dataSets[i].get());
		if (dataSetIdx == iAMdiChild::NoDataSet)
		{
			LOG(lvlWarn, QString("Dataset %1 not found in child!").arg(m_dataSets[i]->name()));
			continue;
		}
		auto viewer = dynamic_cast<iAImageDataForDisplay*>(m_mdiChild->dataSetViewer(dataSetIdx));
		if (!viewer)
		{
			LOG(lvlWarn, QString("No display data found for dataset %1!").arg(m_dataSets[i]->name()));
			continue;
		}
		auto transfer = viewer->transfer();
		combinedVolProp->SetColor(i, transfer->colorTF());
		combinedVolProp->SetScalarOpacity(i, transfer->opacityTF());
	}

	m_combinedVol->SetProperty(combinedVolProp);

	m_combinedVolMapper = vtkSmartPointer<iANModalSmartVolumeMapper>::New();
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

	for (int i = 0; i < countDataSets(); ++i)
	{
		auto dataSetIdx = m_mdiChild->dataSetIndex(m_dataSets[i].get());
		if (dataSetIdx == iAMdiChild::NoDataSet)
		{
			LOG(lvlWarn, QString("Dataset %1 not found in child!").arg(m_dataSets[i]->name()));
			continue;
		}
		auto renderer = m_mdiChild->dataSetRenderer(dataSetIdx);
		if (renderer && renderer->isVisible())
		{
			renderer->setVisible(false);
		}
	}
	m_mdiChild->renderer()->addRenderer(m_combinedVolRenderer);

	//m_mdiChild->dataDockWidget()->setAllChecked(Qt::Unchecked);
	// TODO NEWIO: hide all 3D renderers!
}

inline void iANModalController::applyVolumeSettings()
{
	auto const & vs = m_mdiChild->volumeSettings();
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
	m_combinedVolMapper->SetSampleDistance(vs.SampleDistance);
	m_combinedVolMapper->InteractiveAdjustSampleDistancesOff();
}

int iANModalController::countDataSets()
{
	// Cannot be larger than 4 because of VTK limit
	int numModalities = m_dataSets.size();
	assert(numModalities <= 4);  // Bad: '4' is hard-coded. TODO: improve
	return numModalities;
}

bool iANModalController::checkDataSets(const QList<std::shared_ptr<iAImageData>>& dataSets)
{
	if (dataSets.size() < 1 || dataSets.size() > 4)
	{  // Bad: '4' is hard-coded. TODO: improve
		return false;
	}
	/*
	for (int i = 1; i < modalities.size(); i++) {
		if (!matchModalities(modalities[0], modalities[i])) {
			return false;
		}
	}
*/
	return true;
}

/*
bool iANModalController::_matchModalities(std::shared_ptr<iAImageData> m1, std::shared_ptr<iAImageData> m2) {
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
*/

void iANModalController::setDataSets(const QList<std::shared_ptr<iAImageData>>& dataSets)
{
	if (!checkDataSets(dataSets))
	{
		return;
	}
	m_dataSets = dataSets;

	m_tfs.clear();
	for (auto dataSet : dataSets)
	{
		auto dataSetIdx = m_mdiChild->dataSetIndex(dataSet.get());
		if (dataSetIdx == iAMdiChild::NoDataSet)
		{
			LOG(lvlWarn, QString("Dataset %1 not found in child!").arg(dataSet->name()));
			continue;
		}
		auto viewer = dynamic_cast<iAImageDataForDisplay*>(m_mdiChild->dataSetViewer(dataSetIdx));
		if (!viewer)
		{
			LOG(lvlWarn, QString("No display data found for dataset %1!").arg(dataSet->name()));
			continue;
		}
		m_tfs.append(QSharedPointer<iANModalTFManager>::create(viewer->transfer()));
	}

	resetTf(dataSets);
}

void iANModalController::setMask(vtkSmartPointer<vtkImageData> mask)
{
	m_mask = vtkSmartPointer<vtkImageData>::New();
	m_mask->DeepCopy(mask);
	mask = m_mask;

	unsigned char* ptr = static_cast<unsigned char*>(mask->GetScalarPointer());
#pragma omp parallel for
	FOR_VTKIMG_PIXELS(mask, x, y, z)
	{
		int ijk[3] = {x, y, z};
		int id = mask->ComputePointId(ijk);
		ptr[id] *= 255;
	}

	auto gpuMapper = m_combinedVolMapper->getGPUMapper();
	gpuMapper->SetMaskTypeToBinary();
	gpuMapper->SetMaskInput(mask);
}

void iANModalController::resetTf(size_t dataSetIdx)
{
	auto tf = m_tfs[dataSetIdx];
	tf->tf()->opacityTF()->RemoveAllPoints();
	tf->tf()->colorTF()->RemoveAllPoints();
	tf->removeAllControlPoints();
	tf->addControlPoint(tf->minx(), {0, 0, 0, 0});
	tf->addControlPoint(tf->maxx(), {0, 0, 0, 0});
	tf->update();
}

void iANModalController::resetTf(const QList<std::shared_ptr<iAImageData>>& dataSets)
{
	for (auto dataSet : dataSets)
	{
		auto dataSetIdx = m_mdiChild->dataSetIndex(dataSet.get());
		if (dataSetIdx == iAMdiChild::NoDataSet)
		{
			LOG(lvlWarn, QString("Dataset %1 not found in child!").arg(dataSet->name()));
			continue;
		}
		resetTf(dataSetIdx);
	}
}

void iANModalController::updateLabel(const iANModalLabel& label)
{
	auto list = QList<iANModalLabel>();
	list.append(label);
	updateLabels(list);
}

void iANModalController::updateLabels(const QList<iANModalLabel>& labelsList)
{
	std::vector<iANModalLabel> labels(labelsList.begin(), labelsList.end());
	for (auto tf : m_tfs)
	{
		tf->updateLabels(labels);
		tf->update();
	}
}

void iANModalController::addSeeds(const QList<iANModalSeed>& seeds, const iANModalLabel& label)
{
	for (const auto& seed : seeds)
	{
		auto modality = m_mapOverlayImageId2dataSet.value(seed.overlayImageId);
		unsigned int x = modality->vtkImage()->GetScalarComponentAsDouble(seed.x, seed.y, seed.z, 0);
		int i = m_dataSets.lastIndexOf(modality);

		assert(m_tfs.size() > 0);

		auto tf = m_tfs[i];
		tf->addControlPoint(x, label);
	}
	for (auto tf : m_tfs)
	{
		tf->update();
	}

	m_tracker.addSeeds(seeds, label);
	m_tracker.updateLater();
}

void iANModalController::removeSeeds(const QList<iANModalSeed>& seeds)
{
	for (const auto& seed : seeds)
	{
		auto dataSet = m_mapOverlayImageId2dataSet.value(seed.overlayImageId);
		unsigned int x = dataSet->vtkImage()->GetScalarComponentAsDouble(seed.x, seed.y, seed.z, 0);
		int i = m_dataSets.lastIndexOf(dataSet);
		auto tf = m_tfs[i];
		tf->removeControlPoint(x);
	}
	for (auto tf : m_tfs)
	{
		tf->update();
	}

	m_tracker.removeSeeds(seeds);
	m_tracker.updateLater();
}

void iANModalController::removeAllSeeds()
{
	for (auto tf : m_tfs)
	{
		tf->removeAllControlPoints();
		tf->update();
	}

	m_tracker.removeAllSeeds();
	m_tracker.updateLater();
}

void iANModalController::update()
{
	m_mdiChild->renderer()->update();

	// TODO: see TODO at implementation of _updateMainSlicers
	int type = m_mdiChild->slicer(0)->channel(0)->reslicer()->GetOutput()->GetScalarType();
	VTK_TYPED_CALL(updateMainSlicers, type);

	updateHistograms();
	//m_mdiChild->redrawHistogram();
}

namespace
{
	using Rgb = std::array<unsigned char, 3>;
	using Colors = std::vector<Rgb>;
	using Alphas = std::vector<float>;
	inline void combineColors(const Colors& colors, const Alphas& opacities, Rgb& output)
	{
		assert(colors.size() == opacities.size());
		output = {0, 0, 0};
		for (size_t i = 0; i < colors.size(); ++i)
		{
			float opacity = opacities[i];
			output[0] += (colors[i][0] * opacity);
			output[1] += (colors[i][1] * opacity);
			output[2] += (colors[i][2] * opacity);
		}
	}

	constexpr int NUM_SLICERS = iASlicerMode::SlicerCount;

	constexpr iASlicerMode slicerModes[NUM_SLICERS] = {
		iASlicerMode::YZ,  // X
		iASlicerMode::XY,  // Z
		iASlicerMode::XZ   // Y
	};

	constexpr int slicerAxes[NUM_SLICERS] = {0, 2, 1};  // X, Z, Y (compatible with layout of slicerModes array)

#ifndef NDEBUG
	constexpr int slicerCoordSwapIndices[NUM_SLICERS][NUM_SLICERS] = {{2, 0, 1}, {0, 1, 2}, {0, 2, 1}};
#endif

	inline void swapIndices(const int (&xyz_orig)[3], int mainSlicerIndex, int (&xyz_out)[3])
	{
#ifdef NDEBUG
		Q_UNUSED(xyz_orig);
#endif
		xyz_out[0] = mapSliceToGlobalAxis(mainSlicerIndex, 0);
		xyz_out[1] = mapSliceToGlobalAxis(mainSlicerIndex, 1);
		xyz_out[2] = mapSliceToGlobalAxis(mainSlicerIndex, 2);

		assert(xyz_orig[slicerCoordSwapIndices[mainSlicerIndex][0]] == xyz_out[0]);
		assert(xyz_orig[slicerCoordSwapIndices[mainSlicerIndex][1]] == xyz_out[1]);
		assert(xyz_orig[slicerCoordSwapIndices[mainSlicerIndex][2]] == xyz_out[2]);
	}

	inline void convert_2d_to_3d(const int (&xyz_orig)[3], int mainSlicerIndex, int sliceNum, int (&xyz_out)[3])
	{
		swapIndices(xyz_orig, mainSlicerIndex, xyz_out);
		xyz_out[slicerAxes[mainSlicerIndex]] += sliceNum;
	}
}

#define iANModal_USE_GETSCALARPOINTER
#ifdef iANModal_USE_GETSCALARPOINTER
namespace
{
	inline void setRgba(unsigned char* ptr, const int& id, const Rgb& color, const float& alpha = 255)
	{
		ptr[id + 0] = color[0];
		ptr[id + 1] = color[1];
		ptr[id + 2] = color[2];
		ptr[id + 3] = alpha;
		//unsigned char rgba[4] = { color[0], color[1], color[2], alpha };
		//memcpy(&ptr[id], rgba, 4 * sizeof(unsigned char));
	}

	template <typename T>
	inline double getScalar(T* ptr, const int& id)
	{
		return ptr[id];
	}
}
#define iANModal_IF_USE_GETSCALARPOINTER(a) a
#define iANModal_GET_SCALAR(img, ptr) getScalar(ptr, id_scalar)
#define iANModal_SET_COLOR(img, ptr, color, alpha) setRgba(ptr, id_rgba, color, alpha)
#else
namespace
{
	inline double getScalar(const vtkSmartPointer<vtkImageData>& img, const int& x, const int& y, const int& z)
	{
		return img->GetScalarComponentAsDouble(x, y, z, 0);
	}
	inline void setRgba(const vtkSmartPointer<vtkImageData>& img, const int& x, const int& y, const int& z,
		const Rgb& color, const float& alpha = 255)
	{
		for (int i = 0; i < 3; ++i) img->SetScalarComponentFromFloat(x, y, z, i, color[i]);
		img->SetScalarComponentFromFloat(x, y, z, 3, alpha);
	}
}
#define iANModal_IF_USE_GETSCALARPOINTER(a)
#define iANModal_GET_SCALAR(img, ptr) getScalar(img, x, y, z)
#define iANModal_SET_COLOR(img, ptr, color, alpha) setRgba(img, x, y, z, color, alpha)
#endif

void iANModalController::updateHistograms()
{
	for (auto histogram : m_histograms)
	{
		histogram->update();
	}
}

// Assume that all modalities have the same pixel type
// TODO: Don't...
template <typename PixelType>
void iANModalController::updateMainSlicers()
{
	/*for (int i = 0; i < NUM_SLICERS; ++i) {
		m_mdiChild->slicer(i)->update();
	}
	return;*/

	iATimeAdder ta_color;
	iATimeAdder ta_opacity;

	const auto numDataSets = countDataSets();

	iATimeGuard testAll("Process (2D) slice images");

	for (int mainSlicerIndex = 0; mainSlicerIndex < NUM_SLICERS; ++mainSlicerIndex)
	{
		QString testSliceCaption = QString("Process (2D) slice image %1/%2")
									   .arg(QString::number(mainSlicerIndex + 1))
									   .arg(QString::number(NUM_SLICERS));
		iATimeGuard testSlice(testSliceCaption.toStdString());

		auto slicerMode = slicerModes[mainSlicerIndex];
		auto slicer = m_mdiChild->slicer(slicerMode);
		int sliceNum = slicer->sliceNumber();

		std::vector<vtkSmartPointer<vtkImageData>> sliceImgs2D(numDataSets);
		iANModal_IF_USE_GETSCALARPOINTER(std::vector<PixelType*> sliceImgs2D_ptrs(numDataSets));
		std::vector<vtkScalarsToColors*> sliceColorTf(numDataSets);
		std::vector<vtkPiecewiseFunction*> sliceOpacityTf(numDataSets);

		for (int dataSetIndex = 0; dataSetIndex < countDataSets(); ++dataSetIndex)
		{
			// Get channel for modality
			// ...this will allow us to get the 2D slice image and the transfer functions
			auto channel = slicer->channel(m_channelIds[dataSetIndex]);

			// Make modality transparent
			// TODO: find a better way... this shouldn't be necessary
			slicer->setChannelOpacity(m_channelIds[dataSetIndex], 0);

			// Get the 2D slice image
			auto sliceImg2D = channel->reslicer()->GetOutput();
#ifndef NDEBUG
			auto dim = sliceImg2D->GetDimensions();
			assert(dim[0] == 1 || dim[1] == 1 || dim[2] == 1);
#endif

			// Save 2D slice image and transfer functions for future processing
			sliceImgs2D[dataSetIndex] = sliceImg2D;
			iANModal_IF_USE_GETSCALARPOINTER(
				sliceImgs2D_ptrs[dataSetIndex] = static_cast<PixelType*>(sliceImg2D->GetScalarPointer()));
			sliceColorTf[dataSetIndex] = channel->colorTF();
			sliceOpacityTf[dataSetIndex] = channel->opacityTF();

			assert(sliceImg2D->GetNumberOfScalarComponents() == 1);
			//assert(sliceImg2D->GetScalarType() == VTK_UNSIGNED_SHORT);
		}

		testSlice.time("All info gathered");

		auto sliceImg2D_out = m_sliceImages2D[mainSlicerIndex];

#ifdef iANModal_USE_GETSCALARPOINTER
		auto ptr = static_cast<unsigned char*>(sliceImg2D_out->GetScalarPointer());
		unsigned char* maskPtr = m_mask ? static_cast<unsigned char*>(m_mask->GetScalarPointer()) : nullptr;
#ifndef NDEBUG
		int numVoxels = sliceImg2D_out->GetDimensions()[0] * sliceImg2D_out->GetDimensions()[1] *
			sliceImg2D_out->GetDimensions()[2];
#endif
#endif

		//#pragma omp parallel for
		FOR_VTKIMG_PIXELS(sliceImg2D_out, x, y, z)
		{
#ifdef iANModal_USE_GETSCALARPOINTER
			int ijk[3] = {x, y, z};
			int id_scalar = sliceImg2D_out->ComputePointId(ijk);
			int id_rgba = id_scalar * 4;

#ifndef NDEBUG
			{
				assert(id_scalar < numVoxels);
				unsigned char* ptr_test1 = &ptr[id_rgba];
				unsigned char* ptr_test2 = static_cast<unsigned char*>(sliceImg2D_out->GetScalarPointer(x, y, z));
				assert(ptr_test1 == ptr_test2);
			}
#endif
#endif

			if (m_mask)
			{
				//int ijk_3D[3] = { x + indexAddends[0], y + indexAddends[2], z + indexAddends[1] }; // X,Z,Y -> 0,2,1
				int ijk_3D[3];
				convert_2d_to_3d(ijk, mainSlicerIndex, sliceNum, ijk_3D);
				int id_scalar =
					m_mask->ComputePointId(ijk_3D);  // Shadow id_scalar from previous scope to work with macro
				unsigned char maskValue = iANModal_GET_SCALAR(m_mask, maskPtr);
#ifndef NDEBUG
				{
					unsigned char* ptr_mask_test1 = &maskPtr[id_scalar];
					unsigned char* ptr_mask_test2 = static_cast<unsigned char*>(m_mask->GetScalarPointer(ijk_3D));
					assert(ptr_mask_test1 == ptr_mask_test2);
					unsigned char maskValue_test =
						m_mask->GetScalarComponentAsDouble(ijk_3D[0], ijk_3D[1], ijk_3D[2], 0);
					assert(maskValue == maskValue_test);
				}
#endif
				if (maskValue == 0)
				{
					static constexpr Rgb black = {0, 0, 0};
					iANModal_SET_COLOR(sliceImg2D_out, ptr, black, 0);
					continue;
				}
			}

			Colors colors(numDataSets);
			Alphas opacities(numDataSets);
			float opacitySum = 0;

			// Gather the colors and opacities for this voxel xyz (for each modality)
			for (int ds_i = 0; ds_i < numDataSets; ++ds_i)
			{
#ifdef iANModal_USE_GETSCALARPOINTER
				PixelType* ptr2 = sliceImgs2D_ptrs[ds_i];

#ifndef NDEBUG
				{
					int id_scalar_test = sliceImgs2D[ds_i]->ComputePointId(ijk);
					assert(id_scalar == id_scalar_test);
					PixelType* ptr2_test1 = &ptr2[id_scalar_test];
					PixelType* ptr2_test2 = static_cast<PixelType*>(sliceImgs2D[ds_i]->GetScalarPointer(x, y, z));
					assert(ptr2_test1 == ptr2_test2);
				}
#endif

				PixelType scalar = getScalar(ptr2, id_scalar);

#ifndef NDEBUG
				{
					PixelType scalar_test = sliceImgs2D[ds_i]->GetScalarComponentAsDouble(x, y, z, 0);
					assert(scalar == scalar_test);
				}
#endif

#else
				float scalar = getScalar(sliceImgs2D[ds_i], x, y, z);
#endif

				ta_color.resume();
				const unsigned char* color = sliceColorTf[ds_i]->MapValue(scalar);  // 4 bytes (RGBA)
				ta_color.pause();

				ta_opacity.resume();
				float opacity = sliceOpacityTf[ds_i]->GetValue(scalar);
				ta_opacity.pause();

				colors[ds_i] = {color[0], color[1], color[2]};  // RGB (ignore A)
				opacities[ds_i] = opacity;
				opacitySum += opacity;
			}

			// Normalize opacities so that their sum is 1
			if (opacitySum == 0)
			{
				for (int ds_i = 0; ds_i < numDataSets; ++ds_i)
				{
					opacities[ds_i] = 1 / numDataSets;
					opacitySum = 1;
				}
			}
			else
			{
				for (int ds_i = 0; ds_i < numDataSets; ++ds_i)
				{
					opacities[ds_i] /= opacitySum;
				}
			}

			Rgb combinedColor;
			combineColors(colors, opacities, combinedColor);

			/*unsigned char aaa = index % 2 == 0 ? 255 : 127;
			switch(omp_get_thread_num()) {
			case 0: combinedColor = { aaa, 0, 0 }; break;
			case 1: combinedColor = { 0, aaa, 0 }; break;
			case 2: combinedColor = { 0, 0, aaa }; break;
			case 3: combinedColor = { aaa, aaa, 0 }; break;
			case 4: combinedColor = { aaa, 0, aaa }; break;
			case 5: combinedColor = { 0, aaa, aaa }; break;
			case 6: combinedColor = { aaa, aaa, aaa }; break;
			default: combinedColor = { 0, 0, 0 }; break;
			}*/

			iANModal_SET_COLOR(sliceImg2D_out, ptr, combinedColor, 255);

		}  // end of FOR_VTKIMG_PIXELS

		testSlice.time("All voxels processed");

		sliceImg2D_out->Modified();
		slicer->channel(0)->imageActor()->SetInputData(sliceImg2D_out);
	}

	double time_color = ta_color.elapsed();
	double time_opacity = ta_opacity.elapsed();

	QString duration_color = formatDuration(time_color);
	QString duration_opacity = formatDuration(time_opacity);
	QString duration_both = formatDuration(time_color + time_opacity);

	LOG(lvlInfo, "Accumulated time for color TF mapping: " + duration_color);
	LOG(lvlInfo, "Accumulated time for opacity TF mapping: " + duration_opacity);
	LOG(lvlInfo, "Accumulated time for color + opacity TF mapping: " + duration_both);

	testAll.time("Done!");

	for (int i = 0; i < NUM_SLICERS; ++i)
	{
		m_mdiChild->slicer(i)->update();
	}
}

void iANModalController::trackerBinClicked(iASlicerMode slicerMode, int sliceNumber)
{
	m_mdiChild->slicer(slicerMode)->setSliceNumber(sliceNumber);
	for (auto slicer : m_slicers)
	{
		slicer->setMode(slicerMode);
		slicer->setSliceNumber(sliceNumber);
	}
}
