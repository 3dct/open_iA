// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QWidget>

#include <memory>

struct iANModalLabel;

class QGridLayout;
class QLabel;
class QSlider;

class iANModalLabelsWidget : public QWidget
{
	Q_OBJECT

public:
	iANModalLabelsWidget(QWidget* parent = nullptr);

	void updateTable(const QList<iANModalLabel>&);
	void insertLabel(int row, iANModalLabel, float opacity);
	void removeLabel(int row);
	bool containsLabel(int row);

	float opacity(int row);

	int row(int labelId);

private:
	enum Column
	{
		COLOR = 0,
		OPACITY = 1
	};

	struct Row
	{
		Row()
		{
		}
		Row(int _row, QLabel* _color, QSlider* _opacity) : row(_row), color(_color), opacity(_opacity)
		{
		}
		int row = -1;
		QLabel* color = nullptr;
		QSlider* opacity = nullptr;
	};

	QGridLayout* m_layout;
	QVector<iANModalLabel> m_labels;
	QVector<Row> m_rows;

	int m_nextId = 0;

	void addRow(int row, iANModalLabel, float opacity);
	void updateRow(int row, iANModalLabel);

signals:
	void labelOpacityChanged(int labelId);
};
