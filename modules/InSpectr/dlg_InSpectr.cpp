/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "dlg_InSpectr.h"

#include "dlg_periodicTable.h"
#include "dlg_RefSpectra.h"
#include "iAAccumulatedXRFData.h"
#include "iACharacteristicEnergy.h"
#include "iADecompositionCalculator.h"
#include "iAElementConcentrations.h"
#include "iAElementConstants.h"
#include "iAElementStatisticsInfo.h"
#include "iAEnergySpectrumDiagramData.h"
#include "iAEnergySpectrumWidget.h"
#include "iAFunctionalBoxplotQtDrawer.h"
#include "iAPeriodicTableListener.h"
#include "iAPieChartGlyph.h"
#include "iAPieChartWidget.h"
#include "iAReferenceSpectraLibrary.h"
#include "iAXRFData.h"
#include "iAXRFOverlay.h"

#include <charts/iAChartFunctionTransfer.h>
#include <charts/iAPlotTypes.h>
#include <charts/iAMappingDiagramData.h>
#include <iAChannelData.h>
#include <iAChannelSlicerData.h>
#include <iAColorTheme.h>
#include <iAConnector.h>
#include <iAFunctionalBoxplot.h>
#include <iAMathUtility.h>
#include <iARenderer.h>
#include <iASlicer.h>
#include <iAVtkWidget.h>
#include <io/iAFileUtils.h>
#include <io/iAIO.h>
#include <qthelper/iAWidgetAddHelper.h>
#include <mdichild.h>
#include <qthelper/iADockWidgetWrapper.h>

#include <itkImageBase.h>
#include <itkImage.h>
#include <itkIdentityTransform.h>
#include <itkMutualInformationImageToImageMetric.h>
#include <itkLinearInterpolateImageFunction.h>

#include <vtkColorTransferFunction.h>
#include <vtkDiscretizableColorTransferFunction.h>
#include <vtkImageData.h>
#include <vtkImageResample.h>
#include <vtkInteractorStyleImage.h>
#include <vtkLookupTable.h>
#include <vtkMath.h>
#include <vtkMetaImageWriter.h>
#include <vtkOpenGLRenderer.h>
#include <vtkPiecewiseFunction.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRendererCollection.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkScalarBarActor.h>
#include <vtkTextProperty.h>
#include <vtkTransform.h>

#include <QColorDialog>
#include <QFileDialog>
#include <QMapIterator>

#include <cassert>


dlg_InSpectr::dlg_InSpectr(QWidget *parentWidget, dlg_periodicTable* dlgPeriodicTable, dlg_RefSpectra* dlgRefSpectra):
	dlg_xrfContainer(parentWidget),
	m_pieGlyphsEnabled(false),
	m_spectraHistogramColormap(QString::fromUtf8(":/images/colormap.png")),
	m_initialized(false),
	m_ctfChanged(true),
	m_decompositionLoaded(false),
	m_spectrumDiagram(nullptr),
	m_accumulatedGridLayout(nullptr),
	m_oTF(vtkSmartPointer<vtkPiecewiseFunction>::New()),
	m_cTF(vtkSmartPointer<vtkColorTransferFunction>::New()),
	m_xrfData(new iAXRFData),
	m_enabledChannels(0),
	m_spectrumSelectionChannelID(NotExistingChannel),
	m_periodicTable(dlgPeriodicTable),
	m_selection_ctf(vtkSmartPointer<vtkColorTransferFunction>::New()),
	m_selection_otf(vtkSmartPointer<vtkPiecewiseFunction>::New()),
	m_refSpectra(dlgRefSpectra),
	m_periodicTableListener(new iAPeriodicTableListener(this))
{
	spectrumVisWidget->hide();

	spectraSettings->hide();
	gb_spectraSettings->hide();
	gb_pieGlyphsSettings->hide();

	QColor color(255, 0, 0);
	m_selection_ctf->AddRGBPoint(0, 0, 0, 0);
	m_selection_ctf->AddRGBPoint(1, color.redF(), color.greenF(), color.blueF());

	m_selection_otf->AddPoint(0, 0);
	m_selection_otf->AddPoint(1, 1);

	connect ( comB_AccumulateFunction, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &dlg_InSpectr::updateAccumulate);
	connect ( comB_spectrumSelectionMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &dlg_InSpectr::updateSelectionMode);
	connect ( cb_spectraLines, &QCheckBox::stateChanged, this, &dlg_InSpectr::showSpectraLines);
	connect ( cb_spectraHistograms, &QCheckBox::stateChanged, this, &dlg_InSpectr::showSpectraHistograms);
	connect ( tb_spectraSettings, &QToolButton::toggled, this, &dlg_InSpectr::showSpectraHistogramsSettings);
	connect ( cb_spectrumProbing, &QCheckBox::stateChanged, this, &dlg_InSpectr::showVoxelSpectrum);
	connect ( pb_decompose, &QPushButton::clicked, this, &dlg_InSpectr::decomposeElements);
	connect ( pb_decompositionLoad, &QPushButton::clicked, this, &dlg_InSpectr::loadDecomposition);
	connect ( pb_decompositionStore, &QPushButton::clicked, this, &dlg_InSpectr::storeDecomposition);
	connect ( cb_combinedElementMaps, &QCheckBox::stateChanged, this, &dlg_InSpectr::combinedElementMaps);
	connect ( sl_specHistSensitivity, &QSlider::valueChanged, this, &dlg_InSpectr::spectraHistSensitivityChanged);
	connect ( pb_recompute, &QPushButton::clicked, this, &dlg_InSpectr::recomputeSpectraHistograms);
	connect ( cb_smoothOpacFade, &QCheckBox::stateChanged, this, &dlg_InSpectr::smoothOpacityFadeChecked);
	connect ( sl_specHistOpacThreshold, &QSlider::valueChanged, this, &dlg_InSpectr::spectraOpacityThresholdChanged);
	connect ( comB_colormap, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &dlg_InSpectr::changeColormap);
	connect ( cb_linkedElementMaps, &QCheckBox::stateChanged, this, &dlg_InSpectr::showLinkedElementMaps);
	connect ( cb_pieChartGlyphs, &QCheckBox::stateChanged, this, &dlg_InSpectr::pieGlyphsVisualization);
	connect ( tb_pieGlyphSettings, &QToolButton::toggled, this, &dlg_InSpectr::showPieGlyphsSettings);
	connect ( sl_pieGlyphsOpacity, &QSlider::valueChanged, this, &dlg_InSpectr::updatePieGlyphParameters);
	connect ( sl_pieGlyphsSpacing, &QSlider::valueChanged, this, &dlg_InSpectr::updatePieGlyphParameters);
	connect ( sl_pieGlyphResolution, &QSlider::valueChanged, this, &dlg_InSpectr::updatePieGlyphParameters);
	connect ( sl_concentrationOpacity, &QSlider::valueChanged, this, &dlg_InSpectr::updateConcentrationOpacity);
	connect ( cb_aggregatedSpectrum, &QCheckBox::stateChanged, this, &dlg_InSpectr::showAggregatedSpectrum);
	connect ( cb_functionalBoxplot,  &QCheckBox::stateChanged, this, &dlg_InSpectr::updateFunctionalBoxplot);
	connect ( rb_DrawMode_Log, &QToolButton::toggled, this, &dlg_InSpectr::setLogDrawMode);
	connect ( rb_DrawMode_Lin, &QToolButton::toggled, this, &dlg_InSpectr::setLinDrawMode);
	connect ( pb_computeSimilarityMap, &QPushButton::clicked, this, &dlg_InSpectr::computeSimilarityMap);
}

void dlg_InSpectr::AddSimilarityMarkers()
{
	m_spectrumDiagram->addPlot( m_selectedBinXDrawer );
	m_spectrumDiagram->addPlot( m_selectedBinYDrawer );
	m_spectrumDiagram->update();
}

