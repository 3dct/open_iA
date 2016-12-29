/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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

#include "iAFoamCharacterizationItem.h"

#include <QApplication>
#include <QPainter>
#include <QFile>
#include <QTextStream>

#include <vtkImageData.h>

#include "iAFoamCharacterizationDialog.h"

iAFoamCharacterizationItem::iAFoamCharacterizationItem(vtkImageData* _pImageData, const EItemType& _eItemType)
	                                                                                                   : QTableWidgetItem()
																									   , m_eItemType(_eItemType)
																									   , m_pImageData(_pImageData)
{
	QFont f(font());
	f.setBold(true);

	setFont(f);

	setItemIconColor();
	setItemIcon();

	m_sName = itemTypeStr();

	setText(m_sName);
}


iAFoamCharacterizationItem::~iAFoamCharacterizationItem()
{

}

QString iAFoamCharacterizationItem::executeTimeString() const
{
	const int iSecond((int) m_dExecute);

	const int iH(iSecond / 3600);
	const int iM((iSecond - 3600 * iH) / 60);
	const int iS(iSecond - 3600 * iH - 60 * iM);

	if (iH)
	{
		return QString("%1 h %2 m %3 s").arg(iH).arg(iM).arg(iS);
	}
	else if (iM)
	{
		return QString("%1 m %2 s").arg(iM).arg(iS);
	}
	else
	{
		return QString("%1 s").arg(m_dExecute);
	}
}

QString iAFoamCharacterizationItem::fileRead(QFile* _pFileOpen)
{
	int iText;
	_pFileOpen->read((char*)&iText, sizeof(iText));

	QScopedPointer<char> pText(new char[iText]);
	_pFileOpen->read(pText.data(), iText);

	return QString(pText.data()).mid(0, iText);
}

void iAFoamCharacterizationItem::fileWrite(QFile* _pFileSave, const QString& _sText)
{
	const int iText(_sText.length());
	_pFileSave->write((char*)&iText, sizeof(iText));

	_pFileSave->write((char*)_sText.toStdString().c_str(), iText);
}

vtkImageData* iAFoamCharacterizationItem::imageData() const
{
	return m_pImageData;
}

QIcon iAFoamCharacterizationItem::itemButtonIcon() const
{
	const int iImageLength(18 * qMax(font().pixelSize(), font().pointSize()) / 10);
	const int iImageLength2(iImageLength / 2);

	QScopedPointer<QImage> pImage(new QImage(iImageLength, iImageLength, QImage::Format_ARGB32));
	pImage->fill(0);

	QScopedPointer<QPainter> pPainter(new QPainter(pImage.data()));
	pPainter->setBrush(Qt::NoBrush);
	pPainter->setPen(m_cItemIcon);
	pPainter->drawEllipse(pImage->rect().adjusted(0, 0, -1, -1));
	pPainter->drawLine(2, iImageLength2, iImageLength - 3, iImageLength2);
	pPainter->drawLine(iImageLength2, 2, iImageLength2, iImageLength - 3);

	return QIcon(QPixmap::fromImage(*pImage.data()));
}

bool iAFoamCharacterizationItem::itemEnabled() const
{
	return m_bItemEnabled;
}

iAFoamCharacterizationItem::EItemType iAFoamCharacterizationItem::itemType() const
{
	return m_eItemType;
}

QString iAFoamCharacterizationItem::itemTypeStr() const
{
	switch (m_eItemType)
	{
		case itBinarization:
		return "Binarization";
		break;

		case itFilter:
		return "Filter";
		break;

		default:
		return "Watershed";
		break;
	}
}

QString iAFoamCharacterizationItem::name() const
{
	return m_sName;
}

void iAFoamCharacterizationItem::open(QFile* _pFileOpen)
{
	m_sName = fileRead(_pFileOpen);

	_pFileOpen->read((char*) &m_bItemEnabled, sizeof(m_bItemEnabled));
	setItemIcon();
}

void iAFoamCharacterizationItem::save(QFile* _pFileSave)
{
	_pFileSave->write((char*)&m_eItemType, sizeof(m_eItemType));

	fileWrite(_pFileSave, m_sName);
	_pFileSave->write((char*)&m_bItemEnabled, sizeof(m_bItemEnabled));
}

void iAFoamCharacterizationItem::setItemIcon()
{
	const int iImageLength(qMax(font().pixelSize(), font().pointSize()));

	QScopedPointer<QImage> pImage(new QImage(iImageLength, iImageLength, QImage::Format_ARGB32));
	pImage->fill(0);

	QScopedPointer<QPainter> pPainter(new QPainter(pImage.data()));
	pPainter->setBrush((m_bItemEnabled) ? QBrush(m_cItemIcon) : Qt::NoBrush);
	pPainter->setPen(m_cItemIcon);
	pPainter->drawEllipse(pImage->rect().adjusted(0, 0, -1, -1));

	setIcon(QIcon(QPixmap::fromImage(*pImage.data())));
}

void iAFoamCharacterizationItem::setItemIconColor()
{
	switch (m_eItemType)
	{
		case itBinarization:
		m_cItemIcon = Qt::green;
		break;

		case itFilter:
		m_cItemIcon = Qt::red;
		break;

		default:
		m_cItemIcon = Qt::blue;
		break;
	}
}

void iAFoamCharacterizationItem::setItemEnabled(const bool& _bItemEnabled)
{
	m_bItemEnabled = _bItemEnabled;

	setItemIcon();
}

void iAFoamCharacterizationItem::setItemText()
{
	if (m_dExecute > 0.0)
	{
		setText(m_sName + QString(" (%1)").arg(executeTimeString()));
	}
	else
	{
		setText(m_sName);
	}
}

void iAFoamCharacterizationItem::setName(const QString& _sName)
{
	m_sName = _sName;

	setItemText();
}

void iAFoamCharacterizationItem::setNameTime(const QString& _sName)
{
	m_sName = _sName;
	m_dExecute = 0.0;

	setItemText();
}

void iAFoamCharacterizationItem::setTime(const int& _iMiliSeconds)
{
	m_dExecute = 0.001 * (double)_iMiliSeconds;

	setItemText();
}