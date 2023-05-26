// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "iAMultimodalWidget.h"

#include "iASimpleSlicerWidget.h"

#include <iAChartFunctionTransfer.h>
#include <iAChartWithFunctionsWidget.h>
#include <iAHistogramData.h>
#include <iAPlotTypes.h>

#include <iAChannelData.h>
#include <iAChannelSlicerData.h>
#include <iADataSet.h>
#include <iADataSetRenderer.h>
#include <iALog.h>
#include <iAMdiChild.h>
//#include <iAPerformanceHelper.h>
#include <iAPreferences.h>
#include <iARenderer.h>
#include <iASlicer.h>
#include <iASlicerMode.h>   // for slicerModeString
#include <iASlicerSettings.h>
#include <iAToolsVTK.h>
#include <iATransferFunctionOwner.h>
#include <iAVolumeRenderer.h>    // for default volume settings
#include <iAVolumeViewer.h>

#include <vtkCamera.h>
#include <vtkImageActor.h>
#include <vtkImageData.h>
#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>
#include <vtkSmartPointer.h>
#include <vtkImageAppendComponents.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>
#include <vtkSmartVolumeMapper.h>
#include <vtkRenderer.h>
#include <vtkImageMapToColors.h>
#include <vtkLookupTable.h>

#include <QHBoxLayout>
#include <QLabel>
#include <QString>
#include <QSharedPointer>
#include <QStackedLayout>
#include <QMessageBox>
#include <QCheckBox>
#include <QTimer>

// Debug
#include <QDebug>

static const QString DISABLED_TEXT_COLOR = "rgb(0,0,0)"; // black
static const QString DISABLED_BACKGROUND_COLOR = "rgba(255,255,255)"; // white
static const int TIMER_UPDATE_VISUALIZATIONS_WAIT_MS = 250; // in milliseconds

iAMultimodalWidget::iAMultimodalWidget(iAMdiChild* mdiChild, NumOfMod num):
	m_mdiChild(mdiChild),
	m_histograms(num, {}),
	m_slicerWidgets(num, {}),
	m_timer_updateVisualizations(new QTimer()),
	m_slicerMode(iASlicerMode::XY),
	m_numOfDS(num),
	m_mainSlicersInitialized(false),
	m_minimumWeight(0.01),
	m_copyTFs(num, {})
{
	m_stackedLayout = new QStackedLayout(this);
	m_stackedLayout->setStackingMode(QStackedLayout::StackOne);

	m_disabledLabel = new QLabel(this);
	m_disabledLabel->setAlignment(Qt::AlignCenter);
	//m_disabledLabel->setStyleSheet("background-color: " + DISABLED_BACKGROUND_COLOR + "; color: " + DISABLED_TEXT_COLOR);

	QWidget *innerWidget = new QWidget(this);
	m_innerLayout = new QHBoxLayout(innerWidget);
	m_innerLayout->setContentsMargins(0, 0, 0, 0);

	m_stackedLayout->addWidget(innerWidget);
	m_stackedLayout->addWidget(m_disabledLabel);
	m_stackedLayout->setCurrentIndex(1);

	m_slicerModeLabel = new QLabel();
	m_sliceNumberLabel = new QLabel();
	updateLabels();

	m_checkBox_weightByOpacity = new QCheckBox("Weight by opacity");
	m_checkBox_weightByOpacity->setChecked(true);

	m_checkBox_syncedCamera = new QCheckBox("Synchronize cameras");
	m_checkBox_syncedCamera->setChecked(true);

	m_timer_updateVisualizations->setSingleShot(true);
	m_timerWait_updateVisualizations = TIMER_UPDATE_VISUALIZATIONS_WAIT_MS;

	connect(m_checkBox_weightByOpacity, &QCheckBox::stateChanged, this, &iAMultimodalWidget::checkBoxWeightByOpacityChanged);
	connect(m_checkBox_syncedCamera,    &QCheckBox::stateChanged, this, &iAMultimodalWidget::checkBoxSyncedCameraChanged);

	for (int i = 0; i < 3; ++i)
	{
		connect(mdiChild->slicer(i), &iASlicer::sliceNumberChanged, this, &iAMultimodalWidget::onMainSliceNumberChanged);
	}

	connect(mdiChild, &iAMdiChild::dataSetRendered, this, &iAMultimodalWidget::dataSetAdded);
	connect(mdiChild, &iAMdiChild::dataSetRemoved, this, &iAMultimodalWidget::dataSetRemoved);
	connect(mdiChild, &iAMdiChild::renderSettingsChanged, this, &iAMultimodalWidget::applyVolumeSettings);
	connect(mdiChild, &iAMdiChild::slicerSettingsChanged, this, &iAMultimodalWidget::applySlicerSettings);
	connect(mdiChild, &iAMdiChild::dataSetChanged, this, &iAMultimodalWidget::dataSetChangedSlot);

	connect(m_timer_updateVisualizations, &QTimer::timeout, this, &iAMultimodalWidget::onUpdateVisualizationsTimeout);

	for (auto d : m_mdiChild->dataSetMap())
	{
		dataSetAdded(d.first);
	}
}