void dlg_InSpectr::RemoveSimilarityMarkers()
{
	m_spectrumDiagram->removePlot( m_selectedBinXDrawer );
	m_spectrumDiagram->removePlot( m_selectedBinYDrawer );
	m_spectrumDiagram->update();
}

void dlg_InSpectr::init(double minEnergy, double maxEnergy, bool haveEnergyLevels,
		iAWidgetAddHelper & widgetAddHelper)
{
	spectrumVisWidget->show();
	// initialize functions
	m_oTF->RemoveAllPoints();
	m_oTF->AddPoint ( minEnergy, 0.0 );
	m_oTF->AddPoint ( maxEnergy, 0.0 );

	m_cTF->RemoveAllPoints();
	m_cTF->AddRGBPoint ( minEnergy, 0.0, 0.0, 0.0 );
	m_cTF->AddRGBPoint ( maxEnergy, 0.0, 0.0, 0.0 );
	m_cTF->Build();
	m_xrfData->SetEnergyRange(minEnergy, maxEnergy);
	m_accumulatedXRF = QSharedPointer<iAAccumulatedXRFData>(new iAAccumulatedXRFData(m_xrfData, minEnergy, maxEnergy));
	m_voxelEnergy = QSharedPointer<iAEnergySpectrumDiagramData>(new iAEnergySpectrumDiagramData(m_xrfData.data(), m_accumulatedXRF.data()));
	m_voxelSpectrumDrawer = QSharedPointer<iAStepFunctionPlot>(new iAStepFunctionPlot(m_voxelEnergy, QColor(150, 0, 0)));
	m_spectrumDiagram = new iAEnergySpectrumWidget(this, dynamic_cast<MdiChild*>(parent()), m_accumulatedXRF, m_oTF, m_cTF, this,
		haveEnergyLevels ? "Energy (keV)" : "Energy (bins)");
	m_spectrumDiagram->setObjectName(QString::fromUtf8("EnergySpectrum"));

	m_selectedBinXDrawer = QSharedPointer<iASelectedBinPlot>(new iASelectedBinPlot(m_voxelEnergy, 0, QColor(150, 0, 0, 50)));
	m_selectedBinYDrawer = QSharedPointer<iASelectedBinPlot>(new iASelectedBinPlot(m_voxelEnergy, 0, QColor(0, 0, 150, 50)));

	connect((iAChartTransferFunction*)(m_spectrumDiagram->functions()[0]), &iAChartTransferFunction::Changed, this, &dlg_InSpectr::SpectrumTFChanged);
	iADockWidgetWrapper* spectrumChartContainer = new iADockWidgetWrapper(m_spectrumDiagram, "Spectrum View", "SpectrumChartWidget");
	spectrumChartContainer->setContentsMargins(0, 0, 0, 0);

	InitCommonGUI(widgetAddHelper);
	widgetAddHelper.m_mdiChild->logDockWidget()->show();
	widgetAddHelper.SplitWidget(spectrumChartContainer, widgetAddHelper.m_mdiChild->logDockWidget(), Qt::Vertical);
	widgetAddHelper.m_mdiChild->logDockWidget()->hide();
	widgetAddHelper.SplitWidget(m_pieChartContainer, spectrumChartContainer);

	m_ctfChanged  = true;
	m_initialized = true;

	m_colormapRen = vtkSmartPointer<vtkRenderer>::New();
	m_colormapRen->SetBackground(1.0, 1.0, 1.0);

	CREATE_OLDVTKWIDGET(m_colormapWidget);
	horizontalLayout_8->insertWidget(0, m_colormapWidget);
	vtkSmartPointer<vtkInteractorStyleImage> style = vtkSmartPointer<vtkInteractorStyleImage>::New();
#if VTK_MAJOR_VERSION < 9
	m_colormapWidget->GetRenderWindow()->AddRenderer(m_colormapRen);
	m_colormapWidget->GetInteractor()->SetInteractorStyle(style);
#else
	m_colormapWidget->renderWindow()->AddRenderer(m_colormapRen);
	m_colormapWidget->interactor()->SetInteractorStyle(style);
#endif

	m_colormapLUT = vtkSmartPointer<vtkColorTransferFunction>::New();
	m_colormapLUT->SetColorSpaceToRGB();
	m_colormapLUT->AddRGBPoint(0.0, 0, 0, 0);
	m_colormapLUT->AddRGBPoint(1.0, 0, 0, 0);

	m_colormapScalarBarActor = vtkSmartPointer<vtkScalarBarActor>::New();
	m_colormapScalarBarActor->SetTitle("Frequency");
	m_colormapScalarBarActor->SetNumberOfLabels(9);
	m_colormapScalarBarActor->SetLookupTable(m_colormapLUT);
	m_colormapScalarBarActor->GetLabelTextProperty()->SetColor(0,0,0);
	m_colormapScalarBarActor->GetLabelTextProperty()->SetBold(0);
	m_colormapScalarBarActor->GetLabelTextProperty()->SetShadow(0);
	m_colormapScalarBarActor->GetTitleTextProperty()->SetColor(0,0,0);
	m_colormapScalarBarActor->GetTitleTextProperty()->SetBold(0);
	m_colormapScalarBarActor->GetTitleTextProperty()->SetShadow(0);
	m_colormapScalarBarActor->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
	m_colormapScalarBarActor->SetPosition(0.0, 0.07);
	m_colormapScalarBarActor->SetPosition2(1.0, 0.93);

	m_colormapRen->AddActor2D(m_colormapScalarBarActor);
#if VTK_MAJOR_VERSION < 9
	m_colormapWidget->GetRenderWindow()->Render();
#else
	m_colormapWidget->renderWindow()->Render();
#endif

	m_refSpectra->cb_showRefSpectra->setEnabled(true);
	m_refSpectra->cb_showRefLines->setEnabled(true);
	pb_decompose->setEnabled(true);
}

void dlg_InSpectr::InitElementMaps(/* QSharedPointer<iAElementConcentrations> conc */iAWidgetAddHelper & widgetAddHelper)
{
	InitCommonGUI(widgetAddHelper);
	widgetAddHelper.SplitWidget(m_pieChartContainer, m_periodicTable, Qt::Vertical);
}

void dlg_InSpectr::InitCommonGUI(iAWidgetAddHelper & widgetAddHelper)
{
	m_periodicTable->setListener(m_periodicTableListener);


	// load reference spectra & characteristic energy lines:
	QString rootDir(QCoreApplication::applicationDirPath() + "/refSpectra/");
	m_refSpectraLib = QSharedPointer<iAReferenceSpectraLibrary>(new iAReferenceSpectraLibrary(
		rootDir + "elementSpectra/reference_library.reflib"));
	m_refSpectra->getSpectraList()->setModel(m_refSpectraLib->getItemModel().data());
	EnergyLoader::Load(rootDir + "characteristic-energies.cel", m_characteristicEnergies);
	connect(m_refSpectra->getSpectraList(), &QListView::doubleClicked, this, &dlg_InSpectr::ReferenceSpectrumDoubleClicked, Qt::UniqueConnection);
	connect(m_refSpectra->getSpectraList(), &QListView::clicked, this, &dlg_InSpectr::ReferenceSpectrumClicked, Qt::UniqueConnection );
	connect(m_refSpectraLib->getItemModel().data(), &QStandardItemModel::itemChanged, this, &dlg_InSpectr::ReferenceSpectrumItemChanged, Qt::UniqueConnection);
	connect(m_refSpectra->cb_showRefSpectra, &QCheckBox::stateChanged, this, &dlg_InSpectr::showRefSpectraChanged);
	connect(m_refSpectra->cb_showRefLines, &QCheckBox::stateChanged, this, &dlg_InSpectr::showRefLineChanged);

	m_pieChart = new iAPieChartWidget(this);
	m_pieChart->setObjectName(QString::fromUtf8("Composition"));
	m_pieChartContainer = new iADockWidgetWrapper(m_pieChart, "Element Concentration", "PieChartWidget");
	m_pieChartContainer->setContentsMargins(0, 0, 0, 0);
	//m_pieChartContainer->hide();
	widgetAddHelper.SplitWidget(m_periodicTable, this, Qt::Vertical);
	widgetAddHelper.TabWidget(m_refSpectra, this);
}

