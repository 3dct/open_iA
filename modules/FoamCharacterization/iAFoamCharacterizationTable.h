/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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

#include "iAFoamCharacterizationItem.h"

#include <QItemDelegate>
#include <QTableWidget>

class vtkImageData;

class QDropEvent;
class QPainter;

class iAFoamCharacterizationTable : public QTableWidget
{
	Q_OBJECT

	class iAFoamCharacterizationTableDelegate : public QItemDelegate
	{
	public:
		explicit iAFoamCharacterizationTableDelegate(iAFoamCharacterizationTable* _pTable, QObject* _pParent = nullptr);
		void paint(QPainter* _pPainter, const QStyleOptionViewItem& _sovItem, const QModelIndex& _miItem) const;
	private:
		int m_iMargin = 0;
		iAFoamCharacterizationTable* m_pTable = nullptr;
		QColor colorModified(const QColor& _cColor) const;
		void drawItemRect(QPainter* _pPainter, const QRect& _rItem, const QColor& _cColor) const;
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
	void reset() override;
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
	void dropEvent(QDropEvent* e) override;
	void keyPressEvent(QKeyEvent* e) override;
	void mouseDoubleClickEvent(QMouseEvent* e) override;
	void mousePressEvent(QMouseEvent* e) override;
	void mouseReleaseEvent(QMouseEvent* e) override;
	void resizeEvent(QResizeEvent* e) override;
};
