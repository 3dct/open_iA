// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iALabelsDlg.h"

#include "iAImageCoordinate.h"
#include "iALabellingObjects.h"
#include "ui_labels.h"

// base
#include <iAColorTheme.h>
#include <iAFileUtils.h>
#include <iAImageData.h>
#include <iALog.h>
//#include <iAPerformanceHelper.h>
#include <iAToolsVTK.h>
#include <iAVtkDraw.h>
#include <iAVec3.h>

// charts
#include <iALUT.h>

// guibase
#include <iAChannelData.h>
#include <iAParameterDlg.h>
#include <iAMdiChild.h>

// slicer
#include <iASlicer.h>

#include <vtkImageData.h>
#include <vtkLookupTable.h>
#include <vtkPiecewiseFunction.h>

#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QSettings>
#include <QStandardItemModel>
#include <QStringView>
#include <QThread>
#include <QXmlStreamReader>

#include <random>

namespace
{
	const double DefaultOpacity = 0.5;
	typedef int LabelPixelType;
	const QString LabelsProjectKey = "Labels";
}

struct iAOverlayImage
{
	iAOverlayImage(int _id, QString _name, vtkSmartPointer<iAvtkImageData> _image) :
		id(_id), name(_name), image(_image)
	{
	}
	int id;
	QString name;
	vtkSmartPointer<iAvtkImageData> image;  // label overlay image
	QList<iASlicer*> slicers;
};

struct iAOverlaySlicerData
{
	iAOverlaySlicerData(iAChannelData _channelData, uint _channelId, QList<QMetaObject::Connection> c, int id) :
		channelData(_channelData), overlayImageId(id), channelId(_channelId), connections(c)
	{
	}
	iAChannelData channelData;
	int overlayImageId;
	uint channelId;
	QList<QMetaObject::Connection> connections;
};

iALabelsDlg::iALabelsDlg(iAMdiChild* mdiChild) :
	m_trackingSeeds(true),
	m_colorTheme(iAColorThemeManager::theme("Brewer Set3 (max. 12)")),
	m_mdiChild(mdiChild),
	m_ui(new Ui_labels()),
	m_itemModel(new QStandardItemModel())
{
	m_ui->setupUi(this);
	m_ui->cbColorTheme->addItems(iAColorThemeManager::availableThemes());
	m_ui->cbColorTheme->setCurrentText(m_colorTheme->name());
	connect(m_ui->pbAdd, &QPushButton::clicked, this, &iALabelsDlg::add);
	connect(m_ui->pbRemove, &QPushButton::clicked, this, &iALabelsDlg::remove);
	connect(m_ui->pbStore, &QPushButton::clicked, this, &iALabelsDlg::storeLabels);
	connect(m_ui->pbLoad, &QPushButton::clicked, this, &iALabelsDlg::loadLabels);
	connect(m_ui->pbStoreImage, &QPushButton::clicked, this, &iALabelsDlg::storeImage);
	connect(m_ui->pbSample, &QPushButton::clicked, this, &iALabelsDlg::sample);
	connect(m_ui->pbClear, &QPushButton::clicked, this, &iALabelsDlg::clear);
	connect(m_ui->cbColorTheme, &QComboBox::currentTextChanged, this, &iALabelsDlg::colorThemeChanged);
#if QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
	connect(m_ui->cbTrackSeeds, &QCheckBox::stateChanged, this, [this](int newState) { setSeedsTracking(newState == Qt::Checked); });
#else
	connect(m_ui->cbTrackSeeds, &QCheckBox::checkStateChanged, this, [this](int newState) { setSeedsTracking(newState == Qt::Checked); });
#endif
	m_ui->slOpacity->setValue(DefaultOpacity * m_ui->slOpacity->maximum());
	connect(m_ui->slOpacity, &QSlider::valueChanged, this, &iALabelsDlg::opacityChanged);
	m_itemModel->setHorizontalHeaderItem(0, new QStandardItem("Label"));
	m_itemModel->setHorizontalHeaderItem(1, new QStandardItem("Count"));
	m_ui->lvLabels->setModel(m_itemModel);

	uint channelId = m_mdiChild->createChannel();
	auto img = m_mdiChild->firstImageData();
	if (img)
	{
		int id = addSlicer(m_mdiChild->slicer(iASlicerMode::XY), "Main XY", img->GetExtent(), img->GetSpacing(), channelId);
		addSlicer(m_mdiChild->slicer(iASlicerMode::XZ), id, channelId);
		addSlicer(m_mdiChild->slicer(iASlicerMode::YZ), id, channelId);
	}
}