void dlg_InSpectr::setLogDrawMode(bool checked)
{
	if (checked)
	{
		m_spectrumDiagram->setYMappingMode(iAEnergySpectrumWidget::Logarithmic);
		m_spectrumDiagram->update();
	}
}

void dlg_InSpectr::setLinDrawMode(bool checked)
{
	if (checked)
	{
		m_spectrumDiagram->setYMappingMode(iAEnergySpectrumWidget::Linear);
		m_spectrumDiagram->update();
	}
}

bool dlg_InSpectr::IsInitialized()
{
	return m_initialized;
}

vtkSmartPointer<vtkImageData> dlg_InSpectr::GetCombinedVolume()
{
	return m_xrfData->GetCombinedVolume();
}

vtkSmartPointer<vtkColorTransferFunction> dlg_InSpectr::GetColorTransferFunction()
{
	return m_xrfData->GetColorTransferFunction();
}

QThread* dlg_InSpectr::UpdateForVisualization()
{
	if (m_ctfChanged)
	{
		m_ctfChanged = false;
		return m_xrfData->UpdateCombinedVolume(m_cTF);
	}
	return nullptr;
}

QSharedPointer<iAXRFData> dlg_InSpectr::GetXRFData()
{
	return m_xrfData;
}

void dlg_InSpectr::updateComposition(QVector<double> const & concentration)
{
	if (m_refSpectraLib->spectra.size() == 0) // can't do anything without refspectra!
	{
		return;
	}
	if( !m_pieChartContainer->isVisible() )
		m_pieChartContainer->show();
	m_pieChart->clearPieces();
	for (int i = 0; i<concentration.size(); ++i)
	{
		if (concentration[i] > 0.001)
		{
			QString caption = QString("%1: %2%")
				.arg(m_refSpectraLib->spectra[m_decomposeSelectedElements[i]].name())
				.arg(concentration[i]*100, 0, 'g', 2);
			QColor color = m_refSpectraLib->getElementColor(m_decomposeSelectedElements[i]);
			m_pieChart->addPiece(caption,
				concentration[i]*100,
				color);
		}
	}
	if (m_pieChart->empty())
	{
		m_pieChart->addPiece("Unknown", 100, Qt::lightGray);
	}
	m_pieChart->update();
}

void dlg_InSpectr::UpdateVoxelSpectrum(int x, int y, int z)
{
	m_voxelEnergy->updateEnergyFunction(x, y, z);
	m_spectrumDiagram->update();
}

void dlg_InSpectr::UpdateConcentrationViews( int x, int y, int z )
{
	if (m_elementConcentrations)
	{
		QVector<double> concentrations = m_elementConcentrations->getConcentrationForVoxel(x, y, z);
		updateComposition(concentrations);
		for (int i=0; i<concentrations.size(); ++i)
		{
			m_periodicTable->setConcentration(
				m_refSpectraLib->spectra[m_decomposeSelectedElements[i]].GetSymbol(), concentrations[i],
				m_refSpectraLib->getElementColor(m_decomposeSelectedElements[i]));
		}
		m_periodicTable->repaint();
	}
}

void dlg_InSpectr::SpectrumTFChanged()
{
	m_ctfChanged = true;
}

void dlg_InSpectr::updateAccumulate(int fctIdx)
{
	m_accumulatedXRF->setFct(fctIdx);
	m_spectrumDiagram->update();
}

void dlg_InSpectr::initSpectraLinesDrawer()
{
	int extent[6];
	m_xrfData->GetExtent(extent);

	if (m_spectraLinesDrawer)
	{
		m_spectraLinesDrawer->clear();
	}
	else
	{
		m_spectraLinesDrawer = QSharedPointer<iAPlotCollection>(new iAPlotCollection);
	}

	long numberOfSpectra = (extent[1]-extent[0]+1)*(extent[3]-extent[2]+1)*(extent[5]-extent[4]+1);
	int step = static_cast<int>(std::max(1.0, log10(static_cast<double>(numberOfSpectra))));
	int transparency = 255 / (step * 3);
	for (int x=extent[0]; x<=extent[1]; x += step)
	{
		for (int y=extent[2]; y<=extent[3]; y += step)
		{
			for (int z=extent[4]; z<=extent[5]; z += step)
			{
				QSharedPointer<iAEnergySpectrumDiagramData> dataset(new iAEnergySpectrumDiagramData(m_xrfData.data(), m_accumulatedXRF.data()));
				dataset->updateEnergyFunction(x, y, z);

				bool isSelected = m_activeFilter.empty() ||
					m_xrfData->CheckFilters(x, y, z, m_activeFilter, static_cast<iAFilterMode>(comB_spectrumSelectionMode->currentIndex()));

				QSharedPointer<iALinePlot> lineDrawer(new iALinePlot(dataset,
					m_activeFilter.empty() ? QColor(96, 102, 174, transparency) :
					(isSelected ? QColor(255, 0, 0, transparency): QColor(88, 88, 88, transparency/2))));
				m_spectraLinesDrawer->add(lineDrawer);
			}
		}
	}
}

void dlg_InSpectr::initSpectraOverlay()
{
	int numBin = sb_numBins->value();
	bool smoothFade = cb_smoothOpacFade->isChecked();
	double threshMax = sl_specHistOpacThreshold->maximum() + 1, threshVal = sl_specHistOpacThreshold->value();
	double sensMax   = sl_specHistSensitivity->maximum()   + 1, sensVal   = sl_specHistSensitivity->value();
	m_spectraHistogramImage = CalculateSpectraHistogramImage(
		m_colormapLUT,
		m_accumulatedXRF,
		m_spectraHistogramColormap,
		numBin,
		sensVal, sensMax, threshVal, threshMax, smoothFade);
	m_spectrumDiagram->addImageOverlay(m_spectraHistogramImage);
#if VTK_MAJOR_VERSION < 9
	m_colormapWidget->GetRenderWindow()->Render();
#else
	m_colormapWidget->renderWindow()->Render();
#endif
}

void dlg_InSpectr::showSpectraLines(int show)
{
	if (show)
	{
		if (!m_spectraLinesDrawer)
		{
			initSpectraLinesDrawer();
		}
		m_spectrumDiagram->addPlot(m_spectraLinesDrawer);
	}
	else
	{
		m_spectrumDiagram->removePlot(m_spectraLinesDrawer);
	}
	m_spectrumDiagram->update();
}

void dlg_InSpectr::showSpectraHistograms( int show )
{
	if (show)
	{
		initSpectraOverlay();
		tb_spectraSettings->setEnabled(true);
	}
	else
	{
		if(!m_spectraHistogramImage.isNull())
		{
			m_spectrumDiagram->removeImageOverlay(m_spectraHistogramImage.data());
			tb_spectraSettings->setEnabled(false);
		}
	}
	m_spectrumDiagram->update();
}

void dlg_InSpectr::showSpectraHistogramsSettings( bool isChecked )
{
	if(isChecked)
	{
		spectraSettings->show();
		gb_spectraSettings->show();
	}
	else
	{
		spectraSettings->hide();
		gb_spectraSettings->hide();
	}
}

