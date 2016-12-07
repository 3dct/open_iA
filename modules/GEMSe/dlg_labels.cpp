/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenb�ck, *
*                     Artem & Alexander Amirkhanov, B. Fr�hler                        *
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
* Contact: FH O� Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstra�e 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
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


dlg_labels::dlg_labels(MdiChild* mdiChild, iAColorTheme const * colorTheme):
	m_itemModel(new QStandardItemModel()),
	m_colorTheme(colorTheme),
	m_mdiChild(mdiChild),
	m_labelOverlayThread(0)
{
	connect(pbAdd, SIGNAL(clicked()), this, SLOT(Add()));
	connect(pbRemove, SIGNAL(clicked()), this, SLOT(Remove()));
	connect(pbStore, SIGNAL(clicked()), this, SLOT(Store()));
	connect(pbLoad, SIGNAL(clicked()), this, SLOT(Load()));
	connect(pbStoreImage, SIGNAL(clicked()), this, SLOT(StoreImage()));
	m_itemModel->setHorizontalHeaderItem(0, new QStandardItem("Label"));
	lvLabels->setModel(m_itemModel);
}

QStandardItem* GetCoordinateItem(int x, int y, int z)
{
	QStandardItem* item = new QStandardItem("("+QString::number(x)+", "+QString::number(y)+", "+QString::number(z)+")");
	item->setData(x, Qt::UserRole + 1);
	item->setData(y, Qt::UserRole + 2);
	item->setData(z, Qt::UserRole + 3);
	return item;
}

void dlg_labels::RendererClicked(int x, int y, int z)
{
	AddSeed(x, y, z);
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
	QList<iAImageCoordinate> coordinates = GetSeeds(labelRow);
	for (auto coord: coordinates)
	{
		if (coord.x == x && coord.y == y && coord.z == z)
		{
			return;
		}
	}
	
	AddSeedItem(labelRow, x, y, z);
	
	UpdateOverlay();
}

void dlg_labels::SlicerClicked(int x, int y, int z)
{
	AddSeed(x, y, z);
}


void dlg_labels::AddSeedItem(int labelRow, int x, int y, int z)
{
	m_itemModel->item(labelRow)->setChild(
		m_itemModel->item(labelRow)->rowCount(),
		GetCoordinateItem(x, y, z)
	);
	emit SeedsAvailable();
}


int dlg_labels::AddLabelItem(QString const & labelText)
{
	QStandardItem* newItem = new QStandardItem(labelText);
	newItem->setData(m_colorTheme->GetColor(m_itemModel->rowCount()), Qt::DecorationRole);
	m_itemModel->appendRow(newItem);
	return newItem->row();
}

void dlg_labels::Add()
{
	pbStore->setEnabled(true);
	int labelCount = count();
	AddLabelItem(QString::number( labelCount ));
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
		int curLabel = GetCurLabelRow();
		if (curLabel == -1)
		{
			return;
		}
		updateOverlay = (GetSeedCount(curLabel) > 0);
		m_itemModel->removeRow(curLabel);
		ReColorExistingLabels();
		// recolor existing labels:
	

		if (count() == 0)
		{
			pbStore->setEnabled(false);
		}
	}
	else
	{							// remove a single seed
		item->parent()->removeRow(item->row());
	}
	if (updateOverlay)
	{
		UpdateOverlay();
	}
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

QList<iAImageCoordinate> dlg_labels::GetSeeds(int labelIdx) const
{
	QStandardItem* labelItem = m_itemModel->item(labelIdx);
	QList<iAImageCoordinate> result;
	for (int i=0; i<labelItem->rowCount(); ++i)
	{
		iAImageCoordinate coord;
		QStandardItem* coordItem = labelItem->child(i);
		coord.x = coordItem->data(Qt::UserRole + 1).toInt();
		coord.y = coordItem->data(Qt::UserRole + 2).toInt();
		coord.z = coordItem->data(Qt::UserRole + 3).toInt();
		result.append(coord);
	}
	return result;
}

void dlg_labels::StartOverlayCreation()
{
	m_labelOverlayThread = new iALabelOverlayThread(
			m_labelOverlayImg,
			m_labelOverlayLUT,
			m_labelOverlayOTF,
			m_itemModel,
			count(),
			m_colorTheme,
			m_mdiChild->getImagePointer()->GetExtent(),
			m_mdiChild->getImagePointer()->GetSpacing()
		);
	connect(m_labelOverlayThread, SIGNAL(finished()), this, SLOT(LabelOverlayReady()));
	m_labelOverlayThread->start();
}

void dlg_labels::UpdateOverlay()
{
	if (!m_labelOverlayThread)
	{
		StartOverlayCreation();
	}
	else
	{
		m_newOverlay = true;
	}

}

void dlg_labels::LabelOverlayReady()
{
	iAChannelVisualizationData* chData = m_mdiChild->GetChannelData(ch_LabelOverlay);
	if (!chData)
	{
		chData = new iAChannelVisualizationData();
		m_mdiChild->InsertChannelData(ch_LabelOverlay, chData);
	}
///	assert(chData);										// TODO: check/refactor the need for OTF. shouldn't be needed for slicer overlay alone!
	m_mdiChild->reInitChannel(ch_LabelOverlay, m_labelOverlayImg, m_labelOverlayLUT, m_labelOverlayOTF);
	m_mdiChild->InitChannelRenderer(ch_LabelOverlay, false);
	// m_mdiChild->UpdateChannelSlicerOpacity(ch_LabelOverlay, 1);
	//m_mdiChild->updateSlicers();
	m_mdiChild->updateViews();
	if (m_newOverlay)
	{
		StartOverlayCreation();
		m_newOverlay = false;
	}
	else
	{
		m_labelOverlayThread = nullptr;
	}
}


bool dlg_labels::Load(QString const & filename)
{
	m_itemModel->clear();
	m_itemModel->setHorizontalHeaderItem(0, new QStandardItem("Label"));
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

	UpdateOverlay();

	pbStore->setEnabled(enableStoreBtn);
	emit SeedsAvailable();
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

QString const & dlg_labels::GetFileName()
{
	return m_fileName;
}

void dlg_labels::ReColorExistingLabels()
{
	for (int i=0; i<m_itemModel->rowCount(); ++i)
	{
		m_itemModel->item(i)->setData(m_colorTheme->GetColor(i), Qt::DecorationRole);
	}
}

void dlg_labels::SetColorTheme(iAColorTheme const * colorTheme)
{
	m_colorTheme = colorTheme;
	ReColorExistingLabels();
	UpdateOverlay();
}

bool dlg_labels::AreSeedsAvailable() const
{
	for (int label = 0; label<count(); ++label)
	{
		if (GetSeedCount(label) > 0)
		{
			return true;
		}
	}
	return false;
}