// ----------------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------------

void iAMultimodalWidget::setSlicerMode(iASlicerMode slicerMode)
{
	if (m_slicerMode == slicerMode)
	{
		return;
	}
	disconnectMainSlicer();
	m_slicerMode = slicerMode;
	for (int i = 0; i < m_numOfDS; i++)
	{
		w_slicer(i)->setSlicerMode(slicerMode);
		w_slicer(i)->setSliceNumber(sliceNumber());
	}
	setMainSlicerCamera();
	updateLabels();
	updateVisualizationsLater();

	emit slicerModeChangedExternally(slicerMode);
}

void iAMultimodalWidget::setSliceNumber(int sliceNumber)
{
	for (int i = 0; i < m_numOfDS; i++)
	{
		w_slicer(i)->setSliceNumber(sliceNumber);
	}
	updateLabels();
	updateVisualizationsLater();

	emit sliceNumberChangedExternally(sliceNumber);
}

void iAMultimodalWidget::setWeightsProtected(iABCoord bCoord, double t)
{
	if (bCoord == m_weights)
	{
		return;
	}

	m_weights = bCoord;
	applyWeights();
	updateVisualizationsLater();
	emit weightsChanged2(t);
	emit weightsChanged3(bCoord);
}

void iAMultimodalWidget::updateVisualizationsLater()
{
	m_timer_updateVisualizations->start(m_timerWait_updateVisualizations);
}