void dlg_InSpectr::showVoxelSpectrum(int show)
{
	if (!m_xrfData)
	{
		return;
	}
	if (show)
	{
		m_spectrumDiagram->addPlot(m_voxelSpectrumDrawer);
	}
	else
	{
		m_spectrumDiagram->removePlot(m_voxelSpectrumDrawer);
		m_spectrumDiagram->update();
	}
}

void dlg_InSpectr::showAggregatedSpectrum( int show )
{
	m_spectrumDiagram->plots()[0]->setVisible(show);
	m_spectrumDiagram->update();
}

void dlg_InSpectr::updateFunctionalBoxplot(int show)
{
	if (show)
	{
		m_functionalBoxplotImage = drawFunctionalBoxplot(m_accumulatedXRF->functionalBoxPlot(),
			m_xrfData->size(),
			m_accumulatedXRF->yBounds()[1]);
		m_spectrumDiagram->addImageOverlay(m_functionalBoxplotImage);
	}
	else
	{
		m_spectrumDiagram->removeImageOverlay(m_functionalBoxplotImage.data());
	}
	m_spectrumDiagram->update();
}

void dlg_InSpectr::ReferenceSpectrumDoubleClicked( const QModelIndex &index )
{
	assert(index.row() >= 0);
	QColor initCol = m_refSpectraLib->getElementColor(index);
	QColor newColor = QColorDialog::getColor(initCol, this, "New color for the reference spectrum", QColorDialog::ShowAlphaChannel);
	if(newColor.isValid())
	{
		m_refSpectraLib->getItemModel()->itemFromIndex(index)->setData(newColor, Qt::DecorationRole);

		for (int i=0; i<m_elementRenderers.size(); ++i)
		{
			if (m_elementRenderers[i]->GetRefLibIndex() == static_cast<size_t>(index.row()))
			{
				InitElementRenderer(m_elementRenderers[i], index.row());
			}
		}
	}
}

namespace {
	int findCharEnergy(QVector<iACharacteristicEnergy> const & energies, QString const & symbol)
	{
		for (int i=0; i<energies.size(); ++i)
		{
			if (energies[i].symbol == symbol)
			{
				return i;
			}
		}
		return -1;
	}
}

void dlg_InSpectr::ReferenceSpectrumItemChanged( QStandardItem * item )
{
	QModelIndex index = item->index();
	int indRow = index.row();
	bool showRefSpectra = m_refSpectra->cb_showRefSpectra->isChecked();
	bool showRefLine = m_refSpectra->cb_showRefLines->isChecked();
	if(showRefSpectra)
	{
		RemoveReferenceSpectrum(indRow);
	}
	if (showRefLine)
	{
		RemoveElementLine(m_refSpectraLib->spectra[indRow].GetSymbol());
	}
	int elemRendInd = -1;
	assert(indRow >= 0);
	for (int i=0; i<m_elementRenderers.size(); ++i)
	{
		if( m_elementRenderers[i]->GetRefLibIndex() == static_cast<size_t>(indRow))
		{
			elemRendInd = i;
		}
	}

	if( Qt::Unchecked == item->checkState() )
	{
		if( elemRendInd >= 0)
		{
			m_elementRenderers[elemRendInd]->hide();
		}
	}
	else if( Qt::Checked == item->checkState() )
	{
		if( elemRendInd >= 0)
		{
			m_elementRenderers[elemRendInd]->show();
		}

		if(showRefSpectra)
		{
			AddReferenceSpectrum(indRow);
		}
		if (showRefLine)
		{
			AddElementLine(m_refSpectraLib->spectra[indRow].GetSymbol());
		}
	}
	if (m_spectrumDiagram)
	{
		m_spectrumDiagram->update();
	}
}

void dlg_InSpectr::decomposeElements()
{
	if (!m_refSpectraLib)
	{
		(dynamic_cast<MdiChild*>(parent()))->addMsg(tr("Reference spectra have to be loaded!"));
		return;
	}
	if (m_decompositionCalculator)
	{
		m_decompositionCalculator->Stop();
		(dynamic_cast<MdiChild*>(parent()))->addMsg(tr("Decomposition was aborted by user."));
		return;
	}
	m_elementConcentrations = QSharedPointer<iAElementConcentrations>(new iAElementConcentrations());
	m_decompositionCalculator = QSharedPointer<iADecompositionCalculator>(new iADecompositionCalculator(
		m_elementConcentrations,
		m_xrfData,
		m_accumulatedXRF));
	m_decomposeSelectedElements.clear();
	for (size_t i=0; i<m_refSpectraLib->spectra.size(); ++i)
	{
		if (m_refSpectraLib->getItemModel()->item(i)->checkState() == Qt::Checked)
		{
			m_decomposeSelectedElements.push_back(i);
			m_decompositionCalculator->AddElement(&m_refSpectraLib->spectra[i]);
		}
	}
	if (m_decompositionCalculator->ElementCount() == 0)
	{
		m_decompositionCalculator.clear();
		(dynamic_cast<MdiChild*>(parent()))->addMsg(tr("You have to select at least one element from the reference spectra list!"));
		return;
	}
	pb_decompose->setText("Stop");
	connect(m_decompositionCalculator.data(), &iADecompositionCalculator::success, this, &dlg_InSpectr::decompositionSuccess);
	connect(m_decompositionCalculator.data(), &iADecompositionCalculator::finished, this, &dlg_InSpectr::decompositionFinished);
	connect(m_decompositionCalculator.data(), &iADecompositionCalculator::progress, dynamic_cast<MdiChild*>(parent()), &MdiChild::updateProgressBar);
	m_decompositionCalculator->start();
	(dynamic_cast<MdiChild*>(parent()))->addMsg(tr("Decomposition calculation started..."));
}

void dlg_InSpectr::decompositionSuccess()
{
	(dynamic_cast<MdiChild*>(parent()))->addMsg(tr("Decomposition calculation successful."));
	decompositionAvailable();
}

void dlg_InSpectr::decompositionAvailable()
{
	if (m_elementConcentrations->hasAvgConcentration())
	{
		updateComposition(m_elementConcentrations->getAvgConcentration());
	}
	enableControlsNeedingDecompositionData();
}

void dlg_InSpectr::decompositionFinished()
{
	m_decompositionCalculator.clear();
	(dynamic_cast<MdiChild*>(parent()))->hideProgressBar();
	pb_decompose->setText("Calculate");
}

void dlg_InSpectr::loadDecomposition()
{
	if (!m_refSpectraLib)
	{
		(dynamic_cast<MdiChild*>(parent()))->addMsg(tr("Reference spectra have to be loaded!"));
		return;
	}
	QString fileName = QFileDialog::getOpenFileName(
		QApplication::activeWindow(),
		tr("Load File"),
		(dynamic_cast<MdiChild*>(parent()))->filePath(),
		tr("Volstack files (*.volstack);;")
	);
	if (fileName.isEmpty())
	{
		return;
	}
	if (!m_elementConcentrations)
	{
		m_elementConcentrations = QSharedPointer<iAElementConcentrations>(new iAElementConcentrations());
	}
	else
	{
		m_elementConcentrations->clear();
	}

	iAIO io(
		(dynamic_cast<MdiChild*>(parent()))->logger(),
		dynamic_cast<MdiChild*>(parent()),
		m_elementConcentrations->getImageListPtr()
	);
	io.setupIO(VOLUME_STACK_VOLSTACK_READER, fileName);
	io.start();
	io.wait();

	QString elementNames = io.additionalInfo();
	QStringList elements = elementNames.split(",");

	elements.replaceInStrings(QRegExp("^\\s+"), ""); // trim whitespaces
	updateDecompositionGUI( elements );
}

void dlg_InSpectr::enableControlsNeedingDecompositionData()
{
	m_decompositionLoaded = true;
	pb_decompositionStore->setEnabled(true);
	cb_combinedElementMaps->setEnabled(true);
	cb_linkedElementMaps->setEnabled(true);
}