int iALabelsDlg::addSlicer(iASlicer* slicer, QString name, int* extent, double* spacing, uint channelId)
{
	if (m_mapSlicer2data.contains(slicer))
	{
		LOG(lvlError, QString("Tried to add same slice twice!"));
		return -1;
	}
	int imageId = m_nextId++;
	auto labelOverlayImg = vtkSmartPointer<iAvtkImageData>::New();
	labelOverlayImg->SetExtent(extent);
	labelOverlayImg->SetSpacing(spacing);
	labelOverlayImg->AllocateScalars(VTK_INT, 1);
	clearImage<LabelPixelType>(labelOverlayImg, 0);

	m_mapId2image.insert(imageId, std::make_shared<iAOverlayImage>(imageId, name, labelOverlayImg));

	addSlicer(slicer, imageId, channelId);

	return imageId;
}

void iALabelsDlg::addSlicer(iASlicer* slicer, int imageId, uint channelId)
{
	auto oi = m_mapId2image.value(imageId);
	if (!oi)
	{
		// TODO: throw error
		assert(false);
	}

	oi->slicers.append(slicer);

	auto slicerData = m_mapSlicer2data.value(slicer);
	if (!slicerData)
	{
		int id = oi->id;

		QList<QMetaObject::Connection> c;
		c.append(connect(slicer, &iASlicer::leftClicked, this,
			[this, slicer](double x, double y, double z) { addSeed(x, y, z, slicer); }));
		c.append(connect(slicer, &iASlicer::leftDragged, this,
			[this, slicer](double x, double y, double z) { addSeed(x, y, z, slicer); }));
		c.append(connect(slicer, &iASlicer::rightClicked, this,
			[this, slicer](double x, double y, double z)
		{
			removeSeed(x, y, z, slicer);
			updateChannels(m_mapSlicer2data.value(slicer)->overlayImageId);
		}));
		auto channelData = iAChannelData("name", oi->image, m_labelColorTF, m_labelOpacityTF);
		slicer->addChannel(channelId, channelData, true);
		m_mapSlicer2data.insert(slicer, std::make_shared<iAOverlaySlicerData>(channelData, channelId, c, id));
	}
	else
	{
		if (slicerData->overlayImageId != imageId)
		{
			//TODO: throw error
			assert(false);
		}
		// else: do nothing
	}
}

void iALabelsDlg::removeSlicer(iASlicer* slicer)
{
	auto slicerData = m_mapSlicer2data.value(slicer);
	if (!slicerData)
	{
		return;
	}

	slicer->removeChannel(slicerData->channelId);

	auto connections = slicerData->connections;
	for (auto c : connections)
	{
		disconnect(c);
	}

	m_mapSlicer2data.remove(slicer);

	// WARNING (26.08.2019)
	// Adding same slicer twice is not allowed (see assert in 'addSlicer(iASlicer*, QString, int*, double*)')
	// So we can assume here that no slicer has been added twice
	auto oi = m_mapId2image.value(slicerData->overlayImageId);
	oi->slicers.removeOne(slicer);
	if (oi->slicers.isEmpty())
	{
		/*auto removed = */ m_mapId2image.remove(slicerData->overlayImageId);
	}
}

namespace
{
	QStandardItem* createCoordinateItem(int x, int y, int z, int overlayImageId)
	{
		QStandardItem* item =
			new QStandardItem("(" + QString::number(x) + ", " + QString::number(y) + ", " + QString::number(z) + ")");
		item->setData(x, Qt::UserRole + 1);
		item->setData(y, Qt::UserRole + 2);
		item->setData(z, Qt::UserRole + 3);
		item->setData(overlayImageId, Qt::UserRole + 4);
		return item;
	}
	inline QList<iALabel> createLabelsList(QList<std::shared_ptr<iALabel>> labelPtrs)
	{
		auto labels = QList<iALabel>();
		for (auto labelPtr : labelPtrs)
		{
			labels.append(*labelPtr.get());
		}
		return labels;
	}
}

int findSeed(QStandardItem* labelItem, int x, int y, int z, int overlayImageId)
{
	for (int i = 0; i < labelItem->rowCount(); ++i)
	{
		QStandardItem* coordItem = labelItem->child(i);
		int ix = coordItem->data(Qt::UserRole + 1).toInt();
		int iy = coordItem->data(Qt::UserRole + 2).toInt();
		int iz = coordItem->data(Qt::UserRole + 3).toInt();
		int ioverlayImageId = coordItem->data(Qt::UserRole + 4).toInt();

		if (x == ix && y == iy && z == iz && overlayImageId == ioverlayImageId)
		{
			return i;
		}
	}
	return -1;
}

