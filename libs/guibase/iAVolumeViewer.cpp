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
#include "iAVolumeViewer.h"

#include "iAAABB.h"
#include "iADataSet.h"
#include "iAProgress.h"
#include "iAValueTypeVectorHelpers.h"

#include "defines.h"    // for NotExistingChannel
#include "iAChannelData.h"
#include "iAChannelSlicerData.h"
#include "iADataSetListWidget.h"
#include "iADataSetRenderer.h"
#include "iADockWidgetWrapper.h"
#include "iAJobListView.h"
#include "iAMainWindow.h"
#include "iAMdiChild.h"
#include "iAPreferences.h"
#include "iARenderer.h"
#include "iARunAsync.h"
#include "iASlicer.h"
#include "iAToolsVTK.h"
#include "iATransferFunctionOwner.h"
#include "iAVolumeSettings.h"

#include "iAChartWithFunctionsWidget.h"
#include "iAChartFunctionTransfer.h"
#include "iAHistogramData.h"
#include "iAPlotTypes.h"

#include <vtkCallbackCommand.h>
#include <vtkColorTransferFunction.h>
#include <vtkImageData.h>
#include <vtkOpenGLRenderer.h>
#include <vtkPiecewiseFunction.h>
#include <vtkRenderer.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>
#include <vtkSmartVolumeMapper.h>

#include <QApplication>

namespace
{
	bool isLarge(vtkImageData* img)
	{
		int const* dim = img->GetDimensions();
		auto byteSize = mapVTKTypeToSize(img->GetScalarType());        // limit is in MB, hence the division
		return (byteSize * dim[0] * dim[1] * dim[2] / 1048576.0) >= iAMainWindow::get()->defaultPreferences().LimitForAuto3DRender;
	}

	const QString LinearInterpolation = "Linear interpolation";
	const QString ScalarOpacityUnitDistance = "Scalar Opacity Unit Distance";

	const QString RendererType = "Renderer type";
	const QString Spacing = "Spacing";
	const QString InteractiveAdjustSampleDistance = "Interactively Adjust Sample Distances";
	const QString AutoAdjustSampleDistance = "Auto-Adjust Sample Distances";
	const QString SampleDistance = "Sample distance";
	const QString InteractiveUpdateRate = "Interactive Update Rate";
	const QString FinalColorLevel = "Final Color Level";
	const QString FinalColorWindow = "Final Color Window";
	// VTK 9.2
	//const QString GlobalIlluminationReach = "Global Illumination Reach";
	//const QString VolumetricScatteringBlending = "VolumetricScatteringBlending";
}