void dlg_InSpectr::storeDecomposition()
{
	QString fileName = QFileDialog::getSaveFileName(
		QApplication::activeWindow(),
		tr("Save File"),
		QDir::currentPath(),
		tr("Volstack files (*.volstack);;")
	);
	if (fileName.isEmpty())
	{
		return;
	}

	QString elementInfo("elementNames: ");
	for (int i=0;i<m_decomposeSelectedElements.size(); ++i)
	{
		elementInfo.append(m_refSpectraLib->spectra[m_decomposeSelectedElements[i]].name());
		if (i < m_decomposeSelectedElements.size()-1)
		{
			elementInfo.append(",");
		}
	}

	iAIO io(
		(dynamic_cast<MdiChild*>(parent()))->logger(),
		dynamic_cast<MdiChild*>(parent()),
		m_elementConcentrations->getImageListPtr());

	io.setupIO(VOLUME_STACK_VOLSTACK_WRITER, fileName);
	io.setAdditionalInfo(elementInfo);

	io.start();
	io.wait();
}

void dlg_InSpectr::combinedElementMaps(int show)
{
	if (!m_elementConcentrations)
	{
		return;
	}

	pieGlyphsVisualization( cb_pieChartGlyphs->isChecked() );
	MdiChild * mdiChild = (dynamic_cast<MdiChild*>(parent()));
	if (!show)
	{
		for (int i=0; i<m_enabledChannels; ++i)
		{
			mdiChild->setChannelRenderingEnabled(m_channelIDs[i], false);
		}
		m_enabledChannels = 0;
		cb_pieChartGlyphs->setEnabled(false);

		return;
	}

	m_enabledChannels = 0;
	for (size_t i=0; i < m_refSpectraLib->spectra.size() &&
		m_enabledChannels < iAChannelData::Maximum3DChannels;
		++i)
	{
		if (m_refSpectraLib->getItemModel()->item(i)->checkState() != Qt::Checked ||
			m_decomposeSelectedElements.indexOf(i) == -1)
		{
			m_refSpectraLib->setElementChannel(i, -1);
			continue;
		}
		m_channelColors.resize(m_enabledChannels + 1);
		if (m_channelIDs.size() <= static_cast<size_t>(m_enabledChannels))
		{
			m_channelIDs.push_back(mdiChild->createChannel());
		}
		//auto chData = mdiChild->channelData(m_channelIDs[m_enabledChannels]);
		vtkSmartPointer<vtkImageData> chImgData = m_elementConcentrations->getImage(m_decomposeSelectedElements.indexOf(i));
		QColor color = m_refSpectraLib->getElementColor(i);
		m_channelColors[m_enabledChannels] = color;
		float h, s, v;
		vtkMath::RGBToHSV(color.red()/255.0, color.green()/255.0, color.blue()/255.0, &h, &s, &v);
		m_ctf[m_enabledChannels] = vtkSmartPointer<vtkLookupTable>::New();
		m_ctf[m_enabledChannels]->SetHueRange(h, h);
		m_ctf[m_enabledChannels]->SetSaturationRange(s, s);
		m_ctf[m_enabledChannels]->SetValueRange(v, v);
		m_ctf[m_enabledChannels]->SetAlphaRange(0.0, 1.0);
		m_ctf[m_enabledChannels]->SetTableRange(0.0, 1.0);
		m_ctf[m_enabledChannels]->SetNumberOfTableValues(256);
		m_ctf[m_enabledChannels]->SetRampToLinear();
		m_ctf[m_enabledChannels]->Build();

		m_otf[m_enabledChannels] = vtkSmartPointer<vtkPiecewiseFunction>::New();
		m_otf[m_enabledChannels]->AddPoint(0, 0);
		m_otf[m_enabledChannels]->AddPoint(1, 0.1);

		mdiChild->updateChannel(m_channelIDs[m_enabledChannels], chImgData, m_ctf[m_enabledChannels], m_otf[m_enabledChannels], true);
		mdiChild->updateChannelOpacity(m_channelIDs[m_enabledChannels], 1);

		// set channel index in model data for reference:
		m_refSpectraLib->setElementOpacity(i, 10);
		m_refSpectraLib->setElementChannel(i, m_enabledChannels);

		++m_enabledChannels;
	}
	if (m_enabledChannels > 0)
	{
		cb_pieChartGlyphs->setEnabled(true);
	}
}

void dlg_InSpectr::recomputeSpectraHistograms()
{
	if(!m_spectraHistogramImage.isNull())
		m_spectrumDiagram->removeImageOverlay(m_spectraHistogramImage.data());
	initSpectraOverlay();
	m_spectrumDiagram->update();
}

void dlg_InSpectr::spectraHistSensitivityChanged( int /*newVal*/ )
{
	sl_specHistSensitivity->repaint();
	recomputeSpectraHistograms();
}

void dlg_InSpectr::smoothOpacityFadeChecked( int /*checked*/ )
{
	recomputeSpectraHistograms();
}

void dlg_InSpectr::spectraOpacityThresholdChanged( int /*newVal*/ )
{
	sl_specHistOpacThreshold->repaint();
	recomputeSpectraHistograms();
}

void dlg_InSpectr::changeColormap( int colormapInd )
{
	switch (colormapInd)
	{
	case 0:
		m_spectraHistogramColormap = QImage(QString::fromUtf8(":/images/colormap.png"));
		break;
	case 1:
		m_spectraHistogramColormap = QImage(QString::fromUtf8(":/images/w2b_colormap.png"));
		break;
	case 2:
		m_spectraHistogramColormap = QImage(QString::fromUtf8(":/images/w2r_colormap.png"));
		break;
	}
	recomputeSpectraHistograms();
}

void dlg_InSpectr::OnSelectionUpdate(QVector<iASpectrumFilter> const & filter)
{
	m_activeFilter = filter;
	updateSelection();
}

void dlg_InSpectr::updateSelectionMode(int /*modeIdx*/)
{
	updateSelection();
}

void dlg_InSpectr::updateSelection()
{
	MdiChild* mdiChild = dynamic_cast<MdiChild*>(parent());

	if (m_activeFilter.empty())
	{
		mdiChild->setChannelRenderingEnabled(m_spectrumSelectionChannelID, false);
		return;
	}

	vtkSmartPointer<vtkImageData> result = m_xrfData->FilterSpectrum(m_activeFilter, static_cast<iAFilterMode>(comB_spectrumSelectionMode->currentIndex()));

	if (m_spectrumSelectionChannelID == NotExistingChannel)
		m_spectrumSelectionChannelID = mdiChild->createChannel();
	auto chData = mdiChild->channelData(m_spectrumSelectionChannelID);
	chData->setData(result, m_selection_ctf, m_selection_otf);
	// TODO: initialize channel?
	mdiChild->initChannelRenderer(m_spectrumSelectionChannelID, true);
	mdiChild->updateChannelOpacity(m_spectrumSelectionChannelID, 0.5);

	if (cb_spectraLines->isChecked())
	{
		// filter spectra lines by current filter - highlight those going through selection
		initSpectraLinesDrawer();
		m_spectrumDiagram->update();
	}

	mdiChild->updateViews();
}

