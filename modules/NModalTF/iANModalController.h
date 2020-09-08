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
#pragma once

#include "iANModalObjects.h"
#include "iANModalSeedTracker.h"

#include <vtkSmartPointer.h>
#include <vtkSmartVolumeMapper.h>

#include <QColor>
#include <QList>
#include <QMap>
//#include <QSet>
#include <QSharedPointer>
#include <QString>

#include <unordered_set>

class dlg_labels;

class iANModalTFManager;

class iAModality;
class iASlicer;
class MdiChild;

class vtkVolume;
class vtkRenderer;
class vtkImageData;


class iANModalSmartVolumeMapper : public vtkSmartVolumeMapper {
public:
	static iANModalSmartVolumeMapper *New();
	vtkTypeMacro(iANModalSmartVolumeMapper, vtkSmartVolumeMapper);
	vtkGPUVolumeRayCastMapper* getGPUMapper() { return GPUMapper; }
	//vtkGPUVolumeRayCastMapper* getGPULowResMapper() { return GPULowResMapper; }
	//vtkFixedPointVolumeRayCastMapper  *RayCastMapper;
};


class iANModalController : public QObject {
	Q_OBJECT

	friend class iANModalWidget;

public:
	iANModalController(MdiChild *mdiChild);
	void initialize();

	int countModalities();
	void setModalities(const QList<QSharedPointer<iAModality>> &modalities);
	void setMask(vtkSmartPointer<vtkImageData> mask);
	void resetTf(QSharedPointer<iAModality> modality);
	void resetTf(const QList<QSharedPointer<iAModality>> &modalities);

	void reinitialize();

	void updateLabel(const iANModalLabel &);
	void updateLabels(const QList<iANModalLabel> &);
	void addSeeds(const QList<iANModalSeed> &, const iANModalLabel &);
	void removeSeeds(const QList<iANModalSeed> &);
	void removeSeeds(const iANModalLabel &);

	void update();

private:
	MdiChild *m_mdiChild;

	void _initialize();
	iASlicer* _initializeSlicer(QSharedPointer<iAModality> modality);
	void _initializeCombinedVol();
	void _initializeMainSlicers();
	bool _checkModalities(const QList<QSharedPointer<iAModality>> &modalities);
	bool _matchModalities(QSharedPointer<iAModality> m1, QSharedPointer<iAModality> m2);
	template <typename PixelType> void _updateMainSlicers();

	QList<QSharedPointer<iAModality>> m_modalities;
	QList<QSharedPointer<iANModalTFManager>> m_tfs;
	vtkSmartPointer<vtkImageData> m_mask = nullptr;
	QMap<int, QSharedPointer<iAModality>> m_mapOverlayImageId2modality;
	QList<uint> m_channelIds;
	vtkSmartPointer<vtkVolume> m_combinedVol;
	vtkSmartPointer<iANModalSmartVolumeMapper> m_combinedVolMapper;
	vtkSmartPointer<vtkRenderer> m_combinedVolRenderer;
	vtkSmartPointer<vtkImageData> m_sliceImages2D[3];
	bool m_initialized = false;

	uint m_slicerChannel_main = 0;
	uint m_slicerChannel_label = 1;
	uint m_mainSlicerChannel_nModal;

	//std::unordered_set<iANModalSeed, iANModalSeed::Hasher, iANModalSeed::Comparator> m_seeds;
	int m_maxLabelId = -1;

	void applyVolumeSettings();

	// Internal widgets
	QList<iASlicer*> m_slicers;

	// Seed tracker (visualization next to the main slicers's sliders)
	iANModalSeedTracker m_tracker;

	// MdiChild widgets
	dlg_labels *m_dlg_labels;

signals:
	void allSlicersInitialized();
	void allSlicersReinitialized();

private slots:
	void setSliceNumber(int);
	void setSlicerMode(int);

};