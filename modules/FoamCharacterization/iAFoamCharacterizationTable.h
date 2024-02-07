// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAFoamCharacterizationItem.h"

#include <QItemDelegate>
#include <QTableWidget>

class iAMdiChild;

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
	explicit iAFoamCharacterizationTable(iAMdiChild* child, QWidget* _pParent = nullptr);

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

	iAMdiChild* m_child;

protected:
	void dropEvent(QDropEvent* e) override;
	void keyPressEvent(QKeyEvent* e) override;
	void mouseDoubleClickEvent(QMouseEvent* e) override;
	void mousePressEvent(QMouseEvent* e) override;
	void mouseReleaseEvent(QMouseEvent* e) override;
	void resizeEvent(QResizeEvent* e) override;
};