void dlg_InSpectr::showLinkedElementMaps( int show )
{
	if (!m_elementConcentrations)
	{
		return;
	}
	MdiChild * mdiChild = (dynamic_cast<MdiChild*>(parent()));

	m_rendererManager.removeAll();
	m_rendererManager.addToBundle(mdiChild->renderer()->renderer());

	if (!show)
	{
		for (int i = 0; i < m_elementRenderers.size(); ++i)
		{
			m_elementRenderers[i]->removeObserver();
			delete m_elementRenderers[i];
		}
		m_elementRenderers.clear();
		return;
	}

	bool isFirst = true;
	for (size_t i=0; i<m_refSpectraLib->spectra.size(); ++i)
	{
		if (m_refSpectraLib->getItemModel()->item(i)->checkState() != Qt::Checked ||
			m_decomposeSelectedElements.indexOf(i) == -1)
		{
			continue;
		}

		//Create corresponding widget and visualization
		dlg_elementRenderer *elemRend = new dlg_elementRenderer( mdiChild );
		elemRend->SetRefLibIndex(i);
		InitElementRenderer( elemRend, i );
		bool slicerVisibility[3] = { false, false, false };
		elemRend->GetRenderer()->applySettings(mdiChild->renderSettings(), slicerVisibility);
		elemRend->ApplyVolumeSettings(mdiChild->volumeSettings());
		m_rendererManager.addToBundle(elemRend->GetRenderer()->renderer());
		m_elementRenderers.push_back( elemRend );
		if(isFirst)
			mdiChild->splitDockWidget(mdiChild->renderDockWidget(), elemRend, Qt::Horizontal);
		else
			mdiChild->splitDockWidget(m_elementRenderers[m_elementRenderers.size()-2], elemRend, Qt::Vertical);
		isFirst = false;
	}
}

void dlg_InSpectr::InitElementRenderer( dlg_elementRenderer * elemRend, size_t index )
{
	MdiChild * mdiChild = (dynamic_cast<MdiChild*>(parent()));

	//Derive data needed for visualization
	vtkSmartPointer<vtkImageData> chImgData = m_elementConcentrations->getImage(m_decomposeSelectedElements.indexOf(index));

	vtkSmartPointer<vtkColorTransferFunction> chCTF = vtkSmartPointer<vtkColorTransferFunction>::New();
	QColor color = m_refSpectraLib->getElementColor(index);
	chCTF->AddRGBPoint(0, color.redF(), color.greenF(), color.blueF());
	chCTF->AddRGBPoint(1, color.redF(), color.greenF(), color.blueF());

	vtkSmartPointer<vtkPiecewiseFunction> chOTF = vtkSmartPointer<vtkPiecewiseFunction>::New();
	chOTF->AddPoint(0.4, 0);
	chOTF->AddPoint(1, 0.1);

	QString chElemName = m_refSpectraLib->spectra[index].name();
	vtkPolyData * chPolyData = mdiChild->polyData();

	elemRend->setWindowTitle(chElemName);
	elemRend->SetDataToVisualize( chImgData, chPolyData, chOTF, chCTF );
}

void dlg_InSpectr::updateConcentrationOpacity(int newVal)
{
	if (cb_combinedElementMaps->checkState() != Qt::Checked)
		return;

	QModelIndexList indices = m_refSpectra->refSpectraListView->selectionModel()->selectedIndexes();
	if (indices.empty())
	{
		return;
	}
	m_refSpectraLib->setElementOpacity(indices[0], newVal);
	int channelIdx = m_refSpectraLib->getElementChannel(indices[0]);
	double opacity = (double)newVal / sl_concentrationOpacity->maximum();
	m_otf[channelIdx]->RemoveAllPoints();
	m_otf[channelIdx]->AddPoint(0.0, 0.0);
	m_otf[channelIdx]->AddPoint(1.0, opacity);
	vtkSmartPointer<vtkImageData> chImgData = m_elementConcentrations->getImage(channelIdx);
	(dynamic_cast<MdiChild*>(parent()))->updateChannel(m_channelIDs[channelIdx], chImgData, m_ctf[channelIdx], m_otf[channelIdx], true);
	(dynamic_cast<MdiChild*>(parent()))->updateViews();
}

void dlg_InSpectr::ReferenceSpectrumClicked( const QModelIndex &index )
{
	int opacity;
	if (m_refSpectraLib->getElementOpacity(index, opacity))
	{
		sl_concentrationOpacity->setValue(opacity);
	}
}

QSharedPointer<iAElementConcentrations> dlg_InSpectr::GetElementConcentrations()
{
	return m_elementConcentrations;
}

void dlg_InSpectr::showRefSpectraChanged( int show )
{
	if (!m_spectrumDiagram)
	{
		return;
	}
	if (!show && m_periodicTable->GetCurrentElement() != -1)
	{
		RemoveReferenceSpectrum(GetModelIdx(m_periodicTable->GetCurrentElement()));
	}
	for (size_t i=0; i<m_refSpectraLib->spectra.size(); ++i)
	{
		if (m_refSpectraLib->getItemModel()->item(i)->checkState() == Qt::Checked)
		{
			if(show)
			{
				AddReferenceSpectrum(i);
			}
			else
			{
				RemoveReferenceSpectrum(i);
			}
		}
	}
	m_spectrumDiagram->update();
}

void dlg_InSpectr::showRefLineChanged( int show )
{
	if (!m_spectrumDiagram)
	{
		return;
	}
	if (!show && m_periodicTable->GetCurrentElement() != -1)
	{
		RemoveElementLine(PeriodicTable::elements[m_periodicTable->GetCurrentElement()].shortname.c_str());
	}
	for (size_t i=0; i<m_refSpectraLib->spectra.size(); ++i)
	{
		if (m_refSpectraLib->getItemModel()->item(i)->checkState() == Qt::Checked)
		{
			if(show)
			{
				AddElementLine(m_refSpectraLib->spectra[i].GetSymbol());
			}
			else
			{
				RemoveElementLine(m_refSpectraLib->spectra[i].GetSymbol());
			}
		}
	}
	m_spectrumDiagram->update();
}