void iAMultimodalWidget::updateVisualizationsNow()
{
	m_timer_updateVisualizations->stop();
	// TODO NEWIO: test
	//m_mdiChild->histogram()->update();
	m_mdiChild->renderer()->update();

	if (!m_mainSlicersInitialized)
	{
		return;
	}

	assert(m_numOfDS != UNDEFINED);

	//iATimeGuard test("updateMainSlicers");
	for (auto s = 0; s < iASlicerMode::SlicerCount; ++s)
	{
		auto slicer = m_mdiChild->slicer(s);

		vtkSmartPointer<vtkImageData> slicersColored[3];
		vtkSmartPointer<vtkImageData> slicerInput[3];
		vtkPiecewiseFunction* slicerOpacity[3];
		for (int dataSetIdx = 0; dataSetIdx < m_numOfDS; dataSetIdx++)
		{
			auto channel = slicer->channel(m_channelID[dataSetIdx]);
			slicer->setChannelOpacity(m_channelID[dataSetIdx], 0);

			// This changes everytime the TF changes!
			auto imgMod = channel->reslicer()->GetOutput();
			slicerInput[dataSetIdx] = imgMod;
			slicerOpacity[dataSetIdx] = channel->opacityTF();

			// Source: https://vtk.org/Wiki/VTK/Examples/Cxx/Images/ImageMapToColors
			// This changes everytime the TF changes!
			auto scalarValuesToColors = vtkSmartPointer<vtkImageMapToColors>::New(); // Will it work?
			//scalarValuesToColors->SetLookupTable(channel->m_lut);
			scalarValuesToColors->SetLookupTable(channel->colorTF());
			scalarValuesToColors->SetInputData(imgMod);
			scalarValuesToColors->Update();
			slicersColored[dataSetIdx] = scalarValuesToColors->GetOutput();
		}
		auto imgOut = m_slicerImages[s];

		// if you want to try out alternative using buffers below, start commenting out here
		auto w = getWeights();
		FOR_VTKIMG_PIXELS(imgOut, x, y, z)
		{
			float dsRGB[3][3];
			float weight[3];
			float weightSum = 0;
			for (int ds = 0; ds < m_numOfDS; ++ds)
			{
				// compute weight for this modality:
				weight[ds] = w[ds];
				if (m_checkBox_weightByOpacity->isChecked())
				{
					float intensity = slicerInput[ds]->GetScalarComponentAsFloat(x, y, z, 0);
					double opacity = slicerOpacity[ds]->GetValue(intensity);
					weight[ds] *= std::max(m_minimumWeight, opacity);

				}
				weightSum += weight[ds];
				// get color of this modality:
				for (int component = 0; component < 3; ++component)
				{
					dsRGB[ds][component] = (ds >= m_numOfDS) ? 0
						: slicersColored[ds]->GetScalarComponentAsFloat(x, y, z, component);
				}
			}
			// "normalize" weights (i.e., make their sum equal to 1):
			if (weightSum == 0)
			{
				for (int ds = 0; ds < m_numOfDS; ++ds)
				{
					weight[ds] = 1 / m_numOfDS;
				}
			}
			else
			{
				for (int ds = 0; ds < m_numOfDS; ++ds)
				{
					weight[ds] /= weightSum;
				}
			}
			// compute and set final color values:
			for (int component = 0; component < 3; ++component)
			{
				float value = 0;
				for (int ds = 0; ds < m_numOfDS; ++ds)
				{
					value += dsRGB[ds][component] * weight[ds];
				}
				imgOut->SetScalarComponentFromFloat(x, y, z, component, value);
			}
			float a = 255; // Max alpha!
			imgOut->SetScalarComponentFromFloat(x, y, z, 3, a);
		}

		// Sets the INPUT image which will be sliced again, but we have a sliced image already
		//m_mdiChild->getSlicerDataYZ()->changeImageData(imgOut);
		imgOut->Modified();
		slicer->channel(0)->imageActor()->SetInputData(imgOut);
	}

	for (int i = 0; i < 3; ++i)
	{
		m_mdiChild->slicer(i)->update();
	}
}

void iAMultimodalWidget::updateTransferFunction(int index)
{
	updateOriginalTransferFunction(index);
	w_slicer(index)->update();
	w_histogram(index)->update();
	updateVisualizationsLater();
}

// ----------------------------------------------------------------------------------
// Dataset management
// ----------------------------------------------------------------------------------

void iAMultimodalWidget::dataSetAdded(size_t dataSetIdx)
{
	if (m_dataSetsActive.size() == m_numOfDS)
	{
		return;
	}
	auto viewer = dynamic_cast<iAVolumeViewer*>(m_mdiChild->dataSetViewer(dataSetIdx));
	if (!viewer || !viewer->histogramData(0))
	{
		return;
	}
	m_dataSetsActive.push_back(dataSetIdx);
	if (m_dataSetsActive.size() == m_numOfDS)
	{
		initGUI();
	}
}

void iAMultimodalWidget::dataSetRemoved(size_t dataSetIdx)
{
	auto idx = m_dataSetsActive.indexOf(dataSetIdx);
	if (idx == -1)
	{
		return;
	}
	m_dataSetsActive.remove(idx, 1);
}

