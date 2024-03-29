// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "dlg_elementRenderer.h"

#include "iACharacteristicEnergy.h"
#include "iASpectrumFilter.h"
#include "iASpectrumFunction.h"
#include "ui_InSpectr.h"

#include <iARendererViewSync.h>
#include <qthelper/iAQTtoUIConnector.h>

#include <vtkSmartPointer.h>

#include <QSharedPointer>
#include <QStandardItemModel>

#include <vector>

typedef iAQTtoUIConnector<QDockWidget, Ui_InSpectr>   dlg_xrfContainer;

class dlg_RefSpectra;
class iAAccumulatedXRFData;
class iADecompositionCalculator;
class iAElementConcentrations;
class iAEnergySpectrumWidget;
class iAPeriodicTableListener;
class iAPeriodicTableWidget;
class iAPieChartGlyph;
class iAPieChartWidget;
class iAReferenceSpectraLibrary;
class iAXRFData;

class iAMdiChild;

class iAHistogramData;
class iAPlot;
class iAPlotCollection;
class iASelectedBinPlot;
class iAStepFunctionPlot;

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
	dlg_InSpectr(QWidget *parentWidget, iAPeriodicTableWidget* periodicTable, dlg_RefSpectra* dlgRefSpectra);
	void init(double minEnergy, double maxEnergy, bool haveEnergyLevels, iAMdiChild* child);

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
	void InitCommonGUI();

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
	QSharedPointer<iAHistogramData>                m_voxelEnergy;
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
	iAPeriodicTableWidget*                         m_periodicTable;
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
	iARendererViewSync                             m_rendererManager;
	QVector<iACharacteristicEnergy>                m_characteristicEnergies;
	QDockWidget *                                  m_pieChartContainer;
	QSharedPointer<iAPeriodicTableListener>        m_periodicTableListener;
	iAQVTKWidget *                                 m_colormapWidget;
};