bool seedAlreadyExists(QStandardItem* labelItem, int x, int y, int z, int imgId)  // TODO
{
	return findSeed(labelItem, x, y, z, imgId) != -1;
}

void iALabelsDlg::addSeed(double cx, double cy, double cz, iASlicer* slicer)
{
	if (!m_ui->cbEnableEditing->isChecked())
	{
		return;
	}
	int labelRow = curLabelRow();
	if (labelRow == -1)
	{
		return;
	}
	int imageId = m_mapSlicer2data.value(slicer)->overlayImageId;
	auto oi = m_mapId2image.value(imageId);
	iAVec3d worldCoords(cx, cy, cz);
	auto center = mapWorldCoordsToIndex(oi->image, worldCoords.data());

	QList<iASeed> addedSeeds;

	int radius = m_ui->spinBox->value() - 1;  // -1 because the center voxel shouldn't count
	//iATimeGuard timer(QString("Drawing circle of radius %1").arg(radius).toStdString());

	auto extent = oi->image->GetExtent();
	int xAxis = slicer->globalAxis(iAAxisIndex::X), yAxis = slicer->globalAxis(iAAxisIndex::Y),
		minSlicerX = vtkMath::Max(center[xAxis] - radius, extent[xAxis * 2]),
		maxSlicerX = vtkMath::Min(center[xAxis] + radius, extent[xAxis * 2 + 1]),
		minSlicerY = vtkMath::Max(center[yAxis] - radius, extent[yAxis * 2]),
		maxSlicerY = vtkMath::Min(center[yAxis] + radius, extent[yAxis * 2 + 1]);

	QList<QStandardItem*> items;
	iAVec3i coord(center);
	// compute radius^2, then we don't have to take square root every loop iteration:
	double squareRadius = std::pow(radius, 2);
	for (coord[xAxis] = minSlicerX; coord[xAxis] <= maxSlicerX; ++coord[xAxis])
	{
		for (coord[yAxis] = minSlicerY; coord[yAxis] <= maxSlicerY; ++coord[yAxis])
		{
			int dx = coord[xAxis] - center[xAxis];
			int dy = coord[yAxis] - center[yAxis];
			double squareDis = dx * dx + dy * dy;
			if (squareDis <= squareRadius)
			{
				int x = coord[0];
				int y = coord[1];
				int z = coord[2];
				auto item = addSeedItem(labelRow, coord[0], coord[1], coord[2], imageId);
				if (item)
				{
					items.append(item);
				}
				if (m_trackingSeeds)
				{
					addedSeeds.append(iASeed(x, y, z, imageId, m_labels[labelRow]));
				}
			}
		}
	}
	if (items.size() == 0)
	{
		return;
	}
	if (m_trackingSeeds)
	{
		appendSeeds(labelRow, items);
	}
	updateChannels(imageId);

	if (m_trackingSeeds)
	{
		emit seedsAdded(addedSeeds);
	}
}

QStandardItem* iALabelsDlg::addSeedItem(int labelRow, int x, int y, int z, int imageId)
{
	// make sure we're not adding the same seed twice:
	for (int l = 0; l < m_itemModel->rowCount(); ++l)
	{
		auto item = m_itemModel->item(l);
		int childIdx = findSeed(item, x, y, z, imageId);
		if (childIdx != -1)
		{
			if (l == labelRow)
			{
				return nullptr;  // seed already exists with same label, nothing to do
			}
			else
			{
				// seed already exists with different label; need to remove other
				// TODO: do that removal somehow "in batch" too?
				removeSeed(item->child(childIdx));
			}
		}
	}
	auto image = m_mapId2image.value(imageId)->image;
	drawPixel<LabelPixelType>(image, x, y, z, labelRow + 1);
	return createCoordinateItem(x, y, z, imageId);
}

void iALabelsDlg::appendSeeds(int label, QList<QStandardItem*> const& items)
{
	m_itemModel->item(label)->appendRows(items);
	m_itemModel->item(label, 1)->setText(QString::number(m_itemModel->item(label)->rowCount()));
}

int iALabelsDlg::addLabelItem(QString const& labelText)
{
	QStandardItem* newItem = new QStandardItem(labelText);
	QStandardItem* newItemCount = new QStandardItem("0");
	QColor color = m_colorTheme->color(m_itemModel->rowCount());
	newItem->setData(color, Qt::DecorationRole);
	QList<QStandardItem*> newRow;
	newRow.append(newItem);
	newRow.append(newItemCount);
	m_itemModel->appendRow(newRow);
	auto id = m_nextLabelId++;
	auto label = std::shared_ptr<iALabel>(new iALabel(id, QString::number(id), color));
	m_labels.append(label);
	emit labelAdded(*label);
	return newItem->row();
}

