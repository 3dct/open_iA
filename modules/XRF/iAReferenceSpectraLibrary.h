/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
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

#include "iAElementSpectralInfo.h"

#include <QStandardItemModel>

/**
 * \class	iAReferenceSpectraLibrary
 *
 * \brief	Loads the reference spectra library from the .reflib file
 *
 */

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
		QRegExp splitter("\\s+");
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

		for(int i=0; i<spectra.size(); ++i)
		{
			QStandardItem * item = new QStandardItem(spectra[i].GetName()); //TODO: make sure that the memory does not leak
			item->setData(QColor(255, 0, 0, 255), Qt::DecorationRole);
			item->setData(QVariant(0), Qt::UserRole);
			item->setData(QVariant(-1), Qt::UserRole+1);
			item->setCheckable(true);
			m_refSpectraItemModel->setItem(i, item);
		}
	}
	QSharedPointer<QStandardItemModel> getItemModel()
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
	QSharedPointer<QStandardItemModel> m_refSpectraItemModel;
};
