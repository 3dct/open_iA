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
#include "dlg_XRF.h"

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
#include "iAPieChartWidget.h"
#include "iAReferenceSpectraLibrary.h"
#include "iAXRFData.h"
#include "iAXRFOverlay.h"

#include <charts/iAPlotTypes.h>
#include <charts/iAMappingDiagramData.h>
#include <dlg_transfer.h>
#include <iAChannelData.h>
#include <iAColorTheme.h>
#include <iAConnector.h>
#include <iAFunctionalBoxplot.h>
#include <iAMathUtility.h>
#include <iARenderer.h>
#include <iAVtkWidget.h>
#include <io/iAFileUtils.h>
#include <io/iAIO.h>
#include <qthelper/iAWidgetAddHelper.h>
#include <mdichild.h>
#include <qthelper/iADockWidgetWrapper.h>

#include <itkLabelStatisticsImageFilter.h>
#include <itkImageBase.h>
#include <itkImage.h>
#include <itkIdentityTransform.h>
#include <itkMeanSquaresImageToImageMetric.h>
#include <itkMutualInformationImageToImageMetric.h>
#include <itkNormalizedCorrelationImageToImageMetric.h>
#include <itkLinearInterpolateImageFunction.h>
#include <itkTranslationTransform.h>
#include <itkExtractImageFilter.h>
#include <itkImageMaskSpatialObject.h>

#include <vtkColorTransferFunction.h>
#include <vtkDiscretizableColorTransferFunction.h>
#include <vtkImageData.h>
#include <vtkInteractorStyleImage.h>
#include <vtkLookupTable.h>
#include <vtkMath.h>
#include <vtkMetaImageWriter.h>
#include <vtkOpenGLRenderer.h>
#include <vtkPiecewiseFunction.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkScalarBarActor.h>
#include <vtkTextProperty.h>
#include <vtkTransform.h>

#include <QColorDialog>
#include <QFileDialog>
#include <QMapIterator>


