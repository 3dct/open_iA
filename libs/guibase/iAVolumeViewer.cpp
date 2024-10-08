// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAVolumeViewer.h"

#include "iAImageData.h"
#include "iAProgress.h"

#include "iAChannelID.h"        // for NotExistingChannel
#include "iAChannelData.h"
#include "iAChannelSlicerData.h"
#include "iADataSetListWidget.h"
#include "iADefaultSettings.h"
#include "iADockWidgetWrapper.h"
#include "iAFileUtils.h"    // for MakeAbsolute
#include "iAJobListView.h"
#include "iAMainWindow.h"   // for preferences, styleChanged
#include "iAMdiChild.h"
#include "iAPreferences.h"
#include "iAProfileProbe.h"
#include "iARunAsync.h"
#include "iASlicer.h"
#include "iAToolsVTK.h"
#include "iATransferFunctionOwner.h"
#include "iAVolumeRenderer.h"
#include "iAXmlSettings.h"

#include <iAChartWithFunctionsWidget.h>
#include <iAHistogramData.h>
#include <iAPlotTypes.h>
#include <iAXYPlotData.h>

#include <iAStringHelper.h>

#include <vtkColorTransferFunction.h>
#include <vtkImageData.h>
#include <vtkOpenGLRenderer.h>
#include <vtkPiecewiseFunction.h>

#include <QAction>
#include <QApplication>

namespace
{
	bool isLarge(vtkImageData* img)
	{
		int const* dim = img->GetDimensions();
		auto byteSize = mapVTKTypeToSize(img->GetScalarType());        // limit is in MB, hence the division
		return (byteSize * dim[0] * dim[1] * dim[2] / 1048576.0) >= iAMainWindow::get()->defaultPreferences().LimitForAuto3DRender;
	}

	const QString componentNames[4]{ "red", "green", "blue", "alpha" };

	QString plotName(int plot, int plotCount)
	{
		return "Frequency" + ((plotCount == 1) ? "" : " " + componentNames[plot]);
	}
	QColor plotColor(int plot, int plotCount)
	{
		auto c = plotCount > 1 && plot < 3 ? QColor(componentNames[plot]) : QApplication::palette().color(QPalette::Shadow);
		c.setAlpha(plotCount > 1 ? 64 : 255);
		return c;
	}

	const QChar RenderSlicerFlag('S');
	const QChar RenderHistogramFlag('H');
	const QChar RenderProfileFlag('P');

	const QString TransferFunction = "TransferFunction";
	constexpr const char HistogramBins[] = "Histogram Bins";
	constexpr const char HistogramLogarithmicYAxis[] = "Histogram Logarithmic y axis";

	const QString Histogram = "Histogram";
	const QString ImageStatistics = "ImageStatistics";
	const QString ImageRange = "ImageRange";
	const QString HistogramSeparator = "|";
	const QString ArrayValueSeparator = ";";
}

inline constexpr char VolumeViewerSettingsName[] = "Default Settings/Volume Viewer";
//! Encapsulates the specifics of the settings of a volume viewer.
//! Handles registration of the settings with iASettingsManager (via deriving from iASettingsObject).
class iAVolumeViewerSettings : iASettingsObject<VolumeViewerSettingsName, iAVolumeViewerSettings>
{
public:
	static iAAttributes& defaultAttributes() {
		static iAAttributes attr;
		if (attr.isEmpty())
		{
			addAttr(attr, HistogramBins, iAValueType::Discrete, 256, 2);
			addAttr(attr, HistogramLogarithmicYAxis, iAValueType::Boolean, false);
			selfRegister();
		}
		return attr;
	}
};

iAVolumeViewer::iAVolumeViewer(iADataSet * dataSet) :
	iADataSetViewer(dataSet),
	m_slicerChannelID(NotExistingChannel),
	m_histogram(nullptr),
	m_profileChart(nullptr),
	m_imgStatistics("Computing...")
{
	m_slicer.fill(nullptr);
	for (auto& a : iAVolumeViewerSettings::defaultAttributes())
	{
		addAttribute(a->name(), a->valueType(), a->defaultValue(), a->min(), a->max());
	}
}

iAVolumeViewer::~iAVolumeViewer()
{
	for (auto s: m_slicer)
	{
		if (s)
		{
			s->removeChannel(m_slicerChannelID);
		}
	}
}