void iALabelsDlg::add()
{
	m_ui->pbStore->setEnabled(true);
	addLabelItem(QString::number(m_itemModel->rowCount()));
	reInitChannelTF();
}

void iALabelsDlg::reInitChannelTF()
{
	m_labelColorTF = iALUT::BuildLabelColorTF(m_itemModel->rowCount(), m_colorTheme);
	m_labelOpacityTF = iALUT::BuildLabelOpacityTF(m_itemModel->rowCount());
}

void iALabelsDlg::recolorItems()
{
	for (int row = 0; row < m_itemModel->rowCount(); ++row)
	{
		QColor color = m_colorTheme->color(row);
		auto item = m_itemModel->item(row);
		item->setData(color, Qt::DecorationRole);
		m_labels[row]->color = color;
	}
	emit labelsColorChanged(createLabelsList(m_labels));
}

void iALabelsDlg::updateChannels()
{
	for (auto slicer : m_mapSlicer2data.keys())
	{
		updateChannel(slicer);
	}
}

void iALabelsDlg::updateChannels(int imageId)
{
	for (auto slicer : m_mapId2image.value(imageId)->slicers)
	{
		updateChannel(slicer);
	}
}

void iALabelsDlg::updateChannel(iASlicer* slicer)
{
	auto slicerData = m_mapSlicer2data.value(slicer);
	auto imageId = slicerData->overlayImageId;
	auto oi = m_mapId2image.value(imageId);
	// TODO: only update color transfer functions if required!
	auto img = oi->image;
	img->Modified();
	img->SetScalarRange(0, m_itemModel->rowCount());

	auto channelData = slicerData->channelData;
	channelData.setColorTF(m_labelColorTF);
	channelData.setOpacityTF(m_labelOpacityTF);

	slicer->updateChannel(slicerData->channelId, channelData);
	slicer->update();
}

void iALabelsDlg::remove()
{
	QModelIndexList indices = m_ui->lvLabels->selectionModel()->selectedIndexes();
	if (indices.size() == 0)
	{
		return;
	}
	QStandardItem* item = m_itemModel->itemFromIndex(indices[0]);
	if (!item)
	{
		return;
	}
	if (item->parent() == nullptr)  // a label was selected
	{
		if (item->rowCount() > 0)
		{
			auto reply = QMessageBox::question(this, "Remove Label",
				QString("Are you sure you want to remove the whole label and all of its %1 seeds?")
					.arg(item->rowCount()),
				QMessageBox::Yes | QMessageBox::No);
			if (reply != QMessageBox::Yes)
			{
				return;
			}
		}

		int curLabel = curLabelRow();
		if (curLabel == -1)
		{
			return;
		}

		auto label = m_labels[curLabel];

		QList<iASeed> removedSeeds;
		for (int s = item->rowCount() - 1; s >= 0; --s)
		{
			auto seed = item->child(s);
			int x = seed->data(Qt::UserRole + 1).toInt();
			int y = seed->data(Qt::UserRole + 2).toInt();
			int z = seed->data(Qt::UserRole + 3).toInt();
			int oiid = seed->data(Qt::UserRole + 4).toInt();
			if (m_trackingSeeds)
			{
				removedSeeds.append(iASeed(x, y, z, oiid, label));
			}
			for (auto oi : m_mapId2image.values())
			{
				drawPixel<LabelPixelType>(oi->image, x, y, z, 0);
			}
		}
		m_itemModel->removeRow(curLabel);
		m_labels.removeAt(curLabel);
		for (int l = curLabel; l < m_itemModel->rowCount(); ++l)
		{
			auto itemAfter = m_itemModel->item(l);
			itemAfter->setText(QString::number(l));
			for (int s = itemAfter->rowCount() - 1; s >= 0; --s)
			{
				auto seed = itemAfter->child(s);
				int x = seed->data(Qt::UserRole + 1).toInt();
				int y = seed->data(Qt::UserRole + 2).toInt();
				int z = seed->data(Qt::UserRole + 3).toInt();
				for (auto oi : m_mapId2image.values())
				{
					drawPixel<LabelPixelType>(oi->image, x, y, z, l + 1);
				}
			}
		}
		if (m_itemModel->rowCount() == 0)
		{
			m_ui->pbStore->setEnabled(false);
		}

		// TODO remove respective OverlayImage from m_mapId2image
		// ... with that, the image (vtkSmartPointer<iAvtkImageData>) should also be deleted

		// TODO remove respective SlicerData from m_mapSlicer2data
		// ... by going through all entries and checking if overlayImageId matches that of the removed image

		if (m_trackingSeeds)
		{
			emit seedsRemoved(removedSeeds);
		}

		emit labelRemoved(*label);

		recolorItems();
		updateChannels();
	}
	else
	{  // remove a single seed
		removeSeed(item);
		updateChannels();
	}
}