dlg_XRF::dlg_XRF(QWidget *parentWidget, dlg_periodicTable* dlgPeriodicTable, dlg_RefSpectra* dlgRefSpectra):
	dlg_xrfContainer(parentWidget),
	m_initialized(false),
	m_ctfChanged(true),
	m_decompositionLoaded(false),
	m_spectrumDiagram(NULL),
	m_accumulatedGridLayout(NULL),
	m_oTF(vtkSmartPointer<vtkPiecewiseFunction>::New()),
	m_cTF(vtkSmartPointer<vtkColorTransferFunction>::New()),
	m_xrfData(new iAXRFData),
	m_spectraHistogramColormap(QString::fromUtf8(":/images/colormap.png")),
	m_enabledChannels(0),
	m_periodicTable(dlgPeriodicTable),
	m_selection_ctf(vtkSmartPointer<vtkColorTransferFunction>::New()),
	m_selection_otf(vtkSmartPointer<vtkPiecewiseFunction>::New()),
	m_periodicTableListener(new iAPeriodicTableListener(this)),
	m_refSpectra(dlgRefSpectra),
	m_spectrumSelectionChannelID(NotExistingChannel)
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

	connect ( comB_AccumulateFunction, SIGNAL( currentIndexChanged(int) ), this, SLOT( updateAccumulate(int) ) );
	connect ( comB_spectrumSelectionMode, SIGNAL( currentIndexChanged(int) ), this, SLOT( updateSelectionMode(int) ) );
	connect ( cb_spectraLines, SIGNAL( stateChanged(int) ), this, SLOT( showSpectraLines(int) ) );
	connect ( cb_spectraHistograms, SIGNAL( stateChanged(int) ), this, SLOT(showSpectraHistograms(int)) );
	connect ( tb_spectraSettings, SIGNAL( toggled(bool) ), this, SLOT(showSpectraHistogramsSettings(bool)) );
	connect ( cb_spectrumProbing, SIGNAL( stateChanged(int) ), this, SLOT( showVoxelSpectrum(int) ) );
	connect ( pb_decompose, SIGNAL( clicked() ), this, SLOT( decomposeElements() ) );
	connect ( pb_decompositionLoad, SIGNAL( clicked() ), this, SLOT( loadDecomposition() ) );
	connect ( pb_decompositionStore, SIGNAL( clicked() ), this, SLOT( storeDecomposition() ) );
	connect ( cb_combinedElementMaps, SIGNAL(stateChanged(int)), this, SLOT(combinedElementMaps(int)));
	connect ( sl_specHistSensitivity, SIGNAL( valueChanged(int) ), this, SLOT(spectraHistSensitivityChanged(int) ) );
	connect ( pb_recompute, SIGNAL( clicked() ), this, SLOT(recomputeSpectraHistograms() ) );
	connect ( cb_smoothOpacFade, SIGNAL( stateChanged(int) ), this, SLOT( smoothOpacityFadeChecked(int) ) );
	connect ( sl_specHistOpacThreshold, SIGNAL( valueChanged(int) ), this, SLOT( spectraOpacityThresholdChanged(int) ) );
	connect ( comB_colormap, SIGNAL( currentIndexChanged(int) ), this, SLOT( changeColormap(int) ) );
	connect ( cb_linkedElementMaps, SIGNAL( stateChanged(int) ), this, SLOT( showLinkedElementMaps(int) ) );
	connect ( cb_pieChartGlyphs, SIGNAL( stateChanged(int) ), this, SLOT( pieGlyphsVisualization(int) ) );
	connect ( tb_pieGlyphSettings, SIGNAL( toggled(bool) ), this, SLOT(showPieGlyphsSettings(bool)) );
	connect ( sl_pieGlyphsOpacity, SIGNAL( valueChanged(int) ), this, SLOT( updatePieGlyphParameters(int) ) );
	connect ( sl_pieGlyphsSpacing, SIGNAL( valueChanged(int) ), this, SLOT( updatePieGlyphParameters(int) ) );
	connect ( sl_pieGlyphResolution, SIGNAL( valueChanged(int) ), this, SLOT( updatePieGlyphParameters(int) ) );
	connect ( sl_concentrationOpacity, SIGNAL( valueChanged(int) ), this, SLOT( updateConcentrationOpacity(int) ) );
	connect ( cb_aggregatedSpectrum, SIGNAL( stateChanged(int) ), this, SLOT( showAggregatedSpectrum(int) ) );
	connect ( cb_functionalBoxplot, SIGNAL( stateChanged(int) ), this, SLOT( updateFunctionalBoxplot(int) ) );
	connect ( rb_DrawMode_Log, SIGNAL( toggled(bool) ), this, SLOT( setLogDrawMode(bool) ) );
	connect ( rb_DrawMode_Lin, SIGNAL( toggled(bool) ), this, SLOT( setLinDrawMode(bool) ) );
	connect ( pb_computeSimilarityMap, SIGNAL( clicked() ), this, SLOT( computeSimilarityMap() ) );
}

void dlg_XRF::AddSimilarityMarkers()
{
	m_spectrumDiagram->addPlot( m_selectedBinXDrawer );
	m_spectrumDiagram->addPlot( m_selectedBinYDrawer );
	m_spectrumDiagram->update();
}

void dlg_XRF::RemoveSimilarityMarkers()
{
	m_spectrumDiagram->removePlot( m_selectedBinXDrawer );
	m_spectrumDiagram->removePlot( m_selectedBinYDrawer );
	m_spectrumDiagram->update();
}