iAImageData const* iAVolumeViewer::volume() const
{
	return dynamic_cast<iAImageData*>(m_dataSet);
}

void iAVolumeViewer::showInSlicers(bool show)
{
	for (auto s: m_slicer)
	{
		s->enableChannel(m_slicerChannelID, show);
	}
}

void iAVolumeViewer::unitDistanceChanged(std::array<double, 3> oldUnitDist, iAMdiChild* child)
{
	Q_UNUSED(oldUnitDist);
	// maybe instead of just setting up profile points to be min and max of dataset here, translate existing ones from old to new spacing?
	setupProfilePoints(child);
	child->updateViews();
}
/*
TODO: link to some trigger in dataset list
void setPickable(bool pickable) override
{
	if (m_channelID == NotExistingChannel)
	{
		return;
	}
	for (auto s: m_slicer)
	{
		s->channel(m_channelID)->setMovable(pickable);
	}
}
unsigned int channelID() const override
{
	return m_channelID;
}
*/


void iAVolumeViewer::prepare(iAProgress* p)
{
	p->setStatus(QString("%1: Computing scalar range").arg(m_dataSet->name()));
	auto img = dynamic_cast<iAImageData const*>(m_dataSet)->vtkImage();

	double range[2];
	if (m_dataSet->hasMetaData(ImageRange))
	{
		stringToArray<double>(m_dataSet->metaData(ImageRange).toString(), range, 2, ArrayValueSeparator);
	}
	else
	{
		img->GetScalarRange(range);
	}

	bool readValidTF = false;
	auto tfSpec = m_dataSet->hasMetaData(TransferFunction) ? m_dataSet->metaData(TransferFunction).toString(): "";
	if (!tfSpec.isEmpty())
	{
		// tfSpec could be either a full transfer function specification or a absolute or relative file name (old)
		iAXmlSettings s;
		if (s.fromString(tfSpec))
		{
			readValidTF = true;
		}
		else if (s.read(tfSpec))
		{
			readValidTF = true;
		}
		else
		{
			auto filePath = QFileInfo(m_dataSet->metaData(iADataSet::FileNameKey).toString()).absolutePath();
			if (!s.read(MakeAbsolute(filePath, tfSpec)))
			{
				LOG(lvlWarn, QString("Failed to read transfer function from value %1").arg(tfSpec));
				readValidTF = false;
			}
		}
		if (readValidTF)
		{
			m_transfer = std::make_shared<iATransferFunctionOwner>();
			if (!s.loadTransferFunction(m_transfer.get()))
			{
				readValidTF = false;
			}
			m_transfer->ensureValidity(range);
		}
	}
	auto numCmp = img->GetNumberOfScalarComponents();
	if (!readValidTF)
	{
		// create default transfer function:
		m_transfer = std::make_shared<iATransferFunctionOwner>(range, numCmp == 1);
	}

	m_imgStatistics = m_dataSet->hasMetaData(ImageStatistics) ? m_dataSet->metaData(ImageStatistics).toString() : "";

	m_histogramData.resize(numCmp);
	bool computeHistograms = m_imgStatistics.isEmpty() || !m_dataSet->hasMetaData(Histogram);
	if (!computeHistograms)  // try and load histograms
	{
		auto histoStr = m_dataSet->metaData(Histogram).toString();
		auto histos = histoStr.split(HistogramSeparator);
		if (histos.size() != numCmp)
		{
			computeHistograms = true;
		}
		else
		{
			for (qsizetype c = 0; c < histos.size(); ++c)
			{
				auto values = stringToVector<QVector<double>, double>(histos[c], ArrayValueSeparator);
				auto numBins = values.size();
				if (numBins < 2)
				{
					computeHistograms = true;
					break;
				}
				auto valueType = isVtkIntegerImage(img) ? iAValueType::Discrete : iAValueType::Continuous;
				auto histRange = iAHistogramData::histoRange(range, numBins, valueType);
				m_histogramData[c] = iAHistogramData::create(plotName(static_cast<int>(c), numCmp),
					valueType, range[0], range[0] + histRange, values);
			}
		}
	}
	if (computeHistograms)
	{
		iAImageStatistics stats;
		p->emitProgress(50);
		p->setStatus(QString("%1: Computing histogram and statistics.").arg(m_dataSet->name()));
		m_imgStatistics = "";
		for (int c = 0; c < numCmp; ++c)
		{
			m_histogramData[c] = iAHistogramData::create(plotName(c, numCmp), img, m_attribValues[HistogramBins].toUInt(), &stats, c);
			m_imgStatistics += QString("%1min=%2, max=%3, µ=%4, σ=%5%6")
				.arg(numCmp > 1 ? QString("component %1: ").arg(c) : "")
				.arg(stats.minimum)
				.arg(stats.maximum)
				.arg(stats.mean)
				.arg(stats.standardDeviation)
				.arg(c < numCmp - 1 ? "; " : "");
		}
	}
	// the number of histogram bins could have beeen adapted during creation, see finalNumBin, or determined via loading:
	m_attribValues[HistogramBins] = static_cast<quint32>(m_histogramData[0]->valueCount());
	p->emitProgress(100);

}

