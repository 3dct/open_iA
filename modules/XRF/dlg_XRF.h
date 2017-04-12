/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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

#include <QSharedPointer>
#include <QStandardItemModel>

#include "ui_XRF.h"

#include "dlg_elementRenderer.h"
#include "iAQTtoUIConnector.h"
typedef iAQTtoUIConnector<QDockWidget, Ui_XRF>   dlg_xrfContainer;

#include <vtkSmartPointer.h>

#include "iASpectrumFilter.h"
#include "iASpectrumFunction.h"
#include "iARendererManager.h"

#include <vector>

class QDockWidget;

class QVTKOpenGLWidget;
class vtkColorTransferFunction;
class vtkImageData;
class vtkLookupTable;
class vtkPiecewiseFunction;
class vtkScalarBarActor;

class iAEnergySpectrumWidget;
class iAMultipleFunctionDrawer;
class iAPlot;
class iAPlotData;
class iAStepFunctionDrawer;
class iAWidgetAddHelper;

class dlg_periodicTable;
class dlg_RefSpectra;
class iAAccumulatedXRFData;
struct iACharacteristicEnergy;
class iADecompositionCalculator;
class iAElementConcentrations;
class iAEnergySpectrumDiagramData;
class iAPieChartWidget;
class iAReferenceSpectraLibrary;
class iASelectedBinDrawer;
class iAPeriodicTableListener;
class iAXRFData;

class dlg_XRF : public dlg_xrfContainer, public iASpectrumFilterListener
{
	Q_OBJECT
public:
	dlg_XRF(QWidget *parentWidget, dlg_periodicTable* dlgPeriodicTable, dlg_RefSpectra* dlgRefSpectra);
	~dlg_XRF();
	void init(double minEnergy, double maxEnergy, bool haveEnergyLevels,
		iAWidgetAddHelper& widgetAddHelper);
	void InitElementMaps(iAWidgetAddHelper & widgetAddHelper);

	vtkSmartPointer<vtkImageData> GetCombinedVolume();
	vtkSmartPointer<vtkColorTransferFunction> GetColorTransferFunction();
	QObject* UpdateForVisualization();
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

private:
	void updateDecompositionGUI( QStringList elementsNames );
	void SetupConnections();
	void initSpectraLinesDrawer();
	void initSpectraOverlay();
	void updateComposition(QVector<double> const & concentration);
	void updateSelection();
	void enableControlsNeedingDecompositionData();
	void InitElementRenderer(dlg_elementRenderer * elemRend, size_t index);
	void InitCommonGUI(iAWidgetAddHelper & widgetAddHelper);

	QSharedPointer<QImage>									m_spectraHistogramImage;
	QImage													m_spectraHistogramColormap;
	QSharedPointer<QImage>									m_functionalBoxplotImage;

	bool													m_initialized;
	bool													m_ctfChanged;
	bool													m_decompositionLoaded;
	iAEnergySpectrumWidget									* m_spectrumDiagram;
	iAPieChartWidget										* m_pieChart;
	QGridLayout												* m_accumulatedGridLayout;
	QGridLayout												* m_pieChartGridLayout;
	vtkSmartPointer<vtkPiecewiseFunction>					m_oTF;
	vtkSmartPointer<vtkColorTransferFunction>				m_cTF;
	QSharedPointer<iAXRFData>								m_xrfData;
	QSharedPointer<iAEnergySpectrumDiagramData>				m_voxelEnergy;
	QSharedPointer<iAAccumulatedXRFData>					m_accumulatedXRF;

	QMap<int, QSharedPointer<iAStepFunctionDrawer> >		m_refSpectraDrawers;
	QSharedPointer<iAReferenceSpectraLibrary>				m_refSpectraLib;
	
	QSharedPointer<iAMultipleFunctionDrawer>				m_spectraLinesDrawer;
	QSharedPointer<iAPlot>									m_voxelSpectrumDrawer;

	QSharedPointer<iAElementConcentrations>					m_elementConcentrations;
	QSharedPointer<iADecompositionCalculator>				m_decompositionCalculator;

	QSharedPointer<iASelectedBinDrawer>						m_selectedBinXDrawer;
	QSharedPointer<iASelectedBinDrawer>						m_selectedBinYDrawer;


	QVector<int>											m_decomposeSelectedElements;
	int														m_enabledChannels;
	vtkSmartPointer<vtkLookupTable>							m_ctf[3];
	vtkSmartPointer<vtkPiecewiseFunction>					m_otf[3];
	dlg_periodicTable*										m_periodicTable;
	vtkSmartPointer<vtkColorTransferFunction>				m_selection_ctf;
	vtkSmartPointer<vtkPiecewiseFunction>					m_selection_otf;

	//Spectra Histogram colormap
	vtkSmartPointer<vtkRenderer>							m_colormapRen;
	vtkSmartPointer<vtkScalarBarActor>						m_colormapScalarBarActor;
	vtkSmartPointer<vtkColorTransferFunction>				m_colormapLUT;

	//Individual element renderers
	QVector<dlg_elementRenderer*>							m_elementRenderers;

	QVector<iASpectrumFilter>								m_activeFilter;
	dlg_RefSpectra*											m_refSpectra;

	iARendererManager										m_rendererManager;
	QVector<iACharacteristicEnergy>							m_characteristicEnergies;
	QDockWidget*											m_pieChartContainer;
	QSharedPointer<iAPeriodicTableListener>					m_periodicTableListener;
};