void dlg_XRF::init(double minEnergy, double maxEnergy, bool haveEnergyLevels,
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

	connect((dlg_transfer*)(m_spectrumDiagram->getFunctions()[0]), SIGNAL(Changed()), this, SLOT(SpectrumTFChanged()));
	iADockWidgetWrapper* spectrumChartContainer = new iADockWidgetWrapper(m_spectrumDiagram, "Spectrum View", "SpectrumChartWidget");
	spectrumChartContainer->setContentsMargins(0, 0, 0, 0);

	InitCommonGUI(widgetAddHelper);
	widgetAddHelper.m_mdiChild->getLogDlg()->show();
	widgetAddHelper.SplitWidget(spectrumChartContainer, widgetAddHelper.m_mdiChild->getLogDlg(), Qt::Vertical);
	widgetAddHelper.m_mdiChild->getLogDlg()->hide();
	widgetAddHelper.SplitWidget(m_pieChartContainer, spectrumChartContainer);

	m_ctfChanged  = true;
	m_initialized = true;

	m_colormapRen = vtkSmartPointer<vtkRenderer>::New();
	m_colormapRen->SetBackground(1.0, 1.0, 1.0);

	CREATE_OLDVTKWIDGET(m_colormapWidget);
	horizontalLayout_8->insertWidget(0, m_colormapWidget);
	m_colormapWidget->GetRenderWindow()->AddRenderer(m_colormapRen);
	vtkSmartPointer<vtkInteractorStyleImage> style = vtkSmartPointer<vtkInteractorStyleImage>::New();
	m_colormapWidget->GetInteractor()->SetInteractorStyle(style);

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
	m_colormapWidget->GetRenderWindow()->Render();

	m_refSpectra->cb_showRefSpectra->setEnabled(true);
	m_refSpectra->cb_showRefLines->setEnabled(true);
	pb_decompose->setEnabled(true);
}

void dlg_XRF::InitElementMaps(/* QSharedPointer<iAElementConcentrations> conc */iAWidgetAddHelper & widgetAddHelper)
{
	InitCommonGUI(widgetAddHelper);
	widgetAddHelper.SplitWidget(m_pieChartContainer, m_periodicTable, Qt::Vertical);
}

void dlg_XRF::InitCommonGUI(iAWidgetAddHelper & widgetAddHelper)
{
	m_periodicTable->setListener(m_periodicTableListener);


	// load reference spectra & characteristic energy lines:
	QString rootDir("C:/refSpectra/");
	m_refSpectraLib = QSharedPointer<iAReferenceSpectraLibrary>(new iAReferenceSpectraLibrary(
		rootDir + "elementSpectra/reference_library.reflib"));
	m_refSpectra->getSpectraList()->setModel(m_refSpectraLib->getItemModel().data());
	EnergyLoader::Load(rootDir + "characteristic-energies.cel", m_characteristicEnergies);
	connect(m_refSpectra->getSpectraList(), SIGNAL(doubleClicked(QModelIndex)), this, SLOT(ReferenceSpectrumDoubleClicked(QModelIndex)), Qt::UniqueConnection);
	connect(m_refSpectra->getSpectraList(), SIGNAL(clicked(QModelIndex)), this, SLOT(ReferenceSpectrumClicked(QModelIndex)), Qt::UniqueConnection );
	connect(m_refSpectra->getSpectraList()->model(), SIGNAL(itemChanged ( QStandardItem *)), this, SLOT(ReferenceSpectrumItemChanged( QStandardItem *)), Qt::UniqueConnection);
	connect(m_refSpectra->cb_showRefSpectra, SIGNAL( stateChanged(int) ), this, SLOT( showRefSpectraChanged(int) ) );
	connect(m_refSpectra->cb_showRefLines, SIGNAL( stateChanged(int) ), this, SLOT( showRefLineChanged(int) ) );

	m_pieChart = new iAPieChartWidget(this);
	m_pieChart->setObjectName(QString::fromUtf8("Composition"));
	m_pieChartContainer = new iADockWidgetWrapper(m_pieChart, "Element Concentration", "PieChartWidget");
	m_pieChartContainer->setContentsMargins(0, 0, 0, 0);
	//m_pieChartContainer->hide();
	widgetAddHelper.SplitWidget(m_periodicTable, this, Qt::Vertical);
	widgetAddHelper.TabWidget(m_refSpectra, this);
}

void dlg_XRF::setLogDrawMode(bool checked)
{
	if (checked)
	{
		m_spectrumDiagram->setYMappingMode(iAEnergySpectrumWidget::Logarithmic);
		m_spectrumDiagram->update();
	}
}

void dlg_XRF::setLinDrawMode(bool checked)
{
	if (checked)
	{
		m_spectrumDiagram->setYMappingMode(iAEnergySpectrumWidget::Linear);
		m_spectrumDiagram->update();
	}
}

bool dlg_XRF::IsInitialized()
{
	return m_initialized;
}

