// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iANModalObjects.h"
#include "iANModalSeedTracker.h"

#include <vtkSmartPointer.h>
#include <vtkSmartVolumeMapper.h>

#include <QColor>
#include <QList>
#include <QMap>
#include <QSharedPointer>
#include <QString>
#include <QVector>

#include <unordered_set>

class iALabelsDlg;

class iANModalTFManager;

class iAImageData;
class iAMdiChild;
class iAChartWithFunctionsWidget;
class iASlicer;

class vtkVolume;
class vtkRenderer;
class vtkImageData;

class iANModalSmartVolumeMapper : public vtkSmartVolumeMapper
{
public:
	static iANModalSmartVolumeMapper* New();
	vtkTypeMacro(iANModalSmartVolumeMapper, vtkSmartVolumeMapper);
	vtkGPUVolumeRayCastMapper* getGPUMapper()
	{
		return GPUMapper;
	}
	//vtkGPUVolumeRayCastMapper* getGPULowResMapper() { return GPULowResMapper; }
	//vtkFixedPointVolumeRayCastMapper  *RayCastMapper;
};

class iANModalController : public QObject
{
	Q_OBJECT

	friend class iANModalWidget;

public:
	iANModalController(iAMdiChild* mdiChild);
	void initialize();

	void setDataSets(const QList<std::shared_ptr<iAImageData>>& dataSets);
	void setMask(vtkSmartPointer<vtkImageData> mask);
	void resetTf(size_t dataSetIdx);

	void reinitialize();

	void updateLabel(const iANModalLabel&);
	void updateLabels(const QList<iANModalLabel>&);
	void addSeeds(const QList<iANModalSeed>&, const iANModalLabel&);
	void removeSeeds(const QList<iANModalSeed>&);
	void removeAllSeeds();

private slots:
	void initializeHistogram(size_t dataSetIdx);

private:

	void privateInitialize();
	iASlicer* initializeSlicer(std::shared_ptr<iAImageData>);
	void initializeCombinedVol();
	void initializeMainSlicers();
	bool checkDataSets(const QList<std::shared_ptr<iAImageData>>& dataSets);
	void updateHistograms();
	template <typename PixelType>
	void updateMainSlicers();

	iAMdiChild* m_mdiChild;
	QList<std::shared_ptr<iAImageData>> m_dataSets;
	QList<QSharedPointer<iANModalTFManager>> m_tfs;
	vtkSmartPointer<vtkImageData> m_mask;
	QMap<int, std::shared_ptr<iAImageData>> m_mapOverlayImageId2dataSet;
	QMap<size_t, int> m_dataSetIdx2HistIdx;
	QList<uint> m_channelIds;
	vtkSmartPointer<vtkVolume> m_combinedVol;
	vtkSmartPointer<iANModalSmartVolumeMapper> m_combinedVolMapper;
	vtkSmartPointer<vtkRenderer> m_combinedVolRenderer;
	vtkSmartPointer<vtkImageData> m_sliceImages2D[3];
	bool m_initialized = false;

	uint m_slicerChannel_main = 0;
	uint m_slicerChannel_label = 1;
	uint m_mainSlicerChannel_nModal;

	int m_maxLabelId = -1;

	void applyVolumeSettings();

	// Internal widgets
	QVector<iASlicer*> m_slicers;
	QVector<iAChartWithFunctionsWidget*> m_histograms;

	// Seed tracker (visualization next to the main slicers's sliders)
	iANModalSeedTracker m_tracker;

	iALabelsDlg* m_dlg_labels;

signals:
	void allSlicersInitialized();
	void allSlicersReinitialized();
	void histogramInitialized(int index);

private slots:
	void trackerBinClicked(iASlicerMode mode, int num);

public slots:
	void update();
};
