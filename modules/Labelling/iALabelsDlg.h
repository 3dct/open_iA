// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "labelling_export.h"
#include "iAChannelData.h"
#include "iALabellingObjects.h"

#include <vtkSmartPointer.h>

#include <QDockWidget>
#include <QList>
#include <QMap>

#include <memory>

class iAColorTheme;
class iAImageCoordinate;
class iASlicer;
class iAvtkImageData;
class iAMdiChild;
class Ui_labels;

class QStandardItem;
class QStandardItemModel;

class vtkLookupTable;
class vtkObject;
class vtkPiecewiseFunction;

struct iAOverlayImage;
struct iAOverlaySlicerData;

class Labelling_API iALabelsDlg : public QDockWidget
{
	Q_OBJECT

public:
	iALabelsDlg(iAMdiChild* mdiChild, bool addMainSlicer = true);
	int curLabelRow() const;
	int seedCount(int labelIdx) const;
	bool load(QString const& filename);
	bool store(QString const& filename, bool extendedFormat);

	int addSlicer(iASlicer* slicer, QString name, int* extent, double* spacing, uint channelId);
	void addSlicer(iASlicer* slicer, int imageId, uint channelId = 0);
	void removeSlicer(iASlicer* slicer);

	int labelCount();
	int overlayImageIdBySlicer(iASlicer*);
	//! Sets whether added seed points should be tracked as children of the label ist
	void setSeedsTracking(bool enabled);

signals:
	void seedsAdded(const QList<iASeed>&);
	void seedsRemoved(const QList<iASeed>&);
	void allSeedsRemoved();
	void labelAdded(const iALabel&);
	void labelRemoved(const iALabel&);
	void labelsColorChanged(const QList<iALabel>&);

public slots:
	void add();
	void remove();
	void storeLabels();
	void loadLabels();
	void storeImage();
	void sample();
	void clear();
	void colorThemeChanged(QString const& newThemeName);
	QString const& fileName();
	void opacityChanged(int newValue);

private:
	void addSeed(double, double, double, iASlicer*);
	void removeSeed(double, double, double, iASlicer*);
	void removeSeed(QStandardItem*);
	void removeSeed(int x, int y, int z, int id, int label, int seedItemRow);
	QStandardItem* addSeedItem(int label, int x, int y, int z, int imageId);
	int addLabelItem(QString const& labelText);
	void appendSeeds(int label, QList<QStandardItem*> const& items);
	void reInitChannelTF();
	void recolorItems();
	void updateChannels();
	void updateChannels(int imageId);
	void updateChannel(iASlicer*);
	int chooseOverlayImage(QString title);

	//! whether seeds are also tracked as entries in the label list
	bool m_trackingSeeds;

	iAColorTheme const* m_colorTheme;
	QString m_fileName;
	QList<std::shared_ptr<iALabel>> m_labels;
	int m_nextLabelId = 0;
	int m_nextId = 0;
	QMap<int, std::shared_ptr<iAOverlayImage>> m_mapId2image;
	QMap<iASlicer*, std::shared_ptr<iAOverlaySlicerData>> m_mapSlicer2data;

	vtkSmartPointer<vtkLookupTable> m_labelColorTF;
	vtkSmartPointer<vtkPiecewiseFunction> m_labelOpacityTF;
	iAMdiChild* m_mdiChild;
	std::shared_ptr<Ui_labels> m_ui;
	QStandardItemModel* m_itemModel;
};
