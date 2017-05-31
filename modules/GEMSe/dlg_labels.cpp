/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
#include "pch.h"
#include "dlg_labels.h"

#include "dlg_commoninput.h"
#include "iAColorTheme.h"
#include "iAConsole.h"
#include "iAImageCoordinate.h"
#include "iAChannelID.h"
#include "iAChannelVisualizationData.h"
#include "iAFileUtils.h"
#include "iALabelOverlayThread.h"
#include "iAModality.h"
#include "iAModalityList.h"
#include "iAToolsVTK.h"
#include "iAVtkDraw.h"
#include "mdichild.h"

#include <vtkImageData.h>
#include <vtkLookupTable.h>
#include <vtkMetaImageWriter.h>
#include <vtkPiecewiseFunction.h>

#include <QFileDialog>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QStandardItemModel>
#include <QThread>
#include <QXmlStreamReader>

#include <random>


dlg_labels::dlg_labels(MdiChild* mdiChild, iAColorTheme const * colorTheme):
	m_itemModel(new QStandardItemModel()),
	m_colorTheme(colorTheme),
	m_mdiChild(mdiChild),
	m_maxColor(0)
{
	connect(pbAdd, SIGNAL(clicked()), this, SLOT(Add()));
	connect(pbRemove, SIGNAL(clicked()), this, SLOT(Remove()));
	connect(pbStore, SIGNAL(clicked()), this, SLOT(Store()));
	connect(pbLoad, SIGNAL(clicked()), this, SLOT(Load()));
	connect(pbStoreImage, SIGNAL(clicked()), this, SLOT(StoreImage()));
	connect(pbSample, SIGNAL(clicked()), this, SLOT(Sample()));
	connect(pbClear, SIGNAL(clicked()), this, SLOT(Clear()));
	m_itemModel->setHorizontalHeaderItem(0, new QStandardItem("Label"));
	m_itemModel->setHorizontalHeaderItem(1, new QStandardItem("Count"));
	lvLabels->setModel(m_itemModel);
}


namespace
{
	QStandardItem* GetCoordinateItem(int x, int y, int z)
	{
		QStandardItem* item = new QStandardItem("(" + QString::number(x) + ", " + QString::number(y) + ", " + QString::number(z) + ")");
		item->setData(x, Qt::UserRole + 1);
		item->setData(y, Qt::UserRole + 2);
		item->setData(z, Qt::UserRole + 3);
		return item;
	}
}


void dlg_labels::RendererClicked(int x, int y, int z)
{
	AddSeed(x, y, z);
}


int FindSeed(QStandardItem* labelItem, int x, int y, int z)
{
	for (int i = 0; i<labelItem->rowCount(); ++i)
	{
		QStandardItem* coordItem = labelItem->child(i);
		int ix = coordItem->data(Qt::UserRole + 1).toInt();
		int iy = coordItem->data(Qt::UserRole + 2).toInt();
		int iz = coordItem->data(Qt::UserRole + 3).toInt();
		if (x == ix && y == iy && z == iz)
			return i;
	}
	return -1;
}


bool SeedAlreadyExists(QStandardItem* labelItem, int x, int y, int z)
{
	return FindSeed(labelItem, x, y, z) != -1;
}


void dlg_labels::AddSeed(int x, int y, int z)
{
	if (!cbEnableEditing->isChecked())
	{
		return;
	}
	int labelRow = GetCurLabelRow();
	if (labelRow == -1)
	{
		return;
	}

	// make sure we're not adding the same seed twice:
	for (int l = 0; l < count(); ++l)
	{
		if (SeedAlreadyExists(m_itemModel->item(l), x, y, z))
		{
			return;
		}
	}

	AddSeedItem(labelRow, x, y, z);
	UpdateChannel();
}


void dlg_labels::SlicerClicked(int x, int y, int z)
{
	AddSeed(x, y, z);
}


void dlg_labels::SlicerRightClicked(int x, int y, int z)
{
	for (int l = 0; l < count(); ++l)
	{
		int idx = FindSeed(m_itemModel->item(l), x, y, z);
		if (idx != -1)
		{
			RemoveSeed(m_itemModel->item(l)->child(idx), x, y, z);
			break;
		}
	}
}


void dlg_labels::AddSeedItem(int labelRow, int x, int y, int z)
{
	m_itemModel->item(labelRow, 1)->setText(QString::number(m_itemModel->item(labelRow, 1)->text().toInt() + 1));
	m_itemModel->item(labelRow)->setChild(
		m_itemModel->item(labelRow)->rowCount(),
		GetCoordinateItem(x, y, z)
	);
	drawPixel(m_labelOverlayImg, x, y, z, labelRow+1);
}


