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

#include <QTableWidget>

#include <QItemDelegate>
#include <QPainter>

#include "iAFoamCharacterizationItem.h"

class QDropEvent;
class vtkImageData;

class iAFoamCharacterizationTable : public QTableWidget
{
	Q_OBJECT

	class iAFoamCharacterizationTableDelegate : public QItemDelegate
	{
		public:
			explicit iAFoamCharacterizationTableDelegate(iAFoamCharacterizationTable* _pTable, QObject* _pParent = nullptr) 
																										 : QItemDelegate(_pParent)
																										 , m_pTable(_pTable)
			{
				m_iMargin = 100 * m_pTable->logicalDpiX() / 254;
			}

			void paint(QPainter* _pPainter, const QStyleOptionViewItem& _sovItem, const QModelIndex& _miItem) const
			{
				const int iRow(_miItem.row());

				QModelIndexList mlIndex(m_pTable->selectedIndexes());

				if ((mlIndex.size()) && (m_pTable->hasFocus()))
				{
					const int iRowSelected(mlIndex.at(0).row());

					if (iRow == iRowSelected)
					{
						drawItemRect(_pPainter, _sovItem.rect, m_pTable->palette().color(QPalette::Highlight));
						_pPainter->setPen(m_pTable->palette().color(QPalette::BrightText));
					}
					else
					{
						drawItemRect(_pPainter, _sovItem.rect, m_pTable->palette().color(QPalette::Window));
						_pPainter->setPen(m_pTable->palette().color(QPalette::WindowText));
					}
				}
				else
				{
					drawItemRect(_pPainter, _sovItem.rect, m_pTable->palette().color(QPalette::Window));
					_pPainter->setPen(m_pTable->palette().color(QPalette::WindowText));
				}

				iAFoamCharacterizationItem* pItem ((iAFoamCharacterizationItem*) m_pTable->item(iRow, 0));

				const QRect rText(_sovItem.rect.adjusted(m_iMargin, 0, -m_iMargin, 0));

				QIcon iItemIcon (pItem->icon());

				const QSize sItemIcon (_sovItem.decorationSize);

				_pPainter->drawPixmap ( (rText.left() - sItemIcon.width()) / 2
					                  , rText.top() + (rText.height() - sItemIcon.height()) / 2
								      , iItemIcon.pixmap(sItemIcon.width(), sItemIcon.height())
									  );

				_pPainter->setFont(pItem->font());

				_pPainter->drawText
				            (rText.adjusted(m_iMargin / 4, 0, 0, 0), Qt::AlignLeft | Qt::AlignVCenter, _miItem.data().toString());

				if (pItem->executing())
				{
					const int iProgress (pItem->progress());

					if (iProgress > 0)
					{
						const QRect rProgress ( _sovItem.rect.width() - 3 * m_iMargin
							                  , _sovItem.rect.top() + _sovItem.rect.height() / 4
										      , 2 * m_iMargin, _sovItem.rect.height() / 2
											  );

						_pPainter->drawRect(rProgress);

						QRect rBar (rProgress.adjusted(1, 1, 0, 0));
						rBar.setWidth(rBar.width() * iProgress / 100);

						QLinearGradient lg (rBar.topLeft(), rBar.topRight());
						lg.setColorAt(0.0, Qt::black);
						lg.setColorAt(1.0, pItem->itemIconColor());

						_pPainter->fillRect(rBar, QBrush(lg));
					}
				}
				else
				{
					const QString sTime(pItem->executeTimeString());

					_pPainter->setPen((pItem->modified()) ? colorModified(_pPainter->pen().color()) : _pPainter->pen().color());
					_pPainter->drawText(rText, Qt::AlignRight | Qt::AlignVCenter, sTime);
				}
			}

		private:
			int m_iMargin = 0;

			iAFoamCharacterizationTable* m_pTable = nullptr;

			QColor colorModified(const QColor& _cColor) const
			{
				return ((_cColor.lightness() > 200) ? _cColor.darker(125) : Qt::gray);
			}

			void drawItemRect(QPainter* _pPainter, const QRect& _rItem, const QColor& _cColor) const
			{
				_pPainter->setBrush(colorModified(_cColor));
				_pPainter->setPen(_cColor);
				_pPainter->drawRect(_rItem.adjusted(0, 0, m_iMargin - _rItem.width(), -1));

				_pPainter->fillRect(_rItem.adjusted(m_iMargin, 0, 0, 0), _cColor);
			}
	};

	public:
		explicit iAFoamCharacterizationTable(vtkImageData* _pImageData, QWidget* _pParent = nullptr);

		void addBinarization();
		void addDistanceTransform();
		void addFilter();
		void addWatershed();

		void clear();
		void execute();
		void open(const QString& _sFilename);
		void reset();
		void save(const QString& _sFilename);

	private:
		int m_iRowDrag = -1;
		int m_iRowDrop = -1;

		int m_iCountBinarization = 0;
		int m_iCountDistanceTransform = 0;
		int m_iCountFilter = 0;
		int m_iCountWatershed = 0;

		vtkImageData* m_pImageData = nullptr;

	protected:
		virtual void dropEvent(QDropEvent* e) override;
		virtual void keyPressEvent(QKeyEvent* e) override;
		virtual void mouseDoubleClickEvent(QMouseEvent* e) override;
		virtual void mousePressEvent(QMouseEvent* e) override;
		virtual void mouseReleaseEvent(QMouseEvent* e) override;
		virtual void resizeEvent(QResizeEvent* e) override;
};
