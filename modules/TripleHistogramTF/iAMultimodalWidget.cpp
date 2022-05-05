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

#include "iAMultimodalWidget.h"

#include "iASimpleSlicerWidget.h"

#include <iASlicerImpl.h>    // for slicerModeToString

#include <iAChartFunctionTransfer.h>
#include <iAChartWithFunctionsWidget.h>
#include <iAHistogramData.h>
#include <iAPlotTypes.h>
#include <iAProfileWidget.h>

#include <dlg_modalities.h>
#include <iAChannelData.h>
#include <iAChannelSlicerData.h>
#include <iALog.h>
#include <iAMdiChild.h>
#include <iAModality.h>
#include <iAModalityList.h>
#include <iAModalityTransfer.h>
//#include <iAPerformanceHelper.h>
#include <iAPreferences.h>
#include <iARenderer.h>
#include <iARenderSettings.h>
#include <iASlicer.h>
#include <iASlicerMode.h>
#include <iASlicerSettings.h>
#include <iAToolsVTK.h>
#include <iATransferFunction.h>
#include <iAVolumeRenderer.h>

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
	m_timer_updateVisualizations(new QTimer()),
	m_slicerMode(iASlicerMode::XY),
	m_numOfMod(num),
	m_mainSlicersInitialized(false),
	m_minimumWeight(0.01)
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

	for (int i = 0; i < m_numOfMod; i++)
	{
		m_histograms.push_back(nullptr);
		m_slicerWidgets.push_back(nullptr);
		m_modalitiesActive.push_back(nullptr);
		m_modalitiesHistogramAvailable.push_back(false);
		m_copyTFs.push_back(nullptr);
	}

	connect(m_checkBox_weightByOpacity, &QCheckBox::stateChanged, this, &iAMultimodalWidget::checkBoxWeightByOpacityChanged);
	connect(m_checkBox_syncedCamera,    &QCheckBox::stateChanged, this, &iAMultimodalWidget::checkBoxSyncedCameraChanged);

	for (int i = 0; i < 3; ++i)
	{
		connect(mdiChild->slicer(i), &iASlicer::sliceNumberChanged, this, &iAMultimodalWidget::onMainSliceNumberChanged);
	}

	connect(mdiChild, &iAMdiChild::histogramAvailable, this, &iAMultimodalWidget::histogramAvailable);
	connect(mdiChild, &iAMdiChild::renderSettingsChanged, this, &iAMultimodalWidget::applyVolumeSettings);
	connect(mdiChild, &iAMdiChild::slicerSettingsChanged, this, &iAMultimodalWidget::applySlicerSettings);

	connect(m_mdiChild->dataDockWidget(), &dlg_modalities::modalitiesChanged, this, &iAMultimodalWidget::modalitiesChangedSlot);

	connect(m_timer_updateVisualizations, &QTimer::timeout, this, &iAMultimodalWidget::onUpdateVisualizationsTimeout);

	histogramAvailable();
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
	for (int i = 0; i < m_numOfMod; i++)
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
	for (int i = 0; i < m_numOfMod; i++)
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

	m_mdiChild->histogram()->update();
	m_mdiChild->renderer()->update();

	if (!m_mainSlicersInitialized)
	{
		return;
	}

	assert(m_numOfMod != UNDEFINED);

	//iATimeGuard test("updateMainSlicers");

	iASlicer* slicerArray[] =
	{
		m_mdiChild->slicer(iASlicerMode::YZ),
		m_mdiChild->slicer(iASlicerMode::XY),
		m_mdiChild->slicer(iASlicerMode::XZ)
	};

	for (int mainSlicerIndex = 0; mainSlicerIndex < 3; mainSlicerIndex++)
	{
		auto slicer = slicerArray[mainSlicerIndex];

		vtkSmartPointer<vtkImageData> slicersColored[3];
		vtkSmartPointer<vtkImageData> slicerInput[3];
		vtkPiecewiseFunction* slicerOpacity[3];
		for (int modalityIndex = 0; modalityIndex < m_numOfMod; modalityIndex++)
		{
			auto channel = slicer->channel(m_channelID[modalityIndex]);
			slicer->setChannelOpacity(m_channelID[modalityIndex], 0);

			// This changes everytime the TF changes!
			auto imgMod = channel->reslicer()->GetOutput();
			slicerInput[modalityIndex] = imgMod;
			slicerOpacity[modalityIndex] = channel->opacityTF();

			// Source: https://vtk.org/Wiki/VTK/Examples/Cxx/Images/ImageMapToColors
			// This changes everytime the TF changes!
			auto scalarValuesToColors = vtkSmartPointer<vtkImageMapToColors>::New(); // Will it work?
			//scalarValuesToColors->SetLookupTable(channel->m_lut);
			scalarValuesToColors->SetLookupTable(channel->colorTF());
			scalarValuesToColors->SetInputData(imgMod);
			scalarValuesToColors->Update();
			slicersColored[modalityIndex] = scalarValuesToColors->GetOutput();
		}
		auto imgOut = m_slicerImages[mainSlicerIndex];

		// if you want to try out alternative using buffers below, start commenting out here
		auto w = getWeights();
		FOR_VTKIMG_PIXELS(imgOut, x, y, z)
		{
			float modRGB[3][3];
			float weight[3];
			float weightSum = 0;
			for (int mod = 0; mod < m_numOfMod; ++mod)
			{
				// compute weight for this modality:
				weight[mod] = w[mod];
				if (m_checkBox_weightByOpacity->isChecked())
				{
					float intensity = slicerInput[mod]->GetScalarComponentAsFloat(x, y, z, 0);
					double opacity = slicerOpacity[mod]->GetValue(intensity);
					weight[mod] *= std::max(m_minimumWeight, opacity);

				}
				weightSum += weight[mod];
				// get color of this modality:
				for (int component = 0; component < 3; ++component)
				{
					modRGB[mod][component] = (mod >= m_numOfMod) ? 0
						: slicersColored[mod]->GetScalarComponentAsFloat(x, y, z, component);
					}
			}
			// "normalize" weights (i.e., make their sum equal to 1):
			if (weightSum == 0)
			{
				for (int mod = 0; mod < m_numOfMod; ++mod)
				{
					weight[mod] = 1 / m_numOfMod;
				}
			}
			else
			{
				for (int mod = 0; mod < m_numOfMod; ++mod)
				{
					weight[mod] /= weightSum;
				}
			}
			// compute and set final color values:
			for (int component = 0; component < 3; ++component)
			{
				float value = 0;
				for (int mod = 0; mod < m_numOfMod; ++mod)
				{
					value += modRGB[mod][component] * weight[mod];
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
// Modalities management
// ----------------------------------------------------------------------------------

void iAMultimodalWidget::histogramAvailable()
{
	updateModalities();

	if (!isReady())
	{
		m_disabledLabel->setText("Unable to set up this widget.\n" + m_disabledReason);
		m_stackedLayout->setCurrentIndex(1);
		return;
	}

	m_stackedLayout->setCurrentIndex(0);

	auto appendFilter = vtkSmartPointer<vtkImageAppendComponents>::New();
	appendFilter->SetInputData(getModality(0)->image());
	for (int i = 1; i < m_numOfMod; i++)
	{
		appendFilter->AddInputData(getModality(i)->image());
	}
	appendFilter->Update();

	m_combinedVol = vtkSmartPointer<vtkVolume>::New();
	auto combinedVolProp = vtkSmartPointer<vtkVolumeProperty>::New();

	for (int i = 0; i < m_numOfMod; i++)
	{
		auto transfer = getModality(i)->transfer();
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

	for (int i = 0; i < m_numOfMod; ++i)
	{
		QSharedPointer<iAVolumeRenderer> renderer = getModality(i)->renderer();
		if (renderer->isRendered())
		{
			renderer->remove();
		}
	}
	m_mdiChild->renderer()->addRenderer(m_combinedVolRenderer);

	// The next code section sets up the main slicers

	iASlicer* slicerArray[] =
	{
		m_mdiChild->slicer(iASlicerMode::YZ),
		m_mdiChild->slicer(iASlicerMode::XY),
		m_mdiChild->slicer(iASlicerMode::XZ)
	};

	for (int mainSlicerIndex = 0; mainSlicerIndex < 3; mainSlicerIndex++)
	{
		int const *dims = slicerArray[mainSlicerIndex]->channel(m_channelID[0])->reslicer()->GetOutput()->GetDimensions();
		// should be double const once VTK supports it:
		double * spc = slicerArray[mainSlicerIndex]->channel(m_channelID[0])->reslicer()->GetOutput()->GetSpacing();

		//data->GetImageActor()->SetOpacity(0.0);
		//data->SetManualBackground(1.0, 1.0, 1.0);
		//data->SetManualBackground(0.0, 0.0, 0.0);

		auto imgOut = vtkSmartPointer<vtkImageData>::New();
		imgOut->SetDimensions(dims);
		imgOut->SetSpacing(spc);
		imgOut->AllocateScalars(VTK_UNSIGNED_CHAR, 4);
		m_slicerImages[mainSlicerIndex] = imgOut;
	}
	connectAcrossSlicers();
	setMainSlicerCamera();

	m_mainSlicersInitialized = true;
	updateVisualizationsNow();
}

void iAMultimodalWidget::applyVolumeSettings()
{
	auto vs = m_mdiChild->volumeSettings();
	auto volProp = m_combinedVol->GetProperty();
	volProp->SetAmbient(vs.AmbientLighting);
	volProp->SetDiffuse(vs.DiffuseLighting);
	volProp->SetSpecular(vs.SpecularLighting);
	volProp->SetSpecularPower(vs.SpecularPower);
	volProp->SetInterpolationType(vs.LinearInterpolation);
	volProp->SetShade(vs.Shading);
	if (vs.ScalarOpacityUnitDistance > 0)
	{
		volProp->SetScalarOpacityUnitDistance(vs.ScalarOpacityUnitDistance);
	}
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

void iAMultimodalWidget::applySlicerSettings()
{
	for (int i = 0; i < m_numOfMod; ++i)
	{
		m_slicerWidgets[i]->applySettings(m_mdiChild->slicerSettings().SingleSlicer);
	}
}

// When new modalities are added/removed
void iAMultimodalWidget::updateModalities()
{
	if (m_mdiChild->modalities()->size() >= m_numOfMod)
	{
		bool allModalitiesAreHere = true;
		for (int i = 0; i < m_numOfMod; i++)
		{
			if (/*NOT*/ ! containsModality(m_mdiChild->modality(i)))
			{
				allModalitiesAreHere = false;
				break;
			}
		}
		if (allModalitiesAreHere)
		{
			return; // No need to update modalities if all of them are already here!
		}

	}
	else
	{
		int i = 0;
		for (; i < m_numOfMod && i < m_mdiChild->modalities()->size(); ++i)
		{
			m_modalitiesActive[i] = m_mdiChild->modality(i);
		}
		for (; i < m_numOfMod; i++)
		{
			m_modalitiesActive[i] = nullptr;
		}
		return;
	}

	m_channelID.clear();
	// Initialize modalities being added
	for (int i = 0; i < m_numOfMod; ++i)
	{
		m_modalitiesActive[i] = m_mdiChild->modality(i);

		// TODO: Don't duplicate code from mdichild, call it instead!
		// Histogram {
		size_t newHistBins = m_mdiChild->preferences().HistogramBins;
		if (!m_modalitiesActive[i]->histogramData() ||
			m_modalitiesActive[i]->histogramData()->valueCount() != newHistBins)
		{
			m_modalitiesActive[i]->computeImageStatistics();
			auto histData = iAHistogramData::create(
				"Frequency", m_modalitiesActive[i]->image(), newHistBins, &m_modalitiesActive[i]->info());
			m_modalitiesActive[i]->setHistogramData(histData);
		}
		m_modalitiesHistogramAvailable[i] = true;

		vtkColorTransferFunction *colorFuncCopy = vtkColorTransferFunction::New();
		vtkPiecewiseFunction *opFuncCopy = vtkPiecewiseFunction::New();
		m_copyTFs[i] = createCopyTf(i, colorFuncCopy, opFuncCopy);

		m_histograms[i] = QSharedPointer<iAChartWithFunctionsWidget>::create(nullptr, m_modalitiesActive[i]->name()+" gray value", "Frequency");
		auto histogramPlot = QSharedPointer<iABarGraphPlot>::create(m_modalitiesActive[i]->histogramData(), QColor(70, 70, 70, 255));
		m_histograms[i]->addPlot(histogramPlot);
		m_histograms[i]->setTransferFunction(m_copyTFs[i].data());
		m_histograms[i]->update();
		// }

		// Slicer {
		resetSlicer(i);
		// }

		m_channelID.push_back(m_mdiChild->createChannel());
		auto chData = m_mdiChild->channelData(m_channelID[i]);
		vtkImageData* imageData = m_modalitiesActive[i]->image();
		vtkColorTransferFunction* ctf = m_modalitiesActive[i]->transfer()->colorTF();
		vtkPiecewiseFunction* otf = m_modalitiesActive[i]->transfer()->opacityTF();
		chData->setData(imageData, ctf, otf);
		m_mdiChild->initChannelRenderer(m_channelID[i], false, true);
	}

	connect((iAChartTransferFunction*)(m_histograms[0]->functions()[0]), &iAChartTransferFunction::changed, this, &iAMultimodalWidget::updateTransferFunction1);
	connect((iAChartTransferFunction*)(m_histograms[1]->functions()[0]), &iAChartTransferFunction::changed, this, &iAMultimodalWidget::updateTransferFunction2);
	if (m_numOfMod >= THREE)
	{
		connect((iAChartTransferFunction*)(m_histograms[2]->functions()[0]), &iAChartTransferFunction::changed, this, &iAMultimodalWidget::updateTransferFunction3);
	}

	applyWeights();
	connect((iAChartTransferFunction*)(m_mdiChild->histogram()->functions()[0]), &iAChartTransferFunction::changed, this, &iAMultimodalWidget::originalHistogramChanged);

	emit(modalitiesLoaded_beforeUpdate());

	update();
}

void iAMultimodalWidget::resetSlicer(int i)
{
	// Slicer is replaced here.
	// Make sure there are no other references to the old iASimpleSlicerWidget
	// referenced by the QSharedPointer!
	m_slicerWidgets[i] = QSharedPointer<iASimpleSlicerWidget>::create(nullptr, true);
	m_slicerWidgets[i]->applySettings(m_mdiChild->slicerSettings().SingleSlicer);
	m_slicerWidgets[i]->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
	if (m_modalitiesActive[i])
	{
		m_slicerWidgets[i]->changeModality(m_modalitiesActive[i]);
	}
}

QSharedPointer<iATransferFunction> iAMultimodalWidget::createCopyTf(int index, vtkSmartPointer<vtkColorTransferFunction> colorTf, vtkSmartPointer<vtkPiecewiseFunction> opacityFunction)
{
	colorTf->DeepCopy(m_modalitiesActive[index]->transfer()->colorTF());
	opacityFunction->DeepCopy(m_modalitiesActive[index]->transfer()->opacityTF());
	return QSharedPointer<iASimpleTransferFunction>::create(colorTf, opacityFunction);
}

void iAMultimodalWidget::alertWeightIsZero(QSharedPointer<iAModality> modality)
{
	QString name = modality->fileName();
	QString text =
		"The main transfer function of a modality cannot be changed "
		"while the weight of that modality is zero.\n"
		"Modality:\n" + name +
		"\n\n"
		"To change the transfer function, use an n-modal widget instead "
		"(Double/Triple Histogram Transfer Function).";

	QMessageBox msg;
	msg.setText(text);
	msg.exec();
}

void iAMultimodalWidget::originalHistogramChanged()
{
	QSharedPointer<iAModality> selected = m_mdiChild->modality(m_mdiChild->dataDockWidget()->selected());
	for (int i = 0; i < m_numOfMod; i++)
	{
		if (selected == getModality(i))
		{
			updateCopyTransferFunction(i);
			updateTransferFunction(i);
			updateVisualizationsLater();
			return;
		}
	}
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

	// newly set transfer function (set via the histogram)
	QSharedPointer<iAModalityTransfer> effective = getModality(index)->transfer();

	// copy of previous transfer function, to be updated in this method
	QSharedPointer<iATransferFunction> copy = m_copyTFs[index];

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

	// newly set transfer function (set via the histogram)
	QSharedPointer<iAModalityTransfer> effective = m_modalitiesActive[index]->transfer();

	// copy of previous transfer function, to be updated in this method
	QSharedPointer<iATransferFunction> copy = m_copyTFs[index];

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
	for (int i = 0; i < m_numOfMod; i++)
	{
		vtkPiecewiseFunction *effective = m_modalitiesActive[i]->transfer()->opacityTF();
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
	for (int i = 0; i < m_numOfMod; ++i)
	{
		w_slicer(i)->setCamera(mainCamera);
		w_slicer(i)->update();
	}
	connectMainSlicer();
}

void iAMultimodalWidget::resetSlicerCamera()
{
	disconnectMainSlicer();
	for (int i = 0; i < m_numOfMod; i++)
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
	for (int i = 0; i < m_numOfMod; ++i)
	{
		connect(slicer, &iASlicer::userInteraction, w_slicer(i).data(), &iASimpleSlicerWidget::update);
		connect(w_slicer(i)->getSlicer(), &iASlicer::userInteraction, slicer, &iASlicer::update);
	}
}

void iAMultimodalWidget::disconnectMainSlicer()
{
	iASlicer* slicer = m_mdiChild->slicer(m_slicerMode);
	for (int i = 0; i < m_numOfMod; ++i)
	{
		disconnect(slicer, &iASlicer::userInteraction, w_slicer(i).data(), &iASimpleSlicerWidget::update);
		disconnect(w_slicer(i)->getSlicer(), &iASlicer::userInteraction, slicer, &iASlicer::update);
	}
}

void iAMultimodalWidget::connectAcrossSlicers()
{
	for (int i = 0; i < m_numOfMod; ++i)
	{
		for (int j = 0; j < m_numOfMod; ++j)
		{
			if (i != j)
			{
				connect(w_slicer(i)->getSlicer(), &iASlicer::userInteraction, w_slicer(j).data(), &iASimpleSlicerWidget::update);
			}
		}
	}
}

void iAMultimodalWidget::disconnectAcrossSlicers()
{
	for (int i = 0; i < m_numOfMod; ++i)
	{
		for (int j = 0; j < m_numOfMod; ++j)
		{
			if (i != j)
			{
				disconnect(w_slicer(i)->getSlicer(), &iASlicer::userInteraction, w_slicer(j).data(), &iASimpleSlicerWidget::update);
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

QSharedPointer<iAModality> iAMultimodalWidget::getModality(int index)
{
	return m_modalitiesActive[index];
}

vtkSmartPointer<vtkImageData> iAMultimodalWidget::getModalityImage(int index)
{
	return getModality(index)->image();
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
	for (int i = 0; i < m_numOfMod; i++)
	{
		if (!m_modalitiesActive[i] ||
			!m_modalitiesHistogramAvailable[i])
		{
			int missing = m_numOfMod - 1 - i;
			QString modalit_y_ies_is_are = missing == 1 ? "modality is" : "modalities are";
			m_disabledReason = QString::number(missing) + " " + modalit_y_ies_is_are + " missing.";
			LOG(lvlInfo, m_disabledReason);
			return false;
		}
		if (i > 0)
		{
			int const* dim0 = m_modalitiesActive[0]->image()->GetDimensions();
			int const* dimi = m_modalitiesActive[i]->image()->GetDimensions();
			if (dim0[0] != dimi[0] || dim0[1] != dimi[1] || dim0[2] != dimi[2])
			{
				m_disabledReason = "Dimensions of loaded images are not the same; but the "
					"double/triple modality transfer function widget requires them to be the same!";
				LOG(lvlInfo, m_disabledReason);
				return false;
			}
		}
	}
	return true;
}

bool iAMultimodalWidget::containsModality(QSharedPointer<iAModality> modality)
{
	for (auto mod : m_modalitiesActive)
	{
		if (mod == modality)
		{
			return true;
		}
	}
	return false;
}

int iAMultimodalWidget::getModalitiesCount()
{
	for (int i = m_numOfMod - 1; i >= 0; i--)
	{
		if (m_modalitiesActive[i])
		{
			return i + 1;
		}
	}
	return 0;
}

void iAMultimodalWidget::modalitiesChangedSlot(bool, double const *)
{
	if (getModalitiesCount() < m_numOfMod)
	{
		return;
	}
	for (int m = 0; m < m_histograms.size(); ++m)
	{
		m_histograms[m]->setXCaption(m_modalitiesActive[m]->name() + " gray value");
	}
	modalitiesChanged();
}