class iAVolRenderer : public iADataSetRenderer
{
public:
	iAVolRenderer(vtkRenderer* renderer, vtkImageData* data, iAVolumeViewer* volViewer) :
		iADataSetRenderer(renderer, !isFlat(data) && !isLarge(data)),
		m_volume(vtkSmartPointer<vtkVolume>::New()),
		m_volProp(vtkSmartPointer<vtkVolumeProperty>::New()),
		m_volMapper(vtkSmartPointer<vtkSmartVolumeMapper>::New()),
		m_image(data)
	{
		assert(volViewer);
		m_volMapper->SetBlendModeToComposite();
		m_volume->SetMapper(m_volMapper);
		m_volume->SetProperty(m_volProp);
		m_volume->SetVisibility(true);
		m_volMapper->SetInputData(data);
		if (data->GetNumberOfScalarComponents() > 1)
		{
			m_volMapper->SetBlendModeToComposite();
			m_volProp->IndependentComponentsOff();
		}
		else
		{
			m_volProp->SetColor(0, volViewer->transfer()->colorTF());
		}
		m_volProp->SetScalarOpacity(0, volViewer->transfer()->opacityTF());
		m_volProp->Modified();

		// properties specific to volumes:
		auto volumeSettings = iAMainWindow::get()->defaultVolumeSettings();
		addAttribute(LinearInterpolation, iAValueType::Boolean, volumeSettings.LinearInterpolation);
		addAttribute(Shading, iAValueType::Boolean, volumeSettings.Shading);
		addAttribute(ScalarOpacityUnitDistance, iAValueType::Continuous, volumeSettings.ScalarOpacityUnitDistance);

		// mapper properties:
		QStringList renderTypes = RenderModeMap().values();
		selectOption(renderTypes, renderTypes[volumeSettings.RenderMode]);
		addAttribute(RendererType, iAValueType::Categorical, renderTypes);
		addAttribute(InteractiveAdjustSampleDistance, iAValueType::Boolean, false);
		addAttribute(AutoAdjustSampleDistance, iAValueType::Boolean, false);
		addAttribute(SampleDistance, iAValueType::Continuous, volumeSettings.SampleDistance);
		addAttribute(InteractiveUpdateRate, iAValueType::Continuous, 1.0);
		addAttribute(FinalColorLevel, iAValueType::Continuous, 0.5);
		addAttribute(FinalColorWindow, iAValueType::Continuous, 1.0);
		// -> VTK 9.2 ?
		//addAttribute(GlobalIlluminationReach, iAValueType::Continuous, 0.0, 0.0, 1.0);
		//addAttribute(VolumetricScatteringBlending, iAValueType::Continuous, -1.0, 0.0, 2.0);

		// volume properties:
		auto spc = data->GetSpacing();
		addAttribute(Spacing, iAValueType::Vector3, variantVector<double>({ spc[0], spc[1], spc[2] }));

		applyAttributes(m_attribValues);  // addAttribute adds default values; apply them now!

		// adapt bounding box to changes in position/orientation of volume:
		vtkNew<vtkCallbackCommand> modifiedCallback;
		modifiedCallback->SetCallback(
			[](vtkObject* vtkNotUsed(caller), long unsigned int vtkNotUsed(eventId), void* clientData,
				void* vtkNotUsed(callData))
			{
				reinterpret_cast<iAVolRenderer*>(clientData)->updateOutlineTransform();
			});
		modifiedCallback->SetClientData(this);
		m_volume->AddObserver(vtkCommand::ModifiedEvent, modifiedCallback);
		if (isVisible())
		{
			iAVolRenderer::showDataSet();
		}
	}
	void showDataSet() override
	{
		m_renderer->AddVolume(m_volume);
	}
	void hideDataSet() override
	{
		m_renderer->RemoveVolume(m_volume);
	}
	void applyAttributes(QVariantMap const& values) override
	{
		applyLightingProperties(m_volProp.Get(), values);
		m_volProp->SetInterpolationType(values[LinearInterpolation].toInt());
		m_volProp->SetShade(values[Shading].toBool());
		if (values[ScalarOpacityUnitDistance].toDouble() > 0)
		{
			m_volProp->SetScalarOpacityUnitDistance(values[ScalarOpacityUnitDistance].toDouble());
		}
		/*
		else
		{
			m_volSettings.ScalarOpacityUnitDistance = m_volProp->GetScalarOpacityUnitDistance();
		}
		*/
		m_volMapper->SetRequestedRenderMode(values[RendererType].toInt());
		m_volMapper->SetInteractiveAdjustSampleDistances(values[InteractiveAdjustSampleDistance].toBool());
		m_volMapper->SetAutoAdjustSampleDistances(values[AutoAdjustSampleDistance].toBool());
		m_volMapper->SetSampleDistance(values[SampleDistance].toDouble());
		m_volMapper->SetInteractiveUpdateRate(values[InteractiveUpdateRate].toDouble());
		m_volMapper->SetFinalColorLevel(values[FinalColorLevel].toDouble());
		m_volMapper->SetFinalColorWindow(values[FinalColorWindow].toDouble());
		// VTK 9.2:
		//m_volMapper->SetGlobalIlluminationReach
		//m_volMapper->SetVolumetricScatteringBlending

		QVector<double> pos = values[Position].value<QVector<double>>();
		QVector<double> ori = values[Orientation].value<QVector<double>>();
		assert(pos.size() == 3);
		assert(ori.size() == 3);
		m_volume->SetPosition(pos.data());
		m_volume->SetOrientation(ori.data());
		m_volume->SetPickable(values[Pickable].toBool());

		QVector<double> spc = values[Spacing].value<QVector<double>>();
		assert(spc.size() == 3);
		m_image->SetSpacing(spc.data());
	}
	/*
	void setMovable(bool movable) override
	{
		m_volume->SetPickable(movable);
		m_volume->SetDragable(movable);
	}
	*/
	//QWidget* controlWidget() override
	//{
	//
	//}
	iAAABB bounds() override
	{
		return iAAABB(m_image->GetBounds());
	}
	double const* orientation() const override
	{
		return m_volume->GetOrientation();
	}
	double const* position() const override
	{
		return m_volume->GetPosition();
	}
	void setPosition(double pos[3]) override
	{
		m_volume->SetPosition(pos);
	}
	void setOrientation(double ori[3]) override
	{
		m_volume->SetOrientation(ori);
	}
	vtkProp3D* vtkProp() override
	{
		return m_volume;
	}