vtkSmartPointer<vtkImageData> dlg_XRF::GetCombinedVolume()
{
	return m_xrfData->GetCombinedVolume();
}

vtkSmartPointer<vtkColorTransferFunction> dlg_XRF::GetColorTransferFunction()
{
	return m_xrfData->GetColorTransferFunction();
}

QObject* dlg_XRF::UpdateForVisualization()
{
	if (m_ctfChanged)
	{
		m_ctfChanged = false;
		return m_xrfData->UpdateCombinedVolume(m_cTF);
	}
	return 0;
}

QSharedPointer<iAXRFData> dlg_XRF::GetXRFData()
{
	return m_xrfData;
}

void dlg_XRF::updateComposition(QVector<double> const & concentration)
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
				.arg(m_refSpectraLib->spectra[m_decomposeSelectedElements[i]].GetName())
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

void dlg_XRF::UpdateVoxelSpectrum(int x, int y, int z)
{
	m_voxelEnergy->updateEnergyFunction(x, y, z);
	m_spectrumDiagram->update();
}

void dlg_XRF::UpdateConcentrationViews( int x, int y, int z )
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

void dlg_XRF::SpectrumTFChanged()
{
	m_ctfChanged = true;
}

void dlg_XRF::updateAccumulate(int fctIdx)
{
	m_accumulatedXRF->SetFct(fctIdx);
	m_spectrumDiagram->update();
}

void dlg_XRF::initSpectraLinesDrawer()
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

void dlg_XRF::initSpectraOverlay()
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
	m_colormapWidget->GetRenderWindow()->Render();
}

void dlg_XRF::showSpectraLines(int show)
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

void dlg_XRF::showSpectraHistograms( int show )
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

void dlg_XRF::showSpectraHistogramsSettings( bool isChecked )
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

void dlg_XRF::showVoxelSpectrum(int show)
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

void dlg_XRF::showAggregatedSpectrum( int show )
{
	m_spectrumDiagram->plots()[0]->setVisible(show);
	m_spectrumDiagram->update();
}

void dlg_XRF::updateFunctionalBoxplot(int show)
{
	if (show)
	{
		m_functionalBoxplotImage = drawFunctionalBoxplot(m_accumulatedXRF->GetFunctionalBoxPlot(),
			m_xrfData->size(),
			m_accumulatedXRF->YBounds()[1]);
		m_spectrumDiagram->addImageOverlay(m_functionalBoxplotImage);
	}
	else
	{
		m_spectrumDiagram->removeImageOverlay(m_functionalBoxplotImage.data());
	}
	m_spectrumDiagram->update();
}