int dlg_labels::AddLabelItem(QString const & labelText)
{
	if (!m_labelOverlayImg)
	{
		m_labelOverlayImg = vtkSmartPointer<iAvtkImageData>::New();
		m_labelOverlayImg->SetExtent(m_mdiChild->getImagePointer()->GetExtent());
		m_labelOverlayImg->SetSpacing(m_mdiChild->getImagePointer()->GetSpacing());
		m_labelOverlayImg->AllocateScalars(VTK_INT, 1);
		clearImage(m_labelOverlayImg, 0);
		iAChannelVisualizationData* chData = m_mdiChild->GetChannelData(ch_LabelOverlay);
		if (!chData)
		{
			chData = new iAChannelVisualizationData();
			m_mdiChild->InsertChannelData(ch_LabelOverlay, chData);
		}
	}
	QStandardItem* newItem = new QStandardItem(labelText);
	QStandardItem* newItemCount = new QStandardItem("0");
	newItem->setData(m_colorTheme->GetColor(m_maxColor++), Qt::DecorationRole);
	QList<QStandardItem* > newRow;
	newRow.append(newItem);
	newRow.append(newItemCount);
	m_itemModel->appendRow(newRow);
	return newItem->row();
}


void dlg_labels::Add()
{
	pbStore->setEnabled(true);
	int labelCount = count();
	AddLabelItem(QString::number( labelCount ));
	ReInitChannelTF();
}


void dlg_labels::ReInitChannelTF()
{
	m_labelOverlayLUT = BuildLabelOverlayLUT(count(), m_colorTheme);
	m_labelOverlayOTF = BuildLabelOverlayOTF(count());
}


void dlg_labels::UpdateChannel()
{
	m_labelOverlayImg->Modified();
	m_labelOverlayImg->SetScalarRange(0, count());
	m_mdiChild->reInitChannel(ch_LabelOverlay, m_labelOverlayImg, m_labelOverlayLUT, m_labelOverlayOTF);
	m_mdiChild->InitChannelRenderer(ch_LabelOverlay, false);
	m_mdiChild->updateViews();
}


void dlg_labels::Remove()
{
	QModelIndexList indices = lvLabels->selectionModel()->selectedIndexes();
	if (indices.size() == 0)
		return;
	QStandardItem* item = m_itemModel->itemFromIndex(indices[0]);
	if (!item)
		return;
	bool updateOverlay = true;
	if (item->parent() == nullptr)  // a label was selected
	{
		if (item->rowCount() > 0)
		{
			auto reply = QMessageBox::question(this, "Remove Label",
				QString("Are you sure you want to remove the whole label and all of its %1 seeds?").arg(item->rowCount()),
				QMessageBox::Yes | QMessageBox::No);
			if (reply != QMessageBox::Yes) {
				return;
			}
		}
		int curLabel = GetCurLabelRow();
		if (curLabel == -1)
		{
			return;
		}
		m_itemModel->removeRow(curLabel);
		if (count() == 0)
		{
			pbStore->setEnabled(false);
		}
	}
	else
	{							// remove a single seed
		int x = item->data(Qt::UserRole + 1).toInt();
		int y = item->data(Qt::UserRole + 2).toInt();
		int z = item->data(Qt::UserRole + 3).toInt();
		RemoveSeed(item, x, y, z);
	}
}


void dlg_labels::RemoveSeed(QStandardItem* item, int x, int y, int z)
{
	drawPixel(m_labelOverlayImg, x, y, z, 0);
	int labelRow = item->parent()->row();
	item->parent()->removeRow(item->row());
	m_itemModel->item(labelRow, 1)->setText(QString::number(m_itemModel->item(labelRow, 1)->text().toInt() - 1));
	UpdateChannel();
}


int dlg_labels::count() const
{
	return m_itemModel->rowCount();
}


QString dlg_labels::GetName(int idx) const
{
	QStandardItem * labelItem = m_itemModel->item(idx);
	return labelItem->text();
}


QColor dlg_labels::GetColor(int idx) const
{
	return m_itemModel->item(idx)->data(Qt::DecorationRole).value<QColor>();
}