void dlg_InSpectr::computeSimilarityMap()
{

	QString fileName = QFileDialog::getSaveFileName(
		QApplication::activeWindow(),
		tr("Save Similarity Map"),
		QDir::currentPath(),
		tr("MetaImage (*.mhd);;")
	);
	if (fileName.isEmpty())
	{
		return;
	}
	//init resulting similarity map
	int numEBins = (int)m_xrfData->size();
	vtkSmartPointer<vtkImageData> similarityImageData = vtkSmartPointer<vtkImageData>::New();
	similarityImageData->SetDimensions(numEBins, numEBins, 1);
	similarityImageData->AllocateScalars(VTK_DOUBLE, 1);

	double * similarityData = static_cast <double*> ( similarityImageData->GetScalarPointer() );

	//initialization
	const int Dimensions = 3;//2;
	typedef float ScalarType;
	typedef itk::Image < ScalarType, 3 >  ImageType3D;
	typedef itk::Image < ScalarType, Dimensions >  ImageType;
	typedef itk::MutualInformationImageToImageMetric < ImageType, ImageType >  MutualInformationMetricType;
	typedef MutualInformationMetricType MetricType;
	typedef itk::LinearInterpolateImageFunction < ImageType, double > InterpolatorType;
	typedef itk::IdentityTransform<double, Dimensions>  TransformType;

	iAConnector * connectors = new iAConnector[numEBins];
	ImageType3D ** images = new ImageType3D*[numEBins];
	for (int i=0; i<numEBins; ++i)
	{
		connectors[i].setImage( ( *m_xrfData->GetDataPtr() )[i] );
		connectors[i].modified();
		images[i] = dynamic_cast <ImageType3D*> ( connectors[i].itkImage() );
	}

// 	//extract slice from 3D
// 	ExtractImageType::Pointer extractSliceFilter1, extractSliceFilter2;
// 	ImageType3D::RegionType desiredRegion;
// 	try
// 	{
// 		ImageType3D::IndexType desiredStart;
// 		desiredStart.Fill(0);
// 		ImageType3D::SizeType desiredSize;
// 		desiredSize[0] = ( *m_xrfData->GetDataPtr() )[0]->GetDimensions()[0];
// 		desiredSize[1] = ( *m_xrfData->GetDataPtr() )[0]->GetDimensions()[1];
// 		desiredSize[2] = 0;
// 		desiredRegion = ImageType3D::RegionType(desiredStart, desiredSize);
//
// 		extractSliceFilter1 = ExtractImageType::New();
// 		extractSliceFilter2 = ExtractImageType::New();
// 		extractSliceFilter1->SetDirectionCollapseToIdentity();
// 		extractSliceFilter2->SetDirectionCollapseToIdentity();
// 	}
// 	catch (itk::ExceptionObject & excp)
// 	{
// 		(dynamic_cast<MdiChild*>(parent()))->addMsg("Exception in computeSimilarityMap(): " + QString(excp.GetDescription()));
// 		delete [] connectors;
// 		delete [] images;
// 		return;
// 	}


	const unsigned int numSamples = 2500;
	MdiChild * mdiChild = dynamic_cast <MdiChild*> ( parent() );
	double numIterations = numEBins * numEBins * 0.5;
	double curIteration = 0.0; int percentage = 0;
	mdiChild->addStatusMsg("Computing Similarity Map");
	mdiChild->initProgressBar();
	QCoreApplication::processEvents();
	int errorCount = 0;
	QStringList errDescr;

	//iteration
	//#pragma omp parallel for shared(similarityData)
	for (int i=0; i<numEBins; ++i)
	{
		try
		{
			MetricType::Pointer metric = MetricType::New();
			InterpolatorType::Pointer interpolator = InterpolatorType::New();
			TransformType::Pointer transform = TransformType::New();
			TransformType::ParametersType params(transform->GetNumberOfParameters());

			similarityData[i + i*numEBins] = 1.0f;
			params.Fill(0.0);
			//extractSliceFilter1->SetInput( images[i] ); extractSliceFilter1->SetExtractionRegion(desiredRegion); extractSliceFilter1->Update();
			//interpolator->SetInputImage( extractSliceFilter1->GetOutput() );
			interpolator->SetInputImage( images[i] );
			interpolator->Modified();

			for (int j=0; j<i; ++j)
			{
				//extractSliceFilter2->SetInput( images[j] ); extractSliceFilter2->SetExtractionRegion(desiredRegion); extractSliceFilter2->Update();
				metric->SetNumberOfSpatialSamples(numSamples);
				metric->SetFixedImage ( images[i] );//metric->SetFixedImage ( extractSliceFilter1->GetOutput() );
				metric->SetMovingImage( images[j] );//metric->SetMovingImage ( extractSliceFilter2->GetOutput() );
				metric->SetFixedImageRegion( images[i]->GetLargestPossibleRegion() );//metric->SetFixedImageRegion( extractSliceFilter1->GetOutput()->GetLargestPossibleRegion() );
				metric->SetTransform(transform);
				metric->SetInterpolator(interpolator);
				metric->Initialize();
				double metricValue = metric->GetValue(params);
				similarityData[i + j*numEBins] = similarityData[j + i*numEBins] = metricValue;
				curIteration++; int newPercentage = 100 * curIteration / numIterations;
				if(newPercentage != percentage)
				{
					percentage = newPercentage;
					mdiChild->updateProgressBar( percentage );
					QCoreApplication::processEvents();
				}
			}
		}
		catch (itk::ExceptionObject & excp)
		{
			errorCount++;
			errDescr.append( QString(excp.GetDescription()) );
		}
	}
	try
	{
		vtkSmartPointer<vtkMetaImageWriter> writer = vtkSmartPointer<vtkMetaImageWriter>::New();
		writer->SetCompression(false);
		writer->SetInputData(similarityImageData);
		writer->SetFileName( getLocalEncodingFileName(fileName).c_str() );
		writer->Write();
		writer->Update();
	}
	catch (itk::ExceptionObject & excp)
	{
		(dynamic_cast<MdiChild*>(parent()))->addMsg( "Exception in computeSimilarityMap(): " + QString(excp.GetDescription()) );
	}

	delete [] connectors;
	delete [] images;

	mdiChild->hideProgressBar();
	QCoreApplication::processEvents();

	for (int i=0; i<errorCount; ++i)
		(dynamic_cast<MdiChild*>(parent()))->addMsg("Exception in computeSimilarityMap(): " + errDescr[i]);
}

void dlg_InSpectr::energyBinsSelected( int binX, int binY )
{
	m_selectedBinXDrawer->setPosition( binX );
	m_selectedBinYDrawer->setPosition( binY );
	m_spectrumDiagram->update();
}

void dlg_InSpectr::updateDecompositionGUI( QStringList elementsNames )
{
	m_decomposeSelectedElements.clear();
	m_decomposeSelectedElements.resize( elementsNames.size() );

	QVector<iAElementSpectralInfo*> elementSpectra;

	size_t colorIdx = 0;
	iAColorTheme const * theme = iAColorThemeManager::instance().theme( "Brewer Set1 (max. 9)" );
	for ( size_t i = 0; i < m_refSpectraLib->spectra.size(); ++i )
	{
		int pos = elementsNames.indexOf( m_refSpectraLib->spectra[i].name() );
		if ( pos != -1 )
		{
			m_decomposeSelectedElements[pos] = i;
			elementSpectra.push_back( &m_refSpectraLib->spectra[i] );
			m_refSpectraLib->getItemModel()->item( i )->setCheckState( Qt::Checked );
			if ( colorIdx < theme->size() )
			{
				m_refSpectraLib->getItemModel()->item( i )->setData( theme->color( colorIdx ), Qt::DecorationRole );
			}
			++colorIdx;
		}
		else
		{
			m_refSpectraLib->getItemModel()->item( i )->setCheckState( Qt::Unchecked );
		}
	}

	if ( m_accumulatedXRF )
	{
		m_elementConcentrations->calculateAverageConcentration( m_xrfData, elementSpectra, m_accumulatedXRF );
	}

	decompositionAvailable();
}

void dlg_InSpectr::AddElementLine(QString const & symbol)
{
	if (!m_spectrumDiagram)
	{
		return;
	}
	QColor color(255, 0, 0);
	if (GetModelIdx(symbol) != -1)
	{
		color = m_refSpectraLib->getElementColor(GetModelIdx(symbol));
	}
	int idx = findCharEnergy(m_characteristicEnergies, symbol);
	if (idx != -1)
	{
		m_spectrumDiagram->AddElementLines(&m_characteristicEnergies[idx], color);
	}
	m_spectrumDiagram->update();
}

void dlg_InSpectr::RemoveElementLine(QString const & symbol)
{
	if (!m_spectrumDiagram)
	{
		return;
	}
	int idx = findCharEnergy(m_characteristicEnergies, symbol);
	if (idx != -1)
	{
		m_spectrumDiagram->RemoveElementLines(&m_characteristicEnergies[idx]);
	}
	m_spectrumDiagram->update();
}

void dlg_InSpectr::AddReferenceSpectrum(int modelIdx)
{
	if (!m_spectrumDiagram)
	{
		return;
	}
	if (modelIdx == -1)
	{
		return;
	}
	if (m_refSpectraDrawers.contains(modelIdx))
	{
		RemoveReferenceSpectrum(modelIdx);
	}
	QVector<float> const & energies = m_refSpectraLib->spectra[modelIdx].GetEnergyData();
	QSharedPointer<iAMappingDiagramData> plotData(new iAMappingDiagramData(
		&m_refSpectraLib->spectra[modelIdx].GetCountsData()[0],
		energies.size(), energies[0], energies[energies.size()-1],
		m_xrfData->size(), m_xrfData->GetMinEnergy(), m_xrfData->GetMaxEnergy(),
		m_accumulatedXRF->yBounds()[1]));
	QColor color = m_refSpectraLib->getElementColor(modelIdx);
	QSharedPointer<iAStepFunctionPlot> drawable(new iAStepFunctionPlot(plotData, color));
	m_refSpectraDrawers.insert(modelIdx, drawable);
	m_spectrumDiagram->addPlot(drawable);
	m_spectrumDiagram->update();
}