void dlg_XRF::ReferenceSpectrumDoubleClicked( const QModelIndex &index )
{
	QColor initCol = m_refSpectraLib->getElementColor(index);
	QColor newColor = QColorDialog::getColor(initCol, this, "New color for the reference spectrum", QColorDialog::ShowAlphaChannel);
	if(newColor.isValid())
	{
		m_refSpectraLib->getItemModel()->itemFromIndex(index)->setData(newColor, Qt::DecorationRole);
		
		for (size_t i=0; i<m_elementRenderers.size(); ++i)
		{
			if( m_elementRenderers[i]->GetRefLibIndex() == index.row() )
				InitElementRenderer(m_elementRenderers[i], index.row());
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

void dlg_XRF::ReferenceSpectrumItemChanged( QStandardItem * item )
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
	long elemRendInd = -1;
	for (size_t i=0; i<m_elementRenderers.size(); ++i)
	{
		if( m_elementRenderers[i]->GetRefLibIndex() == indRow )
		{
			elemRendInd = (long)i;
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

void dlg_XRF::decomposeElements()
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
	connect(m_decompositionCalculator.data(), SIGNAL( success() ), this, SLOT (decompositionSuccess()) );
	connect(m_decompositionCalculator.data(), SIGNAL( finished() ), this, SLOT (decompositionFinished()) );
	connect(m_decompositionCalculator.data(), SIGNAL( progress(int) ), dynamic_cast<MdiChild*>(parent()), SLOT(updateProgressBar(int)) );
	m_decompositionCalculator->start();
	(dynamic_cast<MdiChild*>(parent()))->addMsg(tr("Decomposition calculation started..."));
}

void dlg_XRF::decompositionSuccess()
{
	(dynamic_cast<MdiChild*>(parent()))->addMsg(tr("Decomposition calculation successful."));
	decompositionAvailable();
}

void dlg_XRF::decompositionAvailable()
{
	if (m_elementConcentrations->hasAvgConcentration())
	{
		updateComposition(m_elementConcentrations->getAvgConcentration());
	}
	enableControlsNeedingDecompositionData();
}

void dlg_XRF::decompositionFinished()
{
	m_decompositionCalculator.clear();
	(dynamic_cast<MdiChild*>(parent()))->hideProgressBar();
	pb_decompose->setText("Calculate");
}

void dlg_XRF::loadDecomposition()
{
	if (!m_refSpectraLib)
	{
		(dynamic_cast<MdiChild*>(parent()))->addMsg(tr("Reference spectra have to be loaded!"));
		return;
	}
	QString fileName = QFileDialog::getOpenFileName(
		QApplication::activeWindow(),
		tr("Load File"),
		(dynamic_cast<MdiChild*>(parent()))->getFilePath(),
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
		(dynamic_cast<MdiChild*>(parent()))->getLogger(),
		dynamic_cast<MdiChild*>(parent()),
		m_elementConcentrations->getImageListPtr()
	);
	io.setupIO(VOLUME_STACK_VOLSTACK_READER, fileName);
	io.start();
	io.wait();

	QString elementNames = io.getAdditionalInfo();
	QStringList elements = elementNames.split(",");
	
	elements.replaceInStrings(QRegExp("^\\s+"), ""); // trim whitespaces
	updateDecompositionGUI( elements );
}

void dlg_XRF::enableControlsNeedingDecompositionData()
{
	m_decompositionLoaded = true;
	pb_decompositionStore->setEnabled(true);
	cb_combinedElementMaps->setEnabled(true);
	cb_linkedElementMaps->setEnabled(true);
}

void dlg_XRF::storeDecomposition()
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
		elementInfo.append(m_refSpectraLib->spectra[m_decomposeSelectedElements[i]].GetName());
		if (i < m_decomposeSelectedElements.size()-1)
		{
			elementInfo.append(",");
		}
	}

	iAIO io(
		(dynamic_cast<MdiChild*>(parent()))->getLogger(),
		dynamic_cast<MdiChild*>(parent()),
		m_elementConcentrations->getImageListPtr());

	io.setupIO(VOLUME_STACK_VOLSTACK_WRITER, fileName);
	io.setAdditionalInfo(elementInfo);

	io.start();
	io.wait();
}

void dlg_XRF::combinedElementMaps(int show)
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
		if (m_channelIDs.size() <= i)
			m_channelIDs.push_back(mdiChild->createChannel());
		auto chData = mdiChild->getChannelData(m_channelIDs[i]);
		vtkSmartPointer<vtkImageData> chImgData = m_elementConcentrations->getImage(m_decomposeSelectedElements.indexOf(i));
		QColor color = m_refSpectraLib->getElementColor(i);
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

		chData->setColor(color);
		chData->setData(chImgData, m_ctf[m_enabledChannels], m_otf[m_enabledChannels]);
		// TODO: initialize channel?
		mdiChild->initChannelRenderer(m_channelIDs[i], false);
		mdiChild->updateChannelOpacity(m_channelIDs[i], 1);

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

void dlg_XRF::pieGlyphsVisualization( int show )
{
	bool isOn = (bool) show;
	MdiChild * mdiChild = (dynamic_cast<MdiChild*>(parent()));
	updatePieGlyphParameters();
	mdiChild->setSlicerPieGlyphsEnabled(isOn);
	tb_pieGlyphSettings->setEnabled(isOn);
	if(isOn)
		showPieGlyphsSettings( tb_pieGlyphSettings->isChecked() );
	else
		gb_pieGlyphsSettings->hide();
}

void dlg_XRF::recomputeSpectraHistograms()
{
	if(!m_spectraHistogramImage.isNull())
		m_spectrumDiagram->removeImageOverlay(m_spectraHistogramImage.data());
	initSpectraOverlay();
	m_spectrumDiagram->update();
}

void dlg_XRF::spectraHistSensitivityChanged( int newVal )
{
	sl_specHistSensitivity->repaint();
	recomputeSpectraHistograms();
}

void dlg_XRF::smoothOpacityFadeChecked( int checked )
{
	recomputeSpectraHistograms();
}

void dlg_XRF::spectraOpacityThresholdChanged( int newVal )
{
	sl_specHistOpacThreshold->repaint();
	recomputeSpectraHistograms();
}

void dlg_XRF::changeColormap( int colormapInd )
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

void dlg_XRF::OnSelectionUpdate(QVector<iASpectrumFilter> const & filter)
{
	m_activeFilter = filter;
	updateSelection();
}

void dlg_XRF::updateSelectionMode(int modeIdx)
{
	updateSelection();
}

void dlg_XRF::updateSelection()
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
	auto chData = mdiChild->getChannelData(m_spectrumSelectionChannelID);
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

void dlg_XRF::showLinkedElementMaps( int show )
{
	if (!m_elementConcentrations)
	{
		return;
	}
	MdiChild * mdiChild = (dynamic_cast<MdiChild*>(parent()));

	m_rendererManager.removeAll();
	m_rendererManager.addToBundle(mdiChild->getRenderer()->GetRenderer());

	if (!show)
	{
		for (size_t i = 0; i < m_elementRenderers.size(); ++i)
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
		mdiChild->applyRenderSettings( elemRend->GetRenderer() );
		elemRend->ApplyVolumeSettings(mdiChild->getVolumeSettings());
		m_rendererManager.addToBundle(elemRend->GetRenderer()->GetRenderer());
		m_elementRenderers.push_back( elemRend );
		if(isFirst)
			mdiChild->splitDockWidget(mdiChild->getRendererDlg(), elemRend, Qt::Horizontal);
		else
			mdiChild->splitDockWidget(m_elementRenderers[m_elementRenderers.size()-2], elemRend, Qt::Vertical);
		isFirst = false;
	}
}

void dlg_XRF::InitElementRenderer( dlg_elementRenderer * elemRend, size_t index )
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

	QString chElemName = m_refSpectraLib->spectra[index].GetName();
	vtkPolyData * chPolyData = mdiChild->getPolyData();

	elemRend->setWindowTitle(chElemName);
	elemRend->SetDataToVisualize( chImgData, chPolyData, chOTF, chCTF );
}

void dlg_XRF::showPieGlyphsSettings( bool isChecked )
{
	if(isChecked)
		gb_pieGlyphsSettings->show();
	else
		gb_pieGlyphsSettings->hide();
}

void dlg_XRF::updatePieGlyphParameters( int newVal )
{
	double opacity		= (double)sl_pieGlyphsOpacity->value() / sl_pieGlyphsOpacity->maximum();
	double spacing		= 1.0 - (double)sl_pieGlyphsSpacing->value() / sl_pieGlyphsSpacing->maximum();
	double magFactor	= (double)sl_pieGlyphResolution->value() / sl_pieGlyphResolution->maximum() * 0.7;

	MdiChild * mdiChild = (dynamic_cast<MdiChild*>(parent()));
	mdiChild->setPieGlyphParameters(opacity, spacing, magFactor);
}

void dlg_XRF::updateConcentrationOpacity(int newVal)
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

void dlg_XRF::ReferenceSpectrumClicked( const QModelIndex &index )
{
	int opacity;
	if (m_refSpectraLib->getElementOpacity(index, opacity))
	{
		sl_concentrationOpacity->setValue(opacity);
	}
}

QSharedPointer<iAElementConcentrations> dlg_XRF::GetElementConcentrations()
{
	return m_elementConcentrations;
}

void dlg_XRF::showRefSpectraChanged( int show )
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

void dlg_XRF::showRefLineChanged( int show )
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

void dlg_XRF::computeSimilarityMap()
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
	typedef itk::MeanSquaresImageToImageMetric < ImageType, ImageType >  MeanSquaresMetricType;
	typedef itk::MutualInformationImageToImageMetric < ImageType, ImageType >  MutualInformationMetricType;
	typedef itk::NormalizedCorrelationImageToImageMetric < ImageType, ImageType > NormalizedCorrelationMetricType;
	typedef MutualInformationMetricType MetricType;
	typedef itk::LinearInterpolateImageFunction < ImageType, double > InterpolatorType;
	typedef itk::IdentityTransform<double, Dimensions>  TransformType;
	typedef itk::ExtractImageFilter < ImageType3D, ImageType > ExtractImageType;

	iAConnector * connectors = new iAConnector[numEBins];
	ImageType3D ** images = new ImageType3D*[numEBins];
	for (int i=0; i<numEBins; ++i)
	{
		connectors[i].SetImage( ( *m_xrfData->GetDataPtr() )[i] ); 
		connectors[i].Modified();
		images[i] = dynamic_cast <ImageType3D*> ( connectors[i].GetITKImage() );
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

void dlg_XRF::energyBinsSelected( int binX, int binY )
{
	m_selectedBinXDrawer->setPosition( binX );
	m_selectedBinYDrawer->setPosition( binY );
	m_spectrumDiagram->update();
}

void dlg_XRF::updateDecompositionGUI( QStringList elementsNames )
{
	m_decomposeSelectedElements.clear();
	m_decomposeSelectedElements.resize( elementsNames.size() );

	QVector<iAElementSpectralInfo*> elementSpectra;

	int colorIdx = 0;
	iAColorTheme const * theme = iAColorThemeManager::GetInstance().GetTheme( "Brewer Set1 (max. 9)" );
	for ( size_t i = 0; i < m_refSpectraLib->spectra.size(); ++i )
	{
		int pos = elementsNames.indexOf( m_refSpectraLib->spectra[i].GetName() );
		if ( pos != -1 )
		{
			m_decomposeSelectedElements[pos] = i;
			elementSpectra.push_back( &m_refSpectraLib->spectra[i] );
			m_refSpectraLib->getItemModel()->item( i )->setCheckState( Qt::Checked );
			if ( colorIdx < theme->size() )
			{
				m_refSpectraLib->getItemModel()->item( i )->setData( theme->GetColor( colorIdx ), Qt::DecorationRole );
			}
			colorIdx++;
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

void dlg_XRF::AddElementLine(QString const & symbol)
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

void dlg_XRF::RemoveElementLine(QString const & symbol)
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

void dlg_XRF::AddReferenceSpectrum(int modelIdx)
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
	QSharedPointer<iAMappingDiagramData> data(new iAMappingDiagramData(
		&m_refSpectraLib->spectra[modelIdx].GetCountsData()[0],
		energies.size(), energies[0], energies[energies.size()-1],
		m_xrfData->size(), m_xrfData->GetMinEnergy(), m_xrfData->GetMaxEnergy(),
		m_accumulatedXRF->YBounds()[1]));
	QColor color = m_refSpectraLib->getElementColor(modelIdx);
	QSharedPointer<iAStepFunctionPlot> drawable(new iAStepFunctionPlot(data, color));
	m_refSpectraDrawers.insert(modelIdx, drawable);
	m_spectrumDiagram->addPlot(drawable);
	m_spectrumDiagram->update();
}

void dlg_XRF::RemoveReferenceSpectrum(int modelIdx)
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

bool dlg_XRF::ShowElementLines() const
{
	return m_refSpectra->cb_showRefLines->isChecked();
}

bool dlg_XRF::ShowReferenceSpectra() const
{
	return m_refSpectra->cb_showRefSpectra->isChecked();
}

int dlg_XRF::GetModelIdx(int elemIdx) const
{
	return GetModelIdx(PeriodicTable::elements[elemIdx].shortname.c_str());
}

int dlg_XRF::GetModelIdx(QString const & symbol) const
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

bool dlg_XRF::IsElementSelected(int elemIdx) const
{
	int modelIdx = GetModelIdx(elemIdx);
	if (modelIdx == -1)
		return false;
	return m_refSpectraLib->getItemModel()->item(modelIdx)->checkState() == Qt::Checked;
}
