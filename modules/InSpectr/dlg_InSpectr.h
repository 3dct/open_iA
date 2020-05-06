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
#pragma once

#include "dlg_elementRenderer.h"

#include "iACharacteristicEnergy.h"
#include "iASpectrumFilter.h"
#include "iASpectrumFunction.h"
#include "ui_XRF.h"

#include <iARendererManager.h>
#include <iAVtkWidgetFwd.h>
#include <qthelper/iAQTtoUIConnector.h>

#include <vtkSmartPointer.h>

#include <QSharedPointer>
#include <QStandardItemModel>

#include <vector>

typedef iAQTtoUIConnector<QDockWidget, Ui_XRF>   dlg_xrfContainer;

class dlg_periodicTable;
class dlg_RefSpectra;
class iAAccumulatedXRFData;
class iADecompositionCalculator;
class iAElementConcentrations;
class iAEnergySpectrumDiagramData;
class iAEnergySpectrumWidget;
class iAPieChartGlyph;
class iAPieChartWidget;
class iAReferenceSpectraLibrary;
class iAPeriodicTableListener;
class iAXRFData;

class iAPlot;
class iAPlotCollection;
class iASelectedBinPlot;
class iAStepFunctionPlot;
class iAWidgetAddHelper;

class vtkColorTransferFunction;
class vtkImageData;
class vtkLookupTable;
class vtkPiecewiseFunction;
class vtkScalarBarActor;

class QDockWidget;

class dlg_InSpectr : public dlg_xrfContainer, public iASpectrumFilterListener
{
	Q_OBJECT
public:
	dlg_InSpectr(QWidget *parentWidget, dlg_periodicTable* dlgPeriodicTable, dlg_RefSpectra* dlgRefSpectra);
	void init(double minEnergy, double maxEnergy, bool haveEnergyLevels,
		iAWidgetAddHelper& widgetAddHelper);
	void InitElementMaps(iAWidgetAddHelper & widgetAddHelper);

	vtkSmartPointer<vtkImageData> GetCombinedVolume();
	vtkSmartPointer<vtkColorTransferFunction> GetColorTransferFunction();
	QThread* UpdateForVisualization();
	QSharedPointer<iAXRFData> GetXRFData();
	QSharedPointer<iAElementConcentrations> GetElementConcentrations();

	void UpdateVoxelSpectrum(int x, int y, int z);
	void UpdateConcentrationViews(int x, int y, int z);
	bool IsInitialized();

	void OnSelectionUpdate(QVector<iASpectrumFilter> const & filter);
	bool isDecompositionLoaded() {return m_decompositionLoaded;}

	void AddSimilarityMarkers();
	void RemoveSimilarityMarkers();

	void AddElementLine(QString const & symbol);
	void RemoveElementLine(QString const & symbol);
	void AddReferenceSpectrum(int modelIdx);
	void RemoveReferenceSpectrum(int modelIdx);
	bool ShowElementLines() const;
	bool ShowReferenceSpectra() const;
	int GetModelIdx(int elemIdx) const;
	int GetModelIdx(QString const & symbol) const;
	bool IsElementSelected(int elemIdx) const;
	void decompositionAvailable();

private slots:
	void updateAccumulate(int fctIdx);
	void updateSelectionMode(int modeIdx);
	void showSpectraLines(int show);
	void showSpectraHistograms(int show);
	void showSpectraHistogramsSettings( bool isChecked );
	void showPieGlyphsSettings( bool isChecked );
	void showVoxelSpectrum(int show);
	void showAggregatedSpectrum(int show);
	void decomposeElements();
	void loadDecomposition();
	void storeDecomposition();
	void combinedElementMaps(int show);
	void pieGlyphsVisualization(int show);
	void spectraHistSensitivityChanged(int newVal);
	void spectraOpacityThresholdChanged(int newVal);
	void smoothOpacityFadeChecked(int checked);
	void recomputeSpectraHistograms();
	void changeColormap(int colormapInd);
	void showLinkedElementMaps(int show);
	void updatePieGlyphParameters(int newVal = 0);
	void updateConcentrationOpacity(int newVal);
	void showRefSpectraChanged( int show );
	void showRefLineChanged( int show );
	void updateFunctionalBoxplot(int show);
	void setLogDrawMode(bool);
	void setLinDrawMode(bool);
	void decompositionSuccess();
	void decompositionFinished();
	void SpectrumTFChanged();

