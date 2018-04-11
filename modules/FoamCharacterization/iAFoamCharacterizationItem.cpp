/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
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
#include "iAFoamCharacterizationItem.h"

#include "iAFoamCharacterizationDialog.h"
#include "iAFoamCharacterizationTable.h"

#include <vtkImageData.h>

#include <QApplication>
#include <QPainter>
#include <QFile>
#include <QTextStream>

iAFoamCharacterizationItem::iAFoamCharacterizationItem ( iAFoamCharacterizationTable* _pTable
													   , vtkImageData* _pImageData, const EItemType& _eItemType
													   ) : QObject(_pTable)
														 , QTableWidgetItem()
	                                                     , m_pTable (_pTable)
														 , m_eItemType(_eItemType)
														 , m_pImageData(_pImageData)
{
	QFont f(font());
	f.setBold(true);

	setFont(f);

	setItemIconColor();
	setItemIcon();

	setName(itemTypeStr());
}

iAFoamCharacterizationItem::iAFoamCharacterizationItem(iAFoamCharacterizationItem* _pItem) : QObject(_pItem->table())
																						, QTableWidgetItem()
																						, m_bModified(_pItem->modified())
																						, m_dExecuteTime(_pItem->executeTime())
																						, m_pTable(_pItem->table())
																						, m_eItemType(_pItem->itemType())
																						, m_pImageData(_pItem->imageData())
{
	QFont f(font());
	f.setBold(true);

	setFont(f);

	setItemIconColor();
	setItemIcon();

	setItemEnabled(_pItem->itemEnabled());
}

iAFoamCharacterizationItem::~iAFoamCharacterizationItem()
{

}

double iAFoamCharacterizationItem::executeTime() const
{
	return m_dExecuteTime;
}

bool iAFoamCharacterizationItem::executing() const
{
	return m_bExecuting;
}

QString iAFoamCharacterizationItem::executeTimeString() const
{
	if (m_dExecuteTime > 0.0)
	{
		const unsigned int iSecond((int)m_dExecuteTime);

		const unsigned int iH(iSecond / 3600);
		const unsigned int iM(iSecond / 60 - 60 * iH);
		const double dS(m_dExecuteTime - (double) (3600 * iH + 60 * iM));

		if (iH)
		{
			return QString("%1 h %2 m %3 s").arg(iH).arg(iM).arg(dS);
		}
		else if (iM)
		{
			return QString("%1 m %2 s").arg(iM).arg(dS);
		}
		else
		{
			return QString("%1 s").arg(dS);
		}
	}
	else
	{
		return "";
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
	QScopedPointer<QImage> pImage(new QImage(1, 1, QImage::Format_ARGB32));

	const int iImageLengthX(pImage->logicalDpiX() / 6);
	const int iImageLengthY(pImage->logicalDpiY() / 6);

	*pImage.data() = pImage->scaled(iImageLengthX, iImageLengthY);

	pImage->fill(0);

	const int iImageLengthX2(iImageLengthX / 2);
	const int iImageLengthY2(iImageLengthY / 2);

	QScopedPointer<QPainter> pPainter(new QPainter(pImage.data()));
	pPainter->setBrush(Qt::NoBrush);
	pPainter->setPen(m_cItemIcon);
	pPainter->drawEllipse(pImage->rect().adjusted(0, 0, -1, -1));
	pPainter->drawLine(2, iImageLengthY2, iImageLengthX - 3, iImageLengthY2);
	pPainter->drawLine(iImageLengthX2, 2, iImageLengthX2, iImageLengthY - 3);

	return QIcon(QPixmap::fromImage(*pImage.data()));
}

bool iAFoamCharacterizationItem::itemEnabled() const
{
	return m_bItemEnabled;
}

QColor iAFoamCharacterizationItem::itemIconColor() const
{
	return m_cItemIcon;
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

		case itDistanceTransform:
		return "Distance transform";
		break;

		case itFilter:
		return "Filter";
		break;

		default:
		return "Watershed";
		break;
	}
}

bool iAFoamCharacterizationItem::modified() const
{
	return m_bModified;
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

int iAFoamCharacterizationItem::progress() const
{
	return m_iProgress;
}

void iAFoamCharacterizationItem::reset()
{
	m_bModified = false;

	m_dExecuteTime = 0.0;
}

void iAFoamCharacterizationItem::save(QFile* _pFileSave)
{
	_pFileSave->write((char*)&m_eItemType, sizeof(m_eItemType));

	fileWrite(_pFileSave, m_sName);

	_pFileSave->write((char*)&m_bItemEnabled, sizeof(m_bItemEnabled));
}

void iAFoamCharacterizationItem::setExecuting(const bool& _bExecuting)
{
	m_bExecuting = _bExecuting;

	m_iProgress = 0;

	m_pTable->viewport()->repaint();
}

void iAFoamCharacterizationItem::setItemIcon()
{
	QScopedPointer<QImage> pImage(new QImage(1, 1, QImage::Format_ARGB32));

	*pImage.data() = pImage->scaled(pImage->logicalDpiX() / 6, pImage->logicalDpiY() / 6);

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

		case itDistanceTransform:
		m_cItemIcon = Qt::cyan;
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
	setText(m_sName);
}

void iAFoamCharacterizationItem::setModified(const bool& _bModified)
{
	m_bModified = _bModified;
}

void iAFoamCharacterizationItem::setName(const QString& _sName)
{
	m_sName = _sName;

	setItemText();
}

void iAFoamCharacterizationItem::setProgress(const unsigned int& _uiProgress)
{
	m_iProgress = _uiProgress;

	m_pTable->viewport()->repaint();
}

void iAFoamCharacterizationItem::slotObserver(const int& _iValue)
{
	setProgress(_iValue);
}

iAFoamCharacterizationTable* iAFoamCharacterizationItem::table()
{
	return m_pTable;
}