void iALabelsDlg::removeSeed(double x, double y, double z, iASlicer* slicer)
{
	int id = m_mapSlicer2data.value(slicer)->overlayImageId;
	auto image = m_mapId2image.value(id)->image;
	iAVec3d worldCoords(x, y, z);
	auto vxlIdx = mapWorldCoordsToIndex(image, worldCoords.data());
	int label = static_cast<int>(image->GetScalarComponentAsFloat(vxlIdx[0], vxlIdx[1], vxlIdx[2], 0)) - 1;
	if (label == -1) // label not set yet
	{
		return;
	}
	int seedItemRow = findSeed(m_itemModel->item(label), x, y, z, id);
	removeSeed(vxlIdx[0], vxlIdx[1], vxlIdx[2], id, label, seedItemRow);
}

void iALabelsDlg::removeSeed(QStandardItem* item)
{
	int x = item->data(Qt::UserRole + 1).toInt();
	int y = item->data(Qt::UserRole + 2).toInt();
	int z = item->data(Qt::UserRole + 3).toInt();
	int id = item->data(Qt::UserRole + 4).toInt();
	int label = item->parent()->row();
	int seedItemRow = item->row();
	removeSeed(x, y, z, id, label, seedItemRow);
}

void iALabelsDlg::removeSeed(int x, int y, int z, int id, int label, int seedItemRow)
{
	if (label < 0)
	{
		return;
	}
	auto image = m_mapId2image.value(id)->image;
	drawPixel<LabelPixelType>(image, x, y, z, 0);
	if (m_trackingSeeds)
	{
		m_itemModel->item(label)->removeRow(seedItemRow);
		m_itemModel->item(label, 1)->setText(QString::number(m_itemModel->item(label, 1)->text().toInt() - 1));
		emit seedsRemoved(QList<iASeed>{iASeed(x, y, z, id, m_labels[label])});
	}
}

int iALabelsDlg::curLabelRow() const
{
	QModelIndexList indices = m_ui->lvLabels->selectionModel()->selectedIndexes();
	if (indices.size() <= 0)
	{
		return -1;
	}
	QStandardItem* item = m_itemModel->itemFromIndex(indices[0]);
	while (item->parent() != nullptr)
	{
		item = item->parent();
	}
	return item->row();
}

int iALabelsDlg::seedCount(int labelIdx) const
{
	QStandardItem* labelItem = m_itemModel->item(labelIdx);
	return labelItem->rowCount();
}

int iALabelsDlg::labelCount()
{
	return m_itemModel->rowCount();
}

int iALabelsDlg::overlayImageIdBySlicer(iASlicer* slicer)
{
	return m_mapSlicer2data.value(slicer)->overlayImageId;
}

void iALabelsDlg::setSeedsTracking(bool enabled)
{
	m_trackingSeeds = enabled;
	if (!enabled)
	{
		for (int l = 0; l < m_itemModel->rowCount(); ++l)
		{
			m_itemModel->item(l)->removeRows(0, m_itemModel->item(l)->rowCount());
		}
	}
}

void iALabelsDlg::saveState(QSettings& projectFile)
{
	QString outXML;
	QXmlStreamWriter writer(&outXML);
	storeXML(writer, false);
	projectFile.setValue(LabelsProjectKey, outXML.replace("\n", ""));
}

void iALabelsDlg::loadState(QSettings const& projectFile)
{
	int overlayImageId = chooseOverlayImage("Choose an image to load onto");
	if (overlayImageId == -1)
	{
		return;
	}
	QString labelsString = projectFile.value(LabelsProjectKey).toString();
	QXmlStreamReader reader(labelsString);
	loadXML(reader, overlayImageId);
}

int iALabelsDlg::chooseOverlayImage(QString title)
{
	auto size = m_mapId2image.size();
	if (size == 0)
	{
		return -1;
	}
	else if (size == 1)
	{
		return m_mapId2image.first()->id;
	}

	QStringList items;
	QMap<QString, int> map;
	for (std::shared_ptr<iAOverlayImage> oi : m_mapId2image.values())
	{
		QString name = oi->name;
		items.append(name);
		map.insert(name, oi->id);
	}

	bool ok;
	QString selectedString = QInputDialog::getItem(this, title, "Images", items, 0, false, &ok);
	if (!ok || selectedString.isEmpty())
	{
		return -1;
	}

	return map.value(selectedString);
}