	void computeSimilarityMap();

public slots:
	void ReferenceSpectrumDoubleClicked( const QModelIndex &index );
	void ReferenceSpectrumClicked( const QModelIndex &index );
	void ReferenceSpectrumItemChanged( QStandardItem * item );
	void energyBinsSelected( int binX, int binY );

	//! @{ slicer pie glyphs
private slots:
	void updatePieGlyphs(int slicerMode);
private:
	void setSlicerPieGlyphsOn(bool isOn);
	void updatePieGlyphParamsInternal();
	void updateAllPieGlyphs();

	bool            m_pieGlyphsEnabled;         //!< if slice pie glyphs are enabled
	QVector<QSharedPointer<iAPieChartGlyph> > m_pieGlyphs[3];
	double          m_pieGlyphMagFactor;
	double          m_pieGlyphSpacing;
	double          m_pieGlyphOpacity;
	//! @}

	void updateDecompositionGUI( QStringList elementsNames );
	void initSpectraLinesDrawer();
	void initSpectraOverlay();
	void updateComposition(QVector<double> const & concentration);
	void updateSelection();
	void enableControlsNeedingDecompositionData();
	void InitElementRenderer(dlg_elementRenderer * elemRend, size_t index);
	void InitCommonGUI(iAWidgetAddHelper & widgetAddHelper);

	QSharedPointer<QImage>                         m_spectraHistogramImage;
	QImage                                         m_spectraHistogramColormap;
	QSharedPointer<QImage>                         m_functionalBoxplotImage;
	bool                                           m_initialized;
	bool                                           m_ctfChanged;
	bool                                           m_decompositionLoaded;
	iAEnergySpectrumWidget *                       m_spectrumDiagram;
	iAPieChartWidget *                             m_pieChart;
	QGridLayout *                                  m_accumulatedGridLayout;
	QGridLayout *                                  m_pieChartGridLayout;
	vtkSmartPointer<vtkPiecewiseFunction>          m_oTF;
	vtkSmartPointer<vtkColorTransferFunction>      m_cTF;
	QSharedPointer<iAXRFData>                      m_xrfData;
	QSharedPointer<iAEnergySpectrumDiagramData>    m_voxelEnergy;
	QSharedPointer<iAAccumulatedXRFData>           m_accumulatedXRF;
	QMap<int, QSharedPointer<iAStepFunctionPlot> > m_refSpectraDrawers;
	QSharedPointer<iAReferenceSpectraLibrary>      m_refSpectraLib;
	QSharedPointer<iAPlotCollection>               m_spectraLinesDrawer;
	QSharedPointer<iAPlot>                         m_voxelSpectrumDrawer;
	QSharedPointer<iAElementConcentrations>        m_elementConcentrations;
	QSharedPointer<iADecompositionCalculator>      m_decompositionCalculator;
	QSharedPointer<iASelectedBinPlot>              m_selectedBinXDrawer;
	QSharedPointer<iASelectedBinPlot>              m_selectedBinYDrawer;
	QVector<int>                                   m_decomposeSelectedElements;
	int                                            m_enabledChannels;
	std::vector<uint>                              m_channelIDs;
	std::vector<QColor>                            m_channelColors;
	uint                                           m_spectrumSelectionChannelID;
	vtkSmartPointer<vtkLookupTable>                m_ctf[3];
	vtkSmartPointer<vtkPiecewiseFunction>          m_otf[3];
	dlg_periodicTable *                            m_periodicTable;
	vtkSmartPointer<vtkColorTransferFunction>      m_selection_ctf;
	vtkSmartPointer<vtkPiecewiseFunction>          m_selection_otf;
	//! @{ Spectra Histogram colormap
	vtkSmartPointer<vtkRenderer>                   m_colormapRen;
	vtkSmartPointer<vtkScalarBarActor>             m_colormapScalarBarActor;
	vtkSmartPointer<vtkColorTransferFunction>      m_colormapLUT;
	//! @}
	QVector<dlg_elementRenderer*>                  m_elementRenderers; 	//!< Individual element renderers
	QVector<iASpectrumFilter>                      m_activeFilter;
	dlg_RefSpectra *                               m_refSpectra;
	iARendererManager                              m_rendererManager;
	QVector<iACharacteristicEnergy>                m_characteristicEnergies;
	QDockWidget *                                  m_pieChartContainer;
	QSharedPointer<iAPeriodicTableListener>        m_periodicTableListener;
	iAVtkOldWidget *                               m_colormapWidget;
};