void dlg_InSpectr::RemoveReferenceSpectrum(int modelIdx)
{
	if (!m_spectrumDiagram)
	{
		return;
	}
	if (modelIdx == -1)
	{
		return;
	}
	if (m_refSpectraDrawers.contains(modelIdx))
	{
		m_spectrumDiagram->removePlot(m_refSpectraDrawers[modelIdx]);
		m_refSpectraDrawers.remove(modelIdx);
	}
	m_spectrumDiagram->update();
}

bool dlg_InSpectr::ShowElementLines() const
{
	return m_refSpectra->cb_showRefLines->isChecked();
}

bool dlg_InSpectr::ShowReferenceSpectra() const
{
	return m_refSpectra->cb_showRefSpectra->isChecked();
}

int dlg_InSpectr::GetModelIdx(int elemIdx) const
{
	return GetModelIdx(PeriodicTable::elements[elemIdx].shortname.c_str());
}

int dlg_InSpectr::GetModelIdx(QString const & symbol) const
{
	for (size_t i=0; i<m_refSpectraLib->spectra.size(); ++i)
	{
		if (m_refSpectraLib->spectra[i].GetSymbol() == symbol)
		{
			return i;
		}
	}
	return -1;
}

bool dlg_InSpectr::IsElementSelected(int elemIdx) const
{
	int modelIdx = GetModelIdx(elemIdx);
	if (modelIdx == -1)
		return false;
	return m_refSpectraLib->getItemModel()->item(modelIdx)->checkState() == Qt::Checked;
}

//  { Start Slicer Pie Glyphs

void dlg_InSpectr::showPieGlyphsSettings(bool isChecked)
{
	if (isChecked)
		gb_pieGlyphsSettings->show();
	else
		gb_pieGlyphsSettings->hide();
}

void dlg_InSpectr::updatePieGlyphParameters(int /*newVal*/)
{
	updatePieGlyphParamsInternal();
	updateAllPieGlyphs();
}

void dlg_InSpectr::updatePieGlyphParamsInternal()
{
	m_pieGlyphOpacity = (double)sl_pieGlyphsOpacity->value() / sl_pieGlyphsOpacity->maximum();
	m_pieGlyphSpacing = 1.0 - (double)sl_pieGlyphsSpacing->value() / sl_pieGlyphsSpacing->maximum();
	m_pieGlyphMagFactor = (double)sl_pieGlyphResolution->value() / sl_pieGlyphResolution->maximum() * 0.7;
}

void dlg_InSpectr::pieGlyphsVisualization(int show)
{
	bool isOn = (bool)show;
	updatePieGlyphParamsInternal();
	setSlicerPieGlyphsOn(isOn);
	tb_pieGlyphSettings->setEnabled(isOn);
	if (isOn)
		showPieGlyphsSettings(tb_pieGlyphSettings->isChecked());
	else
		gb_pieGlyphsSettings->hide();
}

void dlg_InSpectr::setSlicerPieGlyphsOn(bool isOn)
{
	if (m_pieGlyphsEnabled == isOn)
		return;
	m_pieGlyphsEnabled = isOn;
	auto child = dynamic_cast<MdiChild*>(parent());
	for (int slicerMode = 0; slicerMode < iASlicerMode::SlicerCount; ++slicerMode)
	{
		if (isOn)
			connect(child->slicer(slicerMode), &iASlicer::sliceNumberChanged, this, &dlg_InSpectr::updatePieGlyphs);
		else
			disconnect(child->slicer(slicerMode), &iASlicer::sliceNumberChanged, this, &dlg_InSpectr::updatePieGlyphs);
	}
	updateAllPieGlyphs();
}

void dlg_InSpectr::updateAllPieGlyphs()
{
	for (int slicerMode = 0; slicerMode < iASlicerMode::SlicerCount; ++slicerMode)
	{
		updatePieGlyphs(slicerMode);
	}
}

void dlg_InSpectr::updatePieGlyphs(int slicerMode)
{
	const double EPSILON = 0.0015;
	auto child = dynamic_cast<MdiChild*>(parent());
	auto renWin = child->slicer(slicerMode)->renderWindow();
	auto ren = renWin->GetRenderers()->GetFirstRenderer();
	bool hasPieGlyphs = (m_pieGlyphs[slicerMode].size() > 0);
	if (hasPieGlyphs)
	{
		for (int i = 0; i < m_pieGlyphs[slicerMode].size(); ++i)
		{
			ren->RemoveActor(m_pieGlyphs[slicerMode][i]->actor);
		}
		m_pieGlyphs[slicerMode].clear();
	}

	if (!m_pieGlyphsEnabled)
	{
		if (hasPieGlyphs)
		{
			renWin->GetInteractor()->Render();
		}
		return;
	}

	QVector<double> angleOffsets;

	for (size_t chan = 0; chan < m_channelIDs.size(); ++chan)
	{
		if (!child->slicer(slicerMode)->hasChannel(m_channelIDs[chan]))
		{
			continue;
		}
		iAChannelSlicerData * chSlicerData = child->slicer(slicerMode)->channel(m_channelIDs[chan]);
		vtkSmartPointer<vtkImageResample> resampler = vtkSmartPointer<vtkImageResample>::New();
		resampler->SetInputConnection(chSlicerData->reslicer()->GetOutputPort());
		resampler->InterpolateOn();
		resampler->SetAxisMagnificationFactor(0, m_pieGlyphMagFactor);
		resampler->SetAxisMagnificationFactor(1, m_pieGlyphMagFactor);
		resampler->SetAxisMagnificationFactor(2, m_pieGlyphMagFactor);
		resampler->Update();

		vtkImageData * imgData = resampler->GetOutput();

		int dims[3];
		imgData->GetDimensions(dims);
		QString scalarTypeStr(imgData->GetScalarTypeAsString());

		double origin[3], spacing[3];
		imgData->GetOrigin(origin); imgData->GetSpacing(spacing);

		int index = 0;
		for (int y = 0; y < dims[1]; y++)
		{
			for (int x = 0; x < dims[0]; ++x, ++index)
			{
				float portion = static_cast<float*>(imgData->GetScalarPointer(x, y, 0))[0];
				double angularRange[2] = { 0.0, 360.0*portion };
				if (0 != chan)
				{
					angularRange[0] += angleOffsets[index];
					angularRange[1] += angleOffsets[index];
				}

				if (portion > EPSILON)
				{
					auto pieGlyph = QSharedPointer<iAPieChartGlyph>(new iAPieChartGlyph(angularRange[0], angularRange[1]));
					double pos[3] = { origin[0] + x * spacing[0], origin[1] + y * spacing[1], 1.0 };
					pieGlyph->actor->SetPosition(pos);
					pieGlyph->actor->SetScale((std::min)(spacing[0], spacing[1]) * m_pieGlyphSpacing);
					QColor c(m_channelColors[chan]);
					double color[3] = { c.redF(), c.greenF(), c.blueF() };
					pieGlyph->actor->GetProperty()->SetColor(color);
					pieGlyph->actor->GetProperty()->SetOpacity(m_pieGlyphOpacity);
					ren->AddActor(pieGlyph->actor);
					m_pieGlyphs[slicerMode].push_back(pieGlyph);
				}

				if (0 == chan)
				{
					angleOffsets.push_back(angularRange[1]);
				}
				else
				{
					angleOffsets[index] = angularRange[1];
				}
			}
		}
	}
	renWin->GetInteractor()->Render();
}

// } End Slicer Pie Glyphs