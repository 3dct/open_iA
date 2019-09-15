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

#include "iALabellingObjects.h"
#include "iAChannelData.h"
#include "ui_labels.h"

#include <qthelper/iAQTtoUIConnector.h>

#include <vtkSmartPointer.h>

#include <QList>
#include <QSharedPointer>

class iASlicer;
class iAColorTheme;
class iAModality;
class MdiChild;

struct OverlayImage;

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

	int addSlicer(iASlicer *slicer, QString name, int *extent, double *spacing, uint channelId);
	void addSlicer(iASlicer *slicer, int imageId, uint channelId=0);
	void removeSlicer(iASlicer *slicer);

	int labelCount();
	int overlayImageIdBySlicer(iASlicer*);

	void setSeedsTracking(bool enabled);

	// TEMPORARY
	QStandardItemModel* m_itemModel; // TODO: make private

signals:
	void seedsAdded(QList<iASeed>);
	void seedsRemoved(QList<iASeed>);
	void labelAdded(iALabel);
	void labelRemoved(iALabel);
	void labelsColorChanged(QList<iALabel>);

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
	void removeSeed(int, int, int, iASlicer*);
	void removeSeed(QStandardItem*);
	QStandardItem* addSeedItem(int label, int x, int y, int z, int imageId);
	int addLabelItem(QString const & labelText);
	void appendSeeds(int label, QList<QStandardItem*> const & items);
	void reInitChannelTF();
	void recolorItems();
	void updateChannels();
	void updateChannels(int imageId);
	void updateChannel(iASlicer*);

	bool m_trackingSeeds;

	int chooseOverlayImage(QString title);

	iAColorTheme const * m_colorTheme;
	QString m_fileName;
	QList<QSharedPointer<iALabel>> m_labels;
	int m_nextLabelId = 0;
	int getNextLabelId();

	int m_nextId = 0;
	int getNextId();
	struct OverlayImage
	{
		OverlayImage(int _id, QString _name, vtkSmartPointer<iAvtkImageData> _image) : id(_id), name(_name), image(_image)
		{}
		int id;
		QString name;
		vtkSmartPointer<iAvtkImageData> image; // label overlay image
		QList<iASlicer*> slicers;
	};
	QMap<int, QSharedPointer<OverlayImage>> m_mapId2image;

	struct SlicerData
	{
		SlicerData(iAChannelData _channelData, uint _channelId, QList<QMetaObject::Connection> c, int id) : channelData(_channelData), channelId(_channelId), connections(c), overlayImageId(id)
		{}
		iAChannelData channelData;
		int overlayImageId;
		uint channelId;
		QList<QMetaObject::Connection> connections;
	};
	QMap<iASlicer*, QSharedPointer<SlicerData>> m_mapSlicer2data;

	vtkSmartPointer<vtkLookupTable> m_labelColorTF;
	vtkSmartPointer<vtkPiecewiseFunction> m_labelOpacityTF;
	MdiChild* m_mdiChild;
};