void iAVolumeViewer::createGUI(iAMdiChild* child, size_t dataSetIdx)
{
	if (!m_dataSet->hasMetaData(RenderFlags))
	{
		QString defaultRenderFlags("S");
		auto img = dynamic_cast<iAImageData const*>(m_dataSet)->vtkImage();
		if (!isFlat(img) && !isLarge(img))
		{
			defaultRenderFlags += "R";
		}
		m_dataSet->setMetaData(RenderFlags, defaultRenderFlags);
	}
	iADataSetViewer::createGUI(child, dataSetIdx);
	auto slicerAction = addViewAction("2D", "2d", renderFlagSet(RenderSlicerFlag));
	connect(slicerAction, &QAction::triggered, this, [this, child](bool checked)
		{
			setRenderFlag(RenderSlicerFlag, checked);
			showInSlicers(checked);
			child->updateSlicers();
		});
	m_histogramAction = addViewAction("Histogram", "histogram-tf", renderFlagSet(RenderHistogramFlag));
	connect(m_histogramAction, &QAction::triggered, this, [this](bool checked)
		{
			setRenderFlag(RenderHistogramFlag, checked);
			m_dwHistogram->setVisible(checked);
		});
	auto sliceProfileAction = addViewAction("Slice Profile", "profile", renderFlagSet(RenderProfileFlag));
	connect(sliceProfileAction, &QAction::triggered, this, [this](bool checked)
		{
			setRenderFlag(RenderProfileFlag, checked);
			if (checked)
			{
				updateProfilePlot();
			}
			m_dwProfile->setVisible(checked);
		});
	// histogram
	QString histoName = "Histogram " + m_dataSet->name();
	m_histogram = new iAChartWithFunctionsWidget(child, histoName, "Frequency");
	auto img = dynamic_cast<iAImageData const*>(m_dataSet)->vtkImage();
	int numCmp = img->GetNumberOfScalarComponents();
	for (int c = 0; c < numCmp; ++c)
	{
		auto histogramPlot = std::make_shared<iABarGraphPlot>(m_histogramData[c], plotColor(c, numCmp) );
		m_histogram->addPlot(histogramPlot);
	}
	m_histogram->setTransferFunction(m_transfer.get());
	m_histogram->update();
	// TODO NEWIO:
	//     - better unique widget name!
	//     - option to put combined histograms of multiple datasets into one view
	static int histoNum = -1;
	m_dwHistogram = std::make_shared<iADockWidgetWrapper>(m_histogram, histoName, QString("Histogram%1").arg(++histoNum), "https://github.com/3dct/open_iA/wiki/Histogram");
	connect(m_dwHistogram.get(), &QDockWidget::visibilityChanged, this, [this](bool visible)
	{
		QSignalBlocker sb(m_histogramAction);
		m_histogramAction->setChecked(visible);
	});
	connect(m_histogram, &iAChartWithFunctionsWidget::transferFunctionChanged, child, [child, this]
	{
		for (auto s: m_slicer)
		{
			s->updateMagicLensColors();
		}
		child->updateViews();
	});
	bool visibleHistogram = renderFlagSet(RenderHistogramFlag);
	child->splitDockWidget(child->renderDockWidget(), m_dwHistogram.get(), Qt::Vertical);
	if (!visibleHistogram)
	{
		m_dwHistogram->hide();
	}
	connect(iAMainWindow::get(), &iAMainWindow::styleChanged, this, [this]()
	{
		if (m_histogram->plots().size() > 0)
		{
			m_histogram->plots()[m_histogramData.size()-1]->setColor(QApplication::palette().color(QPalette::Shadow));
		}
		if (m_profileChart->plots().size() > 0)
		{
			m_profileChart->plots()[0]->setColor(QApplication::palette().color(QPalette::Text));
		}
	});

	// slicer
	bool visibleSlicer = renderFlagSet(RenderSlicerFlag);
	m_slicerChannelID = child->createChannel();
	for (int s = 0; s < 3; ++s)
	{
		m_slicer[s] = child->slicer(s);
		child->slicer(s)->addChannel(m_slicerChannelID, iAChannelData(m_dataSet->name(), img, m_transfer->colorTF()/*, TODO NEWIO: opacity TF ?*/), visibleSlicer);
		child->slicer(s)->resetCamera();
	}

	// profile plot:
	bool visibleProfile = renderFlagSet(RenderProfileFlag);
	m_profileProbe = std::make_shared<iAProfileProbe>(img);
	setupProfilePoints(child);
	m_profileChart = new iAChartWidget(nullptr, "Greyvalue", "Distance");
	m_dwProfile = std::make_shared<iADockWidgetWrapper>(m_profileChart, "Profile Plot", "Profile", "https://github.com/3dct/open_iA/wiki/Profile-Plot");
	child->splitDockWidget(child->renderDockWidget(), m_dwProfile.get(), Qt::Vertical);
	if (visibleProfile)
	{
		updateProfilePlot();
	}
	else
	{
		m_dwProfile->hide();
	}
	if (visibleSlicer)
	{
		auto const dim = img->GetDimensions();
		for (int s = 0; s < iASlicerMode::SlicerCount; ++s)
		{
			if (!child->slicerDockWidget(s)->isVisible() && (dim[m_slicer[s]->globalAxis(0)] > 1 && dim[m_slicer[s]->globalAxis(1)] > 1))
			{
				child->slicerDockWidget(s)->setVisible(true);
			}
		}
	}
	connect(child, &iAMdiChild::profilePointChanged, this,
		[this](int pointIdx, double const* globalPos)
	{
		m_profileProbe->updateProbe(pointIdx, globalPos);
		updateProfilePlot();
	});
}

void iAVolumeViewer::setupProfilePoints(iAMdiChild* child)
{
	auto img = dynamic_cast<iAImageData const*>(m_dataSet)->vtkImage();
	auto const start = img->GetOrigin();
	auto const dim = img->GetDimensions();
	auto const spacing = img->GetSpacing();
	double end[3];
	for (int i = 0; i < 3; ++i)
	{
		end[i] = start[i] + (dim[i] - 1) * spacing[i];
	}
	// TODO NEWIO: check if we can do this differently; and if we should maybe not do this if this was already set when the profile of another dataset was initialized!
	child->initProfilePoints(start, end);
	m_profileProbe->updateProbe(0, start);
	m_profileProbe->updateProbe(1, end);
}

QString iAVolumeViewer::information() const
{
	return iADataSetViewer::information() + "\n" + QString("Statistics: %1").arg(m_imgStatistics);
}

void iAVolumeViewer::applyAttributes(QVariantMap const& values)
{
	Q_UNUSED(values);
	auto title = "Histogram " + m_dataSet->name();
	m_histogram->setXCaption(title);
	m_dwHistogram->setWindowTitle(title);

	auto img = dynamic_cast<iAImageData const*>(m_dataSet)->vtkImage();
	size_t newBinCount = iAHistogramData::finalNumBin(img, values[HistogramBins].toUInt());
	m_histogram->setYMappingMode( values[HistogramLogarithmicYAxis].toBool() ? iAChartWidget::Logarithmic : iAChartWidget::Linear);
	m_attribValues[HistogramBins] = static_cast<quint32>(newBinCount);
	constexpr char const FinalNumBinDescr[] = "For discrete-valued datasets, the nearest appropriate value is determined "
		"as the nearest integer divisor of the data value range (the maximum being the full data value range)";
	if (m_histogramData[0]->valueCount() != newBinCount)
	{
		if (newBinCount != values[HistogramBins].toUInt())
		{
			LOG(lvlInfo, QString("Changed number of bins from desired %1 to the next appropriate value %2. %3")
				.arg(values[HistogramBins].toUInt()).arg(newBinCount).arg(FinalNumBinDescr));
		}
		auto fw = runAsync(
			[this, newBinCount, img]
			{
				for (int c = 0; c < img->GetNumberOfScalarComponents(); ++c)
				{
					m_histogramData[c] = iAHistogramData::create(plotName(c, static_cast<int>(newBinCount)), img, newBinCount, nullptr, c);
				}
			},
			[this, img]
			{
				m_histogram->clearPlots();
				auto numCmp = img->GetNumberOfScalarComponents();
				for (int c = 0; c < numCmp; ++c)
				{
					auto histogramPlot = std::make_shared<iABarGraphPlot>(m_histogramData[c], plotColor(c, numCmp));
					m_histogram->addPlot(histogramPlot);
				}
				m_histogram->update();
			},
			this);
		iAJobListView::get()->addJob(
			QString("Computing histogram for dataset %1 (%2 bins)").arg(m_dataSet->name()).arg(newBinCount), nullptr, fw);
	}
	else if (newBinCount != values[HistogramBins].toUInt())
	{
		LOG(lvlInfo, QString("You tried to change the number of bins to %1, but the determined nearest appropriate value %2 "
			"is exactly the one that was set before already; no new histogram will be computed. %3")
			.arg(values[HistogramBins].toUInt()).arg(newBinCount).arg(FinalNumBinDescr));
	}
}