bool iALabelsDlg::load(QString const& filename)
{
	int overlayImageId = chooseOverlayImage("Choose an image to load onto");
	if (overlayImageId == -1)
	{
		return false;
	}
	QFile file(filename);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		LOG(lvlError, QString("Seed file loading: Failed to open file '%1'!").arg(filename));
		return false;
	}

	QXmlStreamReader stream(&file);
	bool result = loadXML(stream, overlayImageId);
	file.close();
	if (file.error() != QFile::NoError)
	{
		LOG(lvlError, QString("Error: Cannot read file '%1': %2").arg(filename).arg(file.errorString()));
		return false;
	}
	if (result)
	{
		QFileInfo fileInfo(file);
		m_fileName = MakeAbsolute(fileInfo.absolutePath(), filename);
	}
	return result;
}

bool iALabelsDlg::loadXML(QXmlStreamReader& stream, int overlayImageId)
{
	std::shared_ptr<iAOverlayImage> oi = m_mapId2image.value(overlayImageId);
	clearImage<LabelPixelType>(oi->image, 0);
	m_itemModel->removeRows(0, m_itemModel->rowCount());

	stream.readNext();
	int curLabelRow = -1;
	bool enableStoreBtn = false;
	QList<QStandardItem*> items;
	auto finalizePrevLabel = [this, &items, &curLabelRow]()
	{
		if (items.size() == 0)
		{
			return;
		}
		if (curLabelRow == -1)
		{
			LOG(lvlWarn, QString("Error loading label XML: Current label not set, cannot add to list!"));
			return;
		}
		appendSeeds(curLabelRow, items);
	};
	while (!stream.atEnd())
	{
		if (stream.isStartElement())
		{
			if (stream.name().compare(QString("Label")) == 0)
			{
				finalizePrevLabel();
				enableStoreBtn = true;
				QString id = stream.attributes().value("id").toString();
				QString name = stream.attributes().value("name").toString();
				curLabelRow = addLabelItem(name);
				items.clear();
				if (m_itemModel->rowCount() - 1 != id.toInt())
				{
					LOG(lvlWarn,
						QString("Inserting row: rowCount %1 <-> label id %2 mismatch!")
							.arg(m_itemModel->rowCount())
							.arg(id));
				}
			}
			else if (stream.name().compare(QString("Seed")) == 0)
			{
				if (curLabelRow == -1)
				{
					LOG(lvlError, "Error loading label XML: Current label not set!");
					return false;
				}
				int x = stream.attributes().value("x").toInt();
				int y = stream.attributes().value("y").toInt();
				int z = stream.attributes().value("z").toInt();
				auto item = addSeedItem(curLabelRow, x, y, z, oi->id);
				if (item)
				{
					items.append(item);
				}
				else
				{   // TODO: check if triggered items are bulk-added only later (see appendSeeds above)
					LOG(lvlWarn,
						QString("Duplicate entry for %1, %2, %3, label %4!").arg(x).arg(y).arg(z).arg(curLabelRow));
				}
			}
		}
		stream.readNext();
	}
	finalizePrevLabel();
	m_ui->pbStore->setEnabled(enableStoreBtn);
	reInitChannelTF();
	updateChannels();
	if (stream.hasError())
	{
		LOG(lvlError, QString("Error: Failed to parse label XML: %1").arg(stream.errorString()));
		return false;
	}
	return true;
}

bool iALabelsDlg::store(QString const& filename, bool extendedFormat)
{
	int id = chooseOverlayImage("Choose a label overlay image to store");
	if (id == -1)
	{
		return false;
	}
	auto oi = m_mapId2image.value(id);

	QFile file(filename);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		QMessageBox::warning(this, "Labels", "Seed file storing: Failed to open file '" + filename + "'!");
		return false;
	}
	QFileInfo fileInfo(file);
	m_fileName = MakeAbsolute(fileInfo.absolutePath(), filename);
	QXmlStreamWriter stream(&file);
	stream.setAutoFormatting(true);
	storeXML(stream, extendedFormat);
	file.close();
	return true;
}