	void setCuttingPlanes(vtkPlane* p1, vtkPlane* p2, vtkPlane* p3) override
	{
		m_volMapper->AddClippingPlane(p1);
		m_volMapper->AddClippingPlane(p2);
		m_volMapper->AddClippingPlane(p3);
	}

	void removeCuttingPlanes() override
	{
		m_volMapper->RemoveAllClippingPlanes();
	}

private:
	//iAVolumeSettings m_volSettings;

	vtkSmartPointer<vtkVolume> m_volume;
	vtkSmartPointer<vtkVolumeProperty> m_volProp;
	vtkSmartPointer<vtkSmartVolumeMapper> m_volMapper;
	vtkImageData* m_image;
	//iAChartWithFunctionsWidget* m_histogram;
};



iAVolumeViewer::iAVolumeViewer(iADataSet const* dataSet) :
	iADataSetViewer(dataSet),
	m_histogram(nullptr),
	m_imgStatistics("Computing..."),
	m_slicerChannelID(NotExistingChannel)
{
}

iAVolumeViewer::~iAVolumeViewer()
{
	removeFromSlicer();
}
	
void iAVolumeViewer::removeFromSlicer()
{
	for (int s = 0; s < 3; ++s)
	{
		m_slicer[s]->removeChannel(m_slicerChannelID);
	}
}
/*
TODO: link to some trigger in dataset list
void setSlicerVisible(bool visible) override
{
	for (int s = 0; s < 3; ++s)
	{
		m_slicer[s]->enableChannel(m_channelID, visible);
	}
}
void setPickable(bool pickable) override
{
	if (m_channelID == NotExistingChannel)
	{
		return;
	}
	for (int s = 0; s < 3; ++s)
	{
		m_slicer[s]->channel(m_channelID)->setMovable(pickable);
	}
}
unsigned int channelID() const override
{
	return m_channelID;
}
*/


void iAVolumeViewer::prepare(iAPreferences const& pref, iAProgress* p)
{
	p->setStatus(QString("%1: Computing scalar range").arg(m_dataSet->name()));
	auto img = dynamic_cast<iAImageData const*>(m_dataSet)->vtkImage();
	m_transfer = std::make_shared<iATransferFunctionOwner>(img->GetScalarRange(), img->GetNumberOfScalarComponents() == 1);
	iAImageStatistics stats;
	// TODO: handle multiple components; but for that, we need to extract the vtkImageAccumulate part,
	//       as it computes the histograms for all components at once!
	p->emitProgress(50);
	p->setStatus(QString("%1: Computing histogram and statistics.").arg(m_dataSet->name()));
	m_histogramData = iAHistogramData::create("Frequency", img, pref.HistogramBins, &stats);
	p->emitProgress(100);
	m_imgStatistics = QString("min=%1, max=%2, mean=%3, stddev=%4")
		.arg(stats.minimum)
		.arg(stats.maximum)
		.arg(stats.mean)
		.arg(stats.standardDeviation);
}