void iAMultimodalWidget::initGUI()
{
	if (m_dataSetsActive.size() < m_numOfDS)
	{
		return;
	}
	m_channelID.clear();
	for (int ds = 0; ds < m_numOfDS; ++ds)
	{
		auto viewer = dynamic_cast<iAVolumeViewer*>(m_mdiChild->dataSetViewer(m_dataSetsActive[ds]));
		if (!viewer || !viewer->histogramData(0))
		{
			LOG(lvlError, QString("DataSet %1: no viewer!").arg(ds));
			continue;
		}
		auto histData = viewer->histogramData(0);
		auto colorTF = vtkSmartPointer<vtkColorTransferFunction>::New();
		auto opacityTF = vtkSmartPointer<vtkPiecewiseFunction>::New();
		colorTF->DeepCopy(dataSetTransfer(ds)->colorTF());
		opacityTF->DeepCopy(dataSetTransfer(ds)->opacityTF());
		m_copyTFs[ds] = std::make_shared<iATransferFunctionOwner>(colorTF, opacityTF);

		m_histograms[ds] = QSharedPointer<iAChartWithFunctionsWidget>::create(nullptr, dataSetName(ds) + " gray value", "Frequency");
		auto histogramPlot = QSharedPointer<iABarGraphPlot>::create(histData, QColor(70, 70, 70, 255));
		m_histograms[ds]->addPlot(histogramPlot);
		m_histograms[ds]->setTransferFunction(m_copyTFs[ds].get());
		m_histograms[ds]->update();

		resetSlicer(ds);

		m_channelID.push_back(m_mdiChild->createChannel());
		auto chData = m_mdiChild->channelData(m_channelID[ds]);
		vtkImageData* imageData = dataSetImage(ds);
		vtkColorTransferFunction* ctf = dataSetTransfer(ds)->colorTF();
		vtkPiecewiseFunction* otf = dataSetTransfer(ds)->opacityTF();
		chData->setData(imageData, ctf, otf);
		m_mdiChild->initChannelRenderer(m_channelID[ds], false, true);

		connect((iAChartTransferFunction*)(m_histograms[ds]->functions()[0]), &iAChartTransferFunction::changed, this, [this, ds]() { updateTransferFunction(ds); });
		connect((iAChartTransferFunction*)(viewer->histogram()->functions()[0]), &iAChartTransferFunction::changed, this, [this, ds]() { originalHistogramChanged(ds); });
	}

	applyWeights();

	emit(dataSetsLoaded_beforeUpdate());

	update();

	if (!isReady())
	{
		m_disabledLabel->setText("Unable to set up this widget.\n" + m_disabledReason);
		m_stackedLayout->setCurrentIndex(1);
		return;
	}

	m_stackedLayout->setCurrentIndex(0);

	auto appendFilter = vtkSmartPointer<vtkImageAppendComponents>::New();
	appendFilter->SetInputData(dataSetImage(0));
	for (int i = 1; i < m_numOfDS; i++)
	{
		appendFilter->AddInputData(dataSetImage(i));
	}
	appendFilter->Update();

	m_combinedVol = vtkSmartPointer<vtkVolume>::New();
	auto combinedVolProp = vtkSmartPointer<vtkVolumeProperty>::New();

	for (int i = 0; i < m_numOfDS; i++)
	{
		auto transfer = dataSetTransfer(i);
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

	for (int i = 0; i < m_numOfDS; ++i)
	{
		auto renderer = dataSetRenderer(i);
		if (renderer->isVisible())
		{
			renderer->setVisible(false);
		}
	}
	m_mdiChild->renderer()->addRenderer(m_combinedVolRenderer);

	// Set up the main slicers
	for (auto s=0; s< iASlicerMode::SlicerCount; ++s)
	{
		int const *dims = m_mdiChild->slicer(s)->channel(m_channelID[0])->reslicer()->GetOutput()->GetDimensions();
		// should be double const once VTK supports it:
		double * spc = m_mdiChild->slicer(s)->channel(m_channelID[0])->reslicer()->GetOutput()->GetSpacing();

		//data->GetImageActor()->SetOpacity(0.0);
		//data->SetManualBackground(1.0, 1.0, 1.0);
		//data->SetManualBackground(0.0, 0.0, 0.0);

		auto imgOut = vtkSmartPointer<vtkImageData>::New();
		imgOut->SetDimensions(dims);
		imgOut->SetSpacing(spc);
		imgOut->AllocateScalars(VTK_UNSIGNED_CHAR, 4);
		m_slicerImages[s] = imgOut;
	}
	connectAcrossSlicers();
	setMainSlicerCamera();

	m_mainSlicersInitialized = true;
	updateVisualizationsNow();
}

void iAMultimodalWidget::applyVolumeSettings()
{
	auto volSettings = extractValues(iAVolumeRenderer::defaultAttributes());
	auto volProp = m_combinedVol->GetProperty();
	volProp->SetAmbient(volSettings[iADataSetRenderer::AmbientLighting].toDouble());
	volProp->SetDiffuse(volSettings[iADataSetRenderer::DiffuseLighting].toDouble());
	volProp->SetSpecular(volSettings[iADataSetRenderer::SpecularLighting].toDouble());
	volProp->SetSpecularPower(volSettings[iADataSetRenderer::SpecularPower].toDouble());
	volProp->SetInterpolationType(iAVolumeRenderer::string2VtkVolInterpolationType(volSettings[iAVolumeRenderer::Interpolation].toString()));
	volProp->SetShade(volSettings[iADataSetRenderer::Shading].toBool());
	auto scalarOpacityUnitDistance = volSettings[iAVolumeRenderer::ScalarOpacityUnitDistance].toDouble();
	if (scalarOpacityUnitDistance > 0)
	{
		volProp->SetScalarOpacityUnitDistance(scalarOpacityUnitDistance);
	}
	// TODO SETTINGS: make clipping plane settings a dataset setting, and make it available somehow?
	//if (m_mdiChild->renderSettings().ShowSlicers)
	//{
	//	m_combinedVolMapper->AddClippingPlane(m_mdiChild->renderer()->plane1());
	//	m_combinedVolMapper->AddClippingPlane(m_mdiChild->renderer()->plane2());
	//	m_combinedVolMapper->AddClippingPlane(m_mdiChild->renderer()->plane3());
	//}
	//else
	//{
	//	m_combinedVolMapper->RemoveAllClippingPlanes();
	//}
	m_combinedVolMapper->SetSampleDistance(volSettings[iAVolumeRenderer::SampleDistance].toDouble());
	m_combinedVolMapper->InteractiveAdjustSampleDistancesOff();
}

void iAMultimodalWidget::applySlicerSettings()
{
	for (int i = 0; i < m_numOfDS; ++i)
	{
		// TODO SETTINGS: apply changed settings?
		//m_slicerWidgets[i]->applySettings(m_mdiChild->singleSlicerSettings());
	}
}

void iAMultimodalWidget::resetSlicer(int i)
{
	// Slicer is replaced here.
	// Make sure there are no other references to the old iASimpleSlicerWidget
	// referenced by the QSharedPointer!
	m_slicerWidgets[i] = QSharedPointer<iASimpleSlicerWidget>::create(nullptr, true);
	m_slicerWidgets[i]->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
	if (i < m_dataSetsActive.size())
	{
		m_slicerWidgets[i]->changeData(dataSetImage(i), dataSetTransfer(i), dataSetName(i));
	}
}

/*
void iAMultimodalWidget::alertWeightIsZero(QString const & name)
{
	QString text =
		"The main transfer function of a modality cannot be changed "
		"while the weight of that modality is zero.\n"
		"Dataset:\n" + name +
		"\n\n"
		"To change the transfer function, use an n-modal widget instead "
		"(Double/Triple Histogram Transfer Function).";

	QMessageBox msg;
	msg.setText(text);
	msg.exec();
}
*/

void iAMultimodalWidget::originalHistogramChanged(int index)
{
	updateCopyTransferFunction(index);
	updateTransferFunction(index);
	updateVisualizationsLater();
}

void iAMultimodalWidget::updateCopyTransferFunction(int index)
{
	if (!isReady())
	{
		return;
	}
	double weight = getWeight(index);
	if (weight == 0)
	{
		updateOriginalTransferFunction(index); // Revert the changes made to the effective TF
		//alertWeightIsZero(getModality(index));
		// For now, just return silently. TODO: show alert?
		return;
	}
	auto* effective = dataSetTransfer( index);
	auto & copy = m_copyTFs[index];

	double valCol[6], valOp[4];
	copy->colorTF()->RemoveAllPoints();
	copy->opacityTF()->RemoveAllPoints();

	for (int j = 0; j < effective->colorTF()->GetSize(); ++j)
	{
		effective->colorTF()->GetNodeValue(j, valCol);
		effective->opacityTF()->GetNodeValue(j, valOp);

		if (valOp[1] > weight)
		{
			valOp[1] = weight;
		}
		double copyOp = valOp[1] / weight;

		effective->opacityTF()->SetNodeValue(j, valOp);

		copy->colorTF()->AddRGBPoint(valCol[0], valCol[1], valCol[2], valCol[3], valCol[4], valCol[5]);
		copy->opacityTF()->AddPoint(valOp[0], copyOp, valOp[2], valOp[3]);
	}
}

void iAMultimodalWidget::updateOriginalTransferFunction(int index)
{
	if (!isReady())
	{
		return;
	}

	double weight = getWeight(index);
	auto effective = dataSetTransfer(index);
	auto copy = m_copyTFs[index];

	double valCol[6], valOp[4];
	effective->colorTF()->RemoveAllPoints();
	effective->opacityTF()->RemoveAllPoints();

	for (int j = 0; j < copy->colorTF()->GetSize(); ++j)
	{
		copy->colorTF()->GetNodeValue(j, valCol);
		copy->opacityTF()->GetNodeValue(j, valOp);

		valOp[1] = valOp[1] * weight; // index 1 means opacity

		effective->colorTF()->AddRGBPoint(valCol[0], valCol[1], valCol[2], valCol[3], valCol[4], valCol[5]);
		effective->opacityTF()->AddPoint(valOp[0], valOp[1], valOp[2], valOp[3]);
	}
}

void iAMultimodalWidget::applyWeights()
{
	if (!isReady())
	{
		return;
	}
	for (int i = 0; i < m_numOfDS; i++)
	{
		vtkPiecewiseFunction *effective = dataSetTransfer(i)->opacityTF();
		vtkPiecewiseFunction *copy = m_copyTFs[i]->opacityTF();

		double pntVal[4];
		for (int j = 0; j < copy->GetSize(); ++j)
		{
			copy->GetNodeValue(j, pntVal);
			pntVal[1] = pntVal[1] * getWeight(i); // index 1 in pntVal means opacity
			effective->SetNodeValue(j, pntVal);
		}
		m_histograms[i]->update();
	}
}

void iAMultimodalWidget::setMainSlicerCamera()
{
	if (!m_checkBox_syncedCamera->isChecked())
	{
		return;
	}
	vtkCamera * mainCamera = m_mdiChild->slicer(m_slicerMode)->camera();
	for (int i = 0; i < m_numOfDS; ++i)
	{
		w_slicer(i)->setCamera(mainCamera);
		w_slicer(i)->update();
	}
	connectMainSlicer();
}

void iAMultimodalWidget::resetSlicerCamera()
{
	disconnectMainSlicer();
	for (int i = 0; i < m_numOfDS; i++)
	{
		auto cam = vtkSmartPointer<vtkCamera>::New();
		w_slicer(i)->setCamera(cam);
		w_slicer(i)->getSlicer()->resetCamera();
		w_slicer(i)->update();
	}
}

void iAMultimodalWidget::connectMainSlicer()
{
	if (!m_checkBox_syncedCamera->isChecked())
	{
		return;
	}
	iASlicer* slicer = m_mdiChild->slicer(m_slicerMode);
	for (int i = 0; i < m_numOfDS; ++i)
	{
		connect(slicer, &iASlicer::userInteraction, w_slicer(i), &iASimpleSlicerWidget::update);
		connect(w_slicer(i)->getSlicer(), &iASlicer::userInteraction, slicer, &iASlicer::update);
	}
}

void iAMultimodalWidget::disconnectMainSlicer()
{
	iASlicer* slicer = m_mdiChild->slicer(m_slicerMode);
	for (int i = 0; i < m_numOfDS; ++i)
	{
		disconnect(slicer, &iASlicer::userInteraction, w_slicer(i), &iASimpleSlicerWidget::update);
		disconnect(w_slicer(i)->getSlicer(), &iASlicer::userInteraction, slicer, &iASlicer::update);
	}
}

void iAMultimodalWidget::connectAcrossSlicers()
{
	for (int i = 0; i < m_numOfDS; ++i)
	{
		for (int j = 0; j < m_numOfDS; ++j)
		{
			if (i != j)
			{
				connect(w_slicer(i)->getSlicer(), &iASlicer::userInteraction, w_slicer(j), &iASimpleSlicerWidget::update);
			}
		}
	}
}

void iAMultimodalWidget::disconnectAcrossSlicers()
{
	for (int i = 0; i < m_numOfDS; ++i)
	{
		for (int j = 0; j < m_numOfDS; ++j)
		{
			if (i != j)
			{
				disconnect(w_slicer(i)->getSlicer(), &iASlicer::userInteraction, w_slicer(j), &iASimpleSlicerWidget::update);
			}
		}
	}
}

// ----------------------------------------------------------------------------------
// Short methods
// ----------------------------------------------------------------------------------

void iAMultimodalWidget::onUpdateVisualizationsTimeout()
{
	updateVisualizationsNow();
}

iASlicerMode iAMultimodalWidget::slicerMode() const
{
	return m_slicerMode;
}

void iAMultimodalWidget::updateLabels()
{
	m_slicerModeLabel->setText("Slicer mode: " + slicerModeString(m_slicerMode));
	m_sliceNumberLabel->setText("Slice number: " + QString::number(sliceNumber()));
}

int iAMultimodalWidget::sliceNumber() const
{
	return m_mdiChild->slicer(m_slicerMode)->sliceNumber();
}

void iAMultimodalWidget::checkBoxWeightByOpacityChanged()
{
	updateVisualizationsNow();
}

void iAMultimodalWidget::checkBoxSyncedCameraChanged()
{
	if (m_checkBox_syncedCamera->isChecked())
	{
		connectAcrossSlicers();
		setMainSlicerCamera();
	}
	else
	{
		disconnectAcrossSlicers();
		resetSlicerCamera();
	}
}

void iAMultimodalWidget::onMainSliceNumberChanged(int mode, int sliceNumber)
{
	setSlicerMode(static_cast<iASlicerMode>(mode));
	setSliceNumber(sliceNumber);
}

vtkSmartPointer<vtkImageData> iAMultimodalWidget::dataSetImage(int index)
{
	return dynamic_cast<iAImageData*>(m_mdiChild->dataSet(m_dataSetsActive[index]).get())->vtkImage();
}

QString iAMultimodalWidget::dataSetName(int index)
{
	return m_mdiChild->dataSet(m_dataSetsActive[index])->name();
}

iATransferFunction* iAMultimodalWidget::dataSetTransfer(int index)
{
	return dynamic_cast<iAVolumeViewer*>(m_mdiChild->dataSetViewer(m_dataSetsActive[index]))->transfer();
}

iADataSetRenderer* iAMultimodalWidget::dataSetRenderer(int index)
{
	return m_mdiChild->dataSetViewer(m_dataSetsActive[index])->renderer();
}

iABCoord iAMultimodalWidget::getWeights()
{
	return m_weights;
}

double iAMultimodalWidget::getWeight(int i)
{
	return m_weights[i];
}

bool iAMultimodalWidget::isReady()
{
	if (m_dataSetsActive.size() < m_numOfDS)
	{
		m_disabledReason = QString("Only %1 out of %2 required datasets loaded!").arg(m_dataSetsActive.size()).arg(m_numOfDS);
		LOG(lvlInfo, m_disabledReason);
		return false;
	}
	for (int i = 1; i < m_numOfDS; i++)
	{
		int const* dim0 = dataSetImage(0)->GetDimensions();
		int const* dimi = dataSetImage(i)->GetDimensions();
		if (dim0[0] != dimi[0] || dim0[1] != dimi[1] || dim0[2] != dimi[2])
		{
			m_disabledReason = "Dimensions of loaded images are not the same; but the "
				"transfer function widget requires them to be the same!";
			LOG(lvlInfo, m_disabledReason);
			return false;
		}
	}
	return true;
}

bool iAMultimodalWidget::containsDataSet(size_t dataSetIdx)
{
	return m_dataSetsActive.contains(dataSetIdx);
}

int iAMultimodalWidget::getDataSetCount()
{
	for (int i = m_numOfDS - 1; i >= 0; i--)
	{
		if (m_dataSetsActive[i] != iAMdiChild::NoDataSet)
		{
			return i + 1;
		}
	}
	return 0;
}

void iAMultimodalWidget::dataSetChangedSlot(size_t dataSetIdx)
{
	if (getDataSetCount() < m_numOfDS)
	{
		return;
	}
	for (int m = 0; m < m_histograms.size(); ++m)
	{
		m_histograms[m]->setXCaption(m_mdiChild->dataSet(m_dataSetsActive[m])->name() + " gray value");
	}
	dataSetChanged(dataSetIdx);
}