void iALabelsDlg::storeXML(QXmlStreamWriter& stream, bool extendedFormat)
{
	stream.writeStartDocument();
	stream.writeStartElement("Labels");

	for (int l = 0; l < m_itemModel->rowCount(); ++l)
	{
		QStandardItem* labelItem = m_itemModel->item(l);
		stream.writeStartElement("Label");
		stream.writeAttribute("id", QString::number(l));
		stream.writeAttribute("name", labelItem->text());
		for (int i = 0; i < labelItem->rowCount(); ++i)
		{
			stream.writeStartElement("Seed");
			QStandardItem* coordItem = labelItem->child(i);
			int x = coordItem->data(Qt::UserRole + 1).toInt();
			int y = coordItem->data(Qt::UserRole + 2).toInt();
			int z = coordItem->data(Qt::UserRole + 3).toInt();
			stream.writeAttribute("x", QString::number(x));
			stream.writeAttribute("y", QString::number(y));
			stream.writeAttribute("z", QString::number(z));
			if (extendedFormat)
			{
				for (auto d: m_mdiChild->dataSetMap())
				{
					auto imgDataSet = dynamic_cast<iAImageData*>(d.second.get());
					double value = imgDataSet->vtkImage()->GetScalarComponentAsDouble(x, y, z, 0);
					stream.writeStartElement("Value");
					stream.writeAttribute("dataSet", QString::number(d.first)); // TODO NEWIO: permanent solution - this code should also be used in storing the tool state, and we should use the ID the dataset gets in the project file here...
					stream.writeAttribute("value", QString::number(value, 'g', 16));
					stream.writeEndElement();
				}
			}
			stream.writeEndElement();
		}
		stream.writeEndElement();
	}
	stream.writeEndElement();
	stream.writeEndDocument();
}

void iALabelsDlg::loadLabels()
{
	QString fileName = QFileDialog::getOpenFileName(QApplication::activeWindow(), tr("Open Seed File"),
		QString(),  // TODO get directory of current file
		tr("Seed file (*.seed);;All files (*)"));
	if (fileName.isEmpty())
	{
		return;
	}
	if (!load(fileName))
	{
		QMessageBox::warning(this, "Labels", "Loading seed file '" + fileName + "' failed!");
	}
}

void iALabelsDlg::storeLabels()
{
	QString fileName = QFileDialog::getSaveFileName(QApplication::activeWindow(), tr("Save Seed File"),
		QString(),  // TODO get directory of current file
		tr("Seed file (*.seed);;All files (*)"));
	if (fileName.isEmpty())
	{
		return;
	}
	QString paramName("Extended Format (also write pixel values, not only positions)");
	iAAttributes param;
	addAttr(param, paramName, iAValueType::Boolean, true);
	iAParameterDlg dlg(this, "Seed File Format", param);
	if (dlg.exec() != QDialog::Accepted)
	{
		LOG(lvlError, "Selection of format aborted, aborting seed file storing");
		return;
	}
	if (!store(fileName, dlg.parameterValues()[paramName].toBool()))
	{
		QMessageBox::warning(this, "Labels", "Storing seed file '" + fileName + "' failed!");
	}
}

void iALabelsDlg::storeImage()
{
	int id = chooseOverlayImage("Choose image to store");
	if (id == -1)
	{
		return;
	}

	auto labelOverlayImage = m_mapId2image.value(id)->image;

	QString fileName = QFileDialog::getSaveFileName(QApplication::activeWindow(), tr("Save Seeds as Image"),
		QString(),  // TODO get directory of current file
		tr("Seed file (*.mhd);;All files (*)"));
	if (fileName.isEmpty())
	{
		return;
	}
	::storeImage(labelOverlayImage, fileName, false);
}

bool haveAllSeeds(QVector<int> const& label2SeedCounts, std::vector<int> const& requiredNumOfSeedsPerLabel)
{
	for (qsizetype i = 0; i < label2SeedCounts.size(); ++i)
	{
		if (label2SeedCounts[i] < requiredNumOfSeedsPerLabel[i])
		{
			return false;
		}
	}
	return true;
}