void iAVolumeViewer::createGUI(iAMdiChild* child, size_t dataSetIdx)
{
	iADataSetViewer::createGUI(child, dataSetIdx);
	// histogram
	QString histoName = "Histogram " + m_dataSet->name();
	m_histogram = new iAChartWithFunctionsWidget(child, histoName, "Frequency");
	auto histogramPlot = QSharedPointer<iABarGraphPlot>::create(
		m_histogramData,
		QApplication::palette().color(QPalette::Shadow));
	m_histogram->addPlot(histogramPlot);
	m_histogram->setTransferFunction(m_transfer.get());
	m_histogram->update();
	// TODO NEWIO:
	//     - better unique widget name!
	//     - option to put combined histograms of multiple datasets into one view / hide histograms by default
	static int histoNum = -1;
	m_histogramDW = std::make_shared<iADockWidgetWrapper>(m_histogram, histoName, QString("Histogram%1").arg(++histoNum));
	child->splitDockWidget(child->renderDockWidget(), m_histogramDW.get(), Qt::Vertical);
	m_histogramDW->hide();
	connect(iAMainWindow::get(), &iAMainWindow::styleChanged, this, [this]() {
		m_histogram->plots()[0]->setColor(QApplication::palette().color(QPalette::Shadow));
		});
	// TODO NEWIO: do we need to call what previously was iAMdiChild::changeTransferFunction ?
	//QObject::connect(m_histogram, &iAChartWithFunctionsWidget::transferFunctionChanged,
	//	child, &iAMdiChild::changeTransferFunction);

	connect(child, &iAMdiChild::preferencesChanged, this,
		[this, child]()
		{
			auto img = dynamic_cast<iAImageData const*>(m_dataSet)->vtkImage();
			size_t newBinCount = iAHistogramData::finalNumBin(img, child->preferences().HistogramBins);
			m_histogram->setYMappingMode(
				child->preferences().HistogramLogarithmicYAxis ? iAChartWidget::Logarithmic : iAChartWidget::Linear);
			if (m_histogramData->valueCount() != newBinCount)
			{
				auto fw = runAsync(
					[this, newBinCount, img]
					{
						iAImageStatistics stats;
						m_histogramData = iAHistogramData::create("Frequency", img, newBinCount, &stats);
					},
					[this]
					{
						m_histogram->clearPlots();
						auto histogramPlot = QSharedPointer<iABarGraphPlot>::create(
							m_histogramData, QApplication::palette().color(QPalette::Shadow));
						m_histogram->addPlot(histogramPlot);
						m_histogram->update();
					},
					this);
				iAJobListView::get()->addJob(
					QString("Updating preferences for dataset %1").arg(m_dataSet->name()), nullptr, fw);
			}
		});

	// slicer
	m_slicerChannelID = child->createChannel();
	auto img = dynamic_cast<iAImageData const*>(m_dataSet)->vtkImage();
	for (int s = 0; s < 3; ++s)
	{
		m_slicer[s] = child->slicer(s);
		child->slicer(s)->addChannel(m_slicerChannelID, iAChannelData(m_dataSet->name(), img, m_transfer->colorTF()), true);
		child->slicer(s)->resetCamera();
	}
}

QString iAVolumeViewer::information() const
{
	return iADataSetViewer::information() + "\n" + QString("Statistics: %1").arg(m_imgStatistics);
}

void iAVolumeViewer::dataSetChanged()
{
	auto title = "Histogram " + m_dataSet->name();
	m_histogram->setXCaption(title);
	m_histogramDW->setWindowTitle(title);
}

void iAVolumeViewer::slicerRegionSelected(double minVal, double maxVal, uint channelID)
{
	if (m_slicerChannelID != channelID)
	{
		return;
	}
	// create "windowed" transfer function,
	// such that the full color and opacity contrast is available between minVal and maxVal
	auto ctf = m_transfer->colorTF();
	double range[2];
	ctf->GetRange(range);
	ctf->RemoveAllPoints();
	ctf->AddRGBPoint(range[0], 0.0, 0.0, 0.0);
	ctf->AddRGBPoint(minVal, 0.0, 0.0, 0.0);
	ctf->AddRGBPoint(maxVal, 1.0, 1.0, 1.0);
	ctf->AddRGBPoint(range[1], 1.0, 1.0, 1.0);
	ctf->Build();
	auto otf = m_transfer->opacityTF();
	otf->RemoveAllPoints();
	otf->AddPoint(range[0], 0.0);
	otf->AddPoint(minVal, 0.0);
	otf->AddPoint(maxVal, 1.0);
	otf->AddPoint(range[1], 1.0);
}

void iAVolumeViewer::setPickable(bool pickable)
{
	iADataSetViewer::setPickable(pickable);
	if (m_slicerChannelID == NotExistingChannel)
	{
		return;
	}
	for (int s = 0; s < 3; ++s)
	{
		m_slicer[s]->channel(m_slicerChannelID)->setMovable(pickable);
	}
}

std::shared_ptr<iADataSetRenderer> iAVolumeViewer::createRenderer(vtkRenderer* ren)
{
	auto img = dynamic_cast<iAImageData const*>(m_dataSet)->vtkImage();
	return std::make_shared<iAVolRenderer>(ren, img, this);
}

bool iAVolumeViewer::hasSlicerVis() const
{
	return true;
}

void iAVolumeViewer::setSlicerVisibility(bool visible)
{
	for (int s = 0; s < 3; ++s)
	{
		m_slicer[s]->enableChannel(m_slicerChannelID, visible);
	}
}

QSharedPointer<iAHistogramData> iAVolumeViewer::histogramData() const
{
	return m_histogramData;
}

iAChartWithFunctionsWidget* iAVolumeViewer::histogram()
{
	return m_histogram;
}

iATransferFunction* iAVolumeViewer::transfer()
{
	return m_transfer.get();
}
