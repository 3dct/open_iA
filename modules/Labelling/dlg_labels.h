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

#include "Labelling_export.h"

#include "ui_labels.h"

#include <qthelper/iAQTtoUIConnector.h>

#include <vtkSmartPointer.h>

#include <QList>
#include <QSharedPointer>

class iASlicer;
class iAColorTheme;
class iAModality;
class MdiChild;

class QStandardItem;
class QStandardItemModel;

class iAvtkImageData;
class vtkLookupTable;
class vtkObject;
class vtkPiecewiseFunction;

struct iAImageCoordinate;

typedef iAQTtoUIConnector<QDockWidget, Ui_labels> dlg_labelUI;

class Labelling_API dlg_labels : public dlg_labelUI
{
	Q_OBJECT

public:
	dlg_labels(MdiChild* mdiChild, bool addMainSlicer = true);
	int curLabelRow() const;
	int seedCount(int labelIdx) const;
	bool load(QString const & filename);
	bool store(QString const & filename, bool extendedFormat);

	void addSlicer(iASlicer *slicer, QSharedPointer<iAModality> modality);
	void removeSlicer(iASlicer *slicer);

	//void disconnectMainSlicers();

	// TEMPORARY
	QStandardItemModel* m_itemModel; // TODO: make private

public slots:
	void rendererClicked(int, int, int, iASlicer*);
	void slicerClicked(int, int, int, iASlicer*);
	void slicerDragged(int, int, int, iASlicer*);
	void slicerRightClicked(int, int, int, iASlicer*);
	void add();
	void remove();
	void storeLabels();
	void loadLabels();
	void storeImage();
	void sample();
	void clear();
	void colorThemeChanged(QString const & newThemeName);
	QString const & fileName();
	void opacityChanged(int newValue);

private:
	void addSeed(int, int, int, iASlicer*);
	void removeSeed(QStandardItem* item, int x, int y, int z, iASlicer* slicer);
	QStandardItem* addSeedItem(int label, int x, int y, int z, iASlicer *slicer);
	int addLabelItem(QString const & labelText);
	void appendSeeds(int label, QList<QStandardItem*> const & items);
	void reInitChannelTF();
	void recolorItems();
	void updateChannel(iASlicer* slicer);

	iAColorTheme const * m_colorTheme;
	QString m_fileName;

	struct ModalityOverlayImage {
		ModalityOverlayImage(QSharedPointer<iAModality> m, vtkSmartPointer<iAvtkImageData> o, iAChannelData cd) : modality(m), overlayImage(o), channelData(cd) {}
		QSharedPointer<iAModality> modality;
		vtkSmartPointer<iAvtkImageData> overlayImage;
		iAChannelData channelData;
		bool operator ==(ModalityOverlayImage moi) {
			return moi.modality == modality;
		}
	};
	QList<iASlicer*> m_slicers;
	// for label overlay:
	QMap<iASlicer*, ModalityOverlayImage> m_slicersData;
	//QList<uint> m_labelChannelIds;
	vtkSmartPointer<vtkLookupTable> m_labelColorTF;
	vtkSmartPointer<vtkPiecewiseFunction> m_labelOpacityTF;
	MdiChild* m_mdiChild;

	//QList<QSharedPointer<iAModality>> m_modalities;
	//uint m_nextChannelId = 0;
	//uint nextChannelId();

	struct SlicerConnections {
		QMetaObject::Connection c[3];
	};
	QList<SlicerConnections> m_connections;
};