void iALabelsDlg::sample()
{
	int imageId = chooseOverlayImage("Choose an image to sample");
	if (imageId == -1)
	{
		return;
	}
	auto oi = m_mapId2image.value(imageId);

	auto gt = m_mdiChild->chooseDataSet("Choose Ground Truth");
	auto imgData = dynamic_cast<iAImageData*>(gt.get());
	if (!imgData)
	{
		LOG(lvlWarn, "The chosen dataset is not valid as ground truth because it doesn't contain image data!");
		return;
	}
	auto img = imgData->vtkImage();
	int labelCount = img->GetScalarRange()[1] + 1;
	if (labelCount > 50)
	{
		auto reply = QMessageBox::question(this, "Sample Seeds",
			QString("According to min/max values in image, there are %1 labels, this seems a bit much. Do you really "
					"want to proceed?")
				.arg(labelCount),
			QMessageBox::Yes | QMessageBox::No);
		if (reply != QMessageBox::Yes)
		{
			return;
		}
	}

	iAAttributes params;
	QString const NumSeeds("Number of Seeds per Label");
	QString const ReduceNum("Reduce seed number for all labels to size of smallest labeling");
	addAttr(params, NumSeeds, iAValueType::Discrete, 50, 1);
	addAttr(params, ReduceNum, iAValueType::Boolean, true);
	iAParameterDlg dlg(this, "Sample Seeds", params);
	if (dlg.exec() != QDialog::Accepted)
	{
		return;
	}
	auto values = dlg.parameterValues();
	int numOfSeeds = values[NumSeeds].toInt();
	bool reduceNum = values[ReduceNum].toBool();

	std::vector<int> numOfSeedsPerLabel(labelCount, numOfSeeds);
	// check that there is at least numOfSeedsPerLabel pixels per label
	std::vector<int> histogram(labelCount, 0);
	FOR_VTKIMG_PIXELS(img, x, y, z)
	{
		int label = static_cast<int>(img->GetScalarComponentAsFloat(x, y, z, 0));
		histogram[label]++;
	}
	int minNumOfSeeds = numOfSeeds;
	for (int i = 0; i < labelCount; ++i)
	{
		if ((histogram[i] * 3 / 4) < numOfSeedsPerLabel[i])
		{
			numOfSeedsPerLabel[i] = histogram[i] * 3 / 4;
			LOG(lvlWarn,
				QString("Reducing number of seeds for label %1 to %2 because it only has %3 pixels")
					.arg(i)
					.arg(numOfSeedsPerLabel[i])
					.arg(histogram[i]));
		}
		if (numOfSeedsPerLabel[i] < minNumOfSeeds)
		{
			minNumOfSeeds = numOfSeedsPerLabel[i];
		}
		if (m_itemModel->rowCount() <= i)
		{
			addLabelItem(QString::number(i));
		}
	}
	if (reduceNum)
	{
		for (int i = 0; i < labelCount; ++i)
		{
			numOfSeedsPerLabel[i] = minNumOfSeeds;
		}
	}

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> xDist(0, img->GetDimensions()[0] - 1);
	std::uniform_int_distribution<> yDist(0, img->GetDimensions()[1] - 1);
	std::uniform_int_distribution<> zDist(0, img->GetDimensions()[2] - 1);

	QVector<int> label2SeedCount(labelCount, 0);
	QVector<QList<QStandardItem*>> items(labelCount);
	while (!haveAllSeeds(label2SeedCount, numOfSeedsPerLabel))
	{
		int x = xDist(gen);
		int y = yDist(gen);
		int z = zDist(gen);
		int label = static_cast<int>(img->GetScalarComponentAsFloat(x, y, z, 0));

		if (label2SeedCount[label] < numOfSeedsPerLabel[label] &&
			!seedAlreadyExists(m_itemModel->item(label), x, y, z, imageId))
		{
			// m_itemModel->item(label)->appendRow(GetCoordinateItem(x, y, z));
			auto item = addSeedItem(label, x, y, z, oi->id);
			if (item)
			{
				label2SeedCount[label]++;
				items[label].append(item);
			}
		}
	}
	for (int l = 0; l < labelCount; ++l)
	{
		appendSeeds(l, items[l]);
	}
	reInitChannelTF();
	updateChannels();
	m_ui->pbStore->setEnabled(true);
}

void iALabelsDlg::clear()
{
	for (auto oi : m_mapId2image.values())
	{
		auto img = oi->image;
		if (img)
		{
			clearImage<LabelPixelType>(img, 0);
		}
	}
	updateChannels();
	m_itemModel->clear();
	emit allSeedsRemoved();
	for (auto label : m_labels)
	{
		emit labelRemoved(*label);
	}
	m_labels.clear();
}

QString const& iALabelsDlg::fileName()
{
	return m_fileName;
}

void iALabelsDlg::colorThemeChanged(QString const& newThemeName)
{
	m_colorTheme = iAColorThemeManager::theme(newThemeName);
	reInitChannelTF();
	recolorItems();
	updateChannels();
}

void iALabelsDlg::opacityChanged(int newValue)
{
	double opacity = static_cast<double>(newValue) / m_ui->slOpacity->maximum();
	for (auto slicer : m_mapSlicer2data.keys())
	{
		auto slicerData = m_mapSlicer2data.value(slicer);
		auto& channelData = slicerData->channelData;
		channelData.setOpacity(opacity);
		slicer->setChannelOpacity(slicerData->channelId, opacity);
		slicer->update();
	}
}
