// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAElementSpectralInfo.h"

#include <QStandardItemModel>

//! Loads the reference spectra library from the .reflib file
class iAReferenceSpectraLibrary
{
public:
	iAReferenceSpectraLibrary(QString const & fileName):
		m_refSpectraItemModel(new QStandardItemModel())
	{
		if(!QFileInfo(fileName).exists())
			return;

		QFile file(fileName);
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
			return;

		//read the data
		QStringList stringList;
		QRegularExpression splitter("\\s+");
		QFileInfo fi(fileName);
		QString fileNamesBase = fi.absolutePath() + "/";
		QString curFileName;
		while (!file.atEnd())
		{
			stringList = QString(file.readLine()).trimmed().split(splitter);
			curFileName = fileNamesBase + stringList[1];
			if(QFileInfo(curFileName).exists())
				spectra.push_back( iAElementSpectralInfo(stringList[0], curFileName) );
		}
		file.close(); //cleanup

		m_refSpectraItemModel->setHorizontalHeaderItem(0, new QStandardItem("Element Name"));

		for(size_t i=0; i<spectra.size(); ++i)
		{
			QStandardItem * item = new QStandardItem(spectra[i].name()); //TODO: make sure that the memory does not leak
			item->setData(QColor(255, 0, 0, 255), Qt::DecorationRole);
			item->setData(QVariant(0), Qt::UserRole);
			item->setData(QVariant(-1), Qt::UserRole+1);
			item->setCheckable(true);
			m_refSpectraItemModel->setItem(static_cast<int>(i), item);
		}
	}
	std::shared_ptr<QStandardItemModel> getItemModel()
	{
		return m_refSpectraItemModel;
	}
	QColor getElementColor(int idx)
	{
		return m_refSpectraItemModel->item(idx)->data(Qt::DecorationRole).value<QColor>();
	}
	QColor getElementColor(QModelIndex const & idx)
	{
		return m_refSpectraItemModel->itemFromIndex(idx)->data(Qt::DecorationRole).value<QColor>();
	}
	int getElementOpacity(QModelIndex const & idx, int & opacity)
	{
		QStandardItem* selected = m_refSpectraItemModel->itemFromIndex(idx);
		int channelIdx = selected->data(Qt::UserRole+1).toInt();
		if (channelIdx > -1)
		{
			opacity = selected->data(Qt::UserRole).toInt();
			return true;
		}
		return false;
	}
	void setElementOpacity(int idx, int opacity)
	{
		m_refSpectraItemModel->item(idx)->setData(QVariant(opacity), Qt::UserRole);
	}
	void setElementOpacity(QModelIndex const & idx, int opacity)
	{
		m_refSpectraItemModel->itemFromIndex(idx)->setData(QVariant(opacity), Qt::UserRole);
	}
	void setElementChannel(int idx, int channel)
	{
		m_refSpectraItemModel->item(idx)->setData(QVariant(channel), Qt::UserRole+1);
	}
	int getElementChannel(QModelIndex const & idx)
	{
		return m_refSpectraItemModel->itemFromIndex(idx)->data(Qt::UserRole+1).toInt();
	}
	std::vector<iAElementSpectralInfo> spectra;
private:
	std::shared_ptr<QStandardItemModel> m_refSpectraItemModel;
};
