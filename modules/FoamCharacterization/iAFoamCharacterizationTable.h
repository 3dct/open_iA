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
						const QColor cItemSelected(m_pTable->palette().color(QPalette::Highlight));

						_pPainter->fillRect(_sovItem.rect, cItemSelected);
						_pPainter->setPen(m_pTable->palette().color(QPalette::BrightText));
					}
					else
					{
						const QColor cItem (m_pTable->palette().color(QPalette::Window));

						_pPainter->fillRect(_sovItem.rect, cItem);
						_pPainter->setPen(m_pTable->palette().color(QPalette::WindowText));
					}
				}
				else
				{
					const QColor cItem(m_pTable->palette().color(QPalette::Window));

					_pPainter->fillRect(_sovItem.rect, cItem);
					_pPainter->setPen(m_pTable->palette().color(QPalette::WindowText));
				}

				iAFoamCharacterizationItem* pItem((iAFoamCharacterizationItem*)m_pTable->item(iRow, 0));

				const int iMargin(100 * m_pTable->logicalDpiX() / 254);

				const QRect rText(_sovItem.rect.adjusted(iMargin, 0, -iMargin, 0));

				QIcon iItemIcon (pItem->icon());
				const QSize sItemIcon (_sovItem.decorationSize);

				_pPainter->drawPixmap ( (rText.left() - sItemIcon.width()) / 2
					                  , rText.top() + (rText.height() - sItemIcon.height()) / 2
								      , iItemIcon.pixmap(sItemIcon.width(), sItemIcon.height())
									  );

				_pPainter->setFont(pItem->font());
				_pPainter->drawText(rText, Qt::AlignLeft | Qt::AlignVCenter, _miItem.data().toString());
				_pPainter->drawText(rText, Qt::AlignRight | Qt::AlignVCenter, pItem->executeTimeString());
			}

		private:
			iAFoamCharacterizationTable* m_pTable = nullptr;

		protected:
			virtual QSize sizeHint(const QStyleOptionViewItem& _sovItem, const QModelIndex& _miItem) const override
			{
				return _sovItem.rect.size();
			}
	};

	public:
		explicit iAFoamCharacterizationTable(vtkImageData* _pImageData, QWidget* _pParent = nullptr);

		void addBinarization();
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
		int m_iCountFilter = 0;
		int m_iCountWatershed = 0;

		vtkImageData* m_pImageData = nullptr;

	protected:
		virtual void dropEvent(QDropEvent* e) override;
		virtual void keyPressEvent(QKeyEvent* e) override;
		virtual void mouseDoubleClickEvent(QMouseEvent* e) override;
		virtual void mousePressEvent(QMouseEvent* e) override;
		virtual void resizeEvent(QResizeEvent*) override;
};