int dlg_labels::GetCurLabelRow() const
{
	QModelIndexList indices = lvLabels->selectionModel()->selectedIndexes();
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


int dlg_labels::GetSeedCount(int labelIdx) const
{
	QStandardItem* labelItem = m_itemModel->item(labelIdx);
	return labelItem->rowCount();
}


bool dlg_labels::Load(QString const & filename)
{
	if (m_labelOverlayImg)
	{
		clearImage(m_labelOverlayImg, 0);
	}
	m_maxColor = 0;
	m_itemModel->clear();
	m_itemModel->setHorizontalHeaderItem(0, new QStandardItem("Label"));
	m_itemModel->setHorizontalHeaderItem(1, new QStandardItem("Count"));
	QFile file(filename);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{	
		DEBUG_LOG(QString("Seed file loading: Failed to open file '%1'!").arg(filename));
		return false;
	}
	QXmlStreamReader stream(&file);
	stream.readNext();
	int curLabelRow = -1;
	
	bool enableStoreBtn = false;
	while (!stream.atEnd())
	{
		if (stream.isStartElement())
		{
			if (stream.name() == "Labels")
			{
				// root element, no action required
			}
			else if (stream.name() == "Label")
			{
				enableStoreBtn = true;
				QString id = stream.attributes().value("id").toString();
				QString name = stream.attributes().value("name").toString();
				curLabelRow = AddLabelItem(name);
				if (m_itemModel->rowCount()-1 != id.toInt())
				{
					DEBUG_LOG(QString("Inserting row: rowCount %1 <-> label id %2 mismatch!")
						.arg(m_itemModel->rowCount())
						.arg(id) );
				}
			}
			else if (stream.name() == "Seed")
			{
				if (curLabelRow == -1)
				{
					DEBUG_LOG(QString("Error loading seed file '%1': Current label not set!")
						.arg(filename) );
					return false;
				}
				int x = stream.attributes().value("x").toInt();
				int y = stream.attributes().value("y").toInt();
				int z = stream.attributes().value("z").toInt();
				AddSeedItem(curLabelRow, x, y, z);
			}
		}
		stream.readNext();
	}

	file.close();
	if (stream.hasError())
	{
	   DEBUG_LOG(QString("Error: Failed to parse seed xml file '%1': %2")
		   .arg(filename)
		   .arg(stream.errorString()) );
	   return false;
	}
	else if (file.error() != QFile::NoError)
	{
		DEBUG_LOG(QString("Error: Cannot read file '%1': %2")
			.arg(filename )
			.arg(file.errorString()) );
	   return false;
	}

	QFileInfo fileInfo(file);
	m_fileName = MakeAbsolute(fileInfo.absolutePath(), filename);
	pbStore->setEnabled(enableStoreBtn);
	ReInitChannelTF();
	UpdateChannel();
	return true;
}


bool dlg_labels::Store(QString const & filename, bool extendedFormat)
{
	QFile file(filename);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
	{	
		QMessageBox::warning(this, "GEMSe", "Seed file storing: Failed to open file '" + filename + "'!");
		return false;
	}
	QFileInfo fileInfo(file);
	m_fileName = MakeAbsolute(fileInfo.absolutePath(), filename);
	QXmlStreamWriter stream(&file);
	stream.setAutoFormatting(true);
	stream.writeStartDocument();
	stream.writeStartElement("Labels");

	auto modalities = m_mdiChild->GetModalities();
	for (int l=0; l<m_itemModel->rowCount(); ++l)
	{
		QStandardItem * labelItem = m_itemModel->item(l);
		stream.writeStartElement("Label");
		stream.writeAttribute("id", QString::number(l));
		stream.writeAttribute("name", labelItem->text());
		for (int i=0; i<labelItem->rowCount(); ++i)
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
				for (int m = 0; m < modalities->size(); ++m)
				{
					auto mod = modalities->Get(m);
					for (int c = 0; c < mod->ComponentCount(); ++c)
					{
						double value = mod->GetComponent(c)->GetScalarComponentAsDouble(x, y, z, 0);
						stream.writeStartElement("Value");
						stream.writeAttribute("modality", QString::number(m));
						stream.writeAttribute("component", QString::number(c));
						stream.writeAttribute("value", QString::number(value, 'g', 16));
						stream.writeEndElement();
					}
				}
			}
			stream.writeEndElement();
		}
		stream.writeEndElement();
	}
	stream.writeEndElement();
	stream.writeEndDocument();
	file.close();
	return true;
}


void dlg_labels::Load()
{
	QString fileName = QFileDialog::getOpenFileName(
		QApplication::activeWindow(),
		tr("Open Seed File"),
		QString() // TODO get directory of current file
		,
		tr("Seed file (*.seed);;All files (*.*)" ) );
	if (fileName.isEmpty())
	{
		return;
	}
	if (!Load(fileName))
	{
		QMessageBox::warning(this, "GEMSe", "Loading seed file '" + fileName + "' failed!");
	}
}


void dlg_labels::Store()
{
	QString fileName = QFileDialog::getSaveFileName(
		QApplication::activeWindow(),
		tr("Save Seed File"),
		QString() // TODO get directory of current file
		,
		tr("Seed file (*.seed);;All files (*.*)" ) );
	if (fileName.isEmpty())
	{
		return;
	}
	QStringList inList;
	inList << tr("$Extended Format (also write pixel values, not only positions)");
	QList<QVariant> inPara;
	inPara << tr("%1").arg(true);
	dlg_commoninput extendedFormatInput(this, "Seed File Format", 1, inList, inPara, nullptr);
	if (extendedFormatInput.exec() != QDialog::Accepted)
	{
		DEBUG_LOG("Selection of format aborted, aborting seed file storing");
		return;
	}
	if (!Store(fileName, extendedFormatInput.getCheckValues()[0]))
	{
		QMessageBox::warning(this, "GEMSe", "Storing seed file '" + fileName + "' failed!");
	}
}


void dlg_labels::StoreImage()
{
	QString fileName = QFileDialog::getSaveFileName(
		QApplication::activeWindow(),
		tr("Save Seeds as Image"),
		QString() // TODO get directory of current file
		,
		tr("Seed file (*.mhd);;All files (*.*)" ) );
	if (fileName.isEmpty())
	{
		return;
	}
	vtkMetaImageWriter *metaImageWriter = vtkMetaImageWriter::New();
	metaImageWriter->SetFileName(fileName.toStdString().c_str());
	metaImageWriter->SetInputData(m_labelOverlayImg);
	metaImageWriter->SetCompression( false );
	metaImageWriter->Write();
	metaImageWriter->Delete();
}


bool haveAllSeeds(QVector<int> const & label2SeedCounts, std::vector<int> const & requiredNumOfSeedsPerLabel)
{
	for (int i=0; i<label2SeedCounts.size(); ++i)
	{
		if (label2SeedCounts[i] < requiredNumOfSeedsPerLabel[i])
		{
			return false;
		}
	}
	return true;
}


void dlg_labels::Sample()
{
	int gt = m_mdiChild->chooseModalityNr("Choose Ground Truth");
	vtkSmartPointer<vtkImageData> img = m_mdiChild->GetModality(gt)->GetImage();
	int labelCount = img->GetScalarRange()[1]+1;
	if (labelCount > 50)
	{
		auto reply = QMessageBox::question(this, "Sample Seeds",
			QString("According to min/max values in image, there are %1 labels, this seems a bit much. Do you really want to proceed?").arg(labelCount),
			QMessageBox::Yes | QMessageBox::No);
		if (reply != QMessageBox::Yes) {
			return;
		}
	}

	QStringList     inParams; inParams
		<< "*Number of Seeds per Label"
		<< "$Reduce seed number for all labels to size of smallest labeling";
	QList<QVariant> inValues; inValues
		<< 50
		<< true;
	dlg_commoninput input(this, "Sample Seeds", inParams.size(), inParams, inValues, nullptr);
	if (input.exec() != QDialog::Accepted)
	{
		return;
	}
	int numOfSeeds = input.getSpinBoxValues()[0];
	bool reduceNum = input.getCheckValues()[1];

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
			DEBUG_LOG(QString("Reducing number of seeds for label %1 to %2 because it only has %3 pixels")
				.arg(i).arg(numOfSeedsPerLabel[i]).arg(histogram[i]));
		}
		if (numOfSeedsPerLabel[i] < minNumOfSeeds)
		{
			minNumOfSeeds = numOfSeedsPerLabel[i];
		}
		if (m_itemModel->rowCount() <= i)
		{
			AddLabelItem(QString::number(i));
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
	while (!haveAllSeeds(label2SeedCount, numOfSeedsPerLabel))
	{
		int x = xDist(gen);
		int y = yDist(gen);
		int z = zDist(gen);
		int label = static_cast<int>(img->GetScalarComponentAsFloat(x, y, z, 0));

		if (label2SeedCount[label] < numOfSeedsPerLabel[label] && !SeedAlreadyExists(m_itemModel->item(label), x, y, z))
		{
			// m_itemModel->item(label)->appendRow(GetCoordinateItem(x, y, z));
			AddSeedItem(label, x, y, z);
			label2SeedCount[label]++;
		}
	}
	ReInitChannelTF();
	UpdateChannel();
	pbStore->setEnabled(true);
}


void dlg_labels::Clear()
{
	if (m_labelOverlayImg)
	{
		clearImage(m_labelOverlayImg, 0);
		UpdateChannel();
	}
	m_itemModel->clear();
	m_maxColor = 0;
}


QString const & dlg_labels::GetFileName()
{
	return m_fileName;
}


void dlg_labels::SetColorTheme(iAColorTheme const * colorTheme)
{
	m_colorTheme = colorTheme;
	m_maxColor = 0;
}