uint iAVolumeViewer::slicerChannelID() const
{
	return m_slicerChannelID;
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
	for (auto s: m_slicer)
	{
		s->channel(m_slicerChannelID)->setMovable(pickable);
	}
}

std::shared_ptr<iADataSetRenderer> iAVolumeViewer::createRenderer(vtkRenderer* ren, QVariantMap const& overrideValues)
{
	auto img = dynamic_cast<iAImageData const*>(m_dataSet)->vtkImage();
	return std::make_shared<iAVolumeRenderer>(ren, img, transfer(), overrideValues);
}

std::shared_ptr<iAHistogramData> iAVolumeViewer::histogramData(int component) const
{
	return m_histogramData[component];
}

iAChartWithFunctionsWidget* iAVolumeViewer::histogram()
{
	return m_histogram;
}

iATransferFunction* iAVolumeViewer::transfer()
{
	return m_transfer.get();
}

void iAVolumeViewer::updateProfilePlot()
{
	const QColor ProfileColor(QApplication::palette().color(QPalette::Text));
	m_profileProbe->updateData();
	m_profileChart->clearPlots();
	auto scalars = m_profileProbe->scalars();
	auto profilePlotData = std::make_shared<iAXYPlotData>(
		QString("Profile %1").arg(m_dataSet->name()),
		iAValueType::Continuous,
		m_profileProbe->numberOfPoints());
	double stepWidth = m_profileProbe->rayLength() / (m_profileProbe->numberOfPoints()-1) ;
	for (vtkIdType p = 0; p < m_profileProbe->numberOfPoints(); ++p)
	{
		profilePlotData->addValue(p * stepWidth, scalars->GetTuple1(p) );
	}
	m_profileChart->addPlot(std::make_shared<iALinePlot>(profilePlotData, ProfileColor));
	m_profileChart->update();
}

QVariantMap iAVolumeViewer::additionalState() const
{
	QVariantMap result;
	// TODO: simpler encoding for transfer function?
	iAXmlSettings s;
	s.saveTransferFunction(m_transfer.get());
	result.insert(TransferFunction, s.toString().replace(QRegularExpression("\n[ ]*"), ""));

	QString histoStr;
	for (size_t hidx=0; hidx < m_histogramData.size(); ++hidx)
	{
		auto h = m_histogramData[hidx];
		// TODO: retrieve vector interface from histogramdata?
		for (size_t idx = 0; idx < h->valueCount(); ++idx)
		{
			histoStr += QString::number(h->yValue(idx));
			if (idx < h->valueCount() - 1)
			{
				histoStr += ArrayValueSeparator;
			}
		}
		if (hidx < m_histogramData.size() - 1)
		{
			histoStr += HistogramSeparator;
		}
	}
	result.insert(Histogram, histoStr);
	result.insert(ImageStatistics, m_imgStatistics);
	result.insert(ImageRange, arrayToString(volume()->vtkImage()->GetScalarRange(), 2, ArrayValueSeparator));
	return result;
}
