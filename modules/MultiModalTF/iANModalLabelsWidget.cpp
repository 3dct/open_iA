// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iANModalLabelsWidget.h"

#include "iANModalObjects.h"

#include <QCheckBox>
#include <QGridLayout>
#include <QLabel>
#include <QPalette>
#include <QSlider>

#include <cassert>

namespace
{
	inline void setLabelColor(QLabel* label, QColor color)
	{
		QPalette palette = label->palette();
		palette.setColor(label->backgroundRole(), color);
		label->setAutoFillBackground(true);
		label->setPalette(palette);
	}
	const static int SLIDER_MAX = 65535;
	inline void setOpacity(QSlider* slider, float opacityF)
	{
		int opacity = opacityF * SLIDER_MAX;
		slider->setValue(opacity);
	}
	inline float getOpacity(QSlider* slider)
	{
		return ((float)slider->value()) / SLIDER_MAX;
	}
}

iANModalLabelsWidget::iANModalLabelsWidget(QWidget* parent)
{
	setParent(parent);
	m_layout = new QGridLayout(this);
}

void iANModalLabelsWidget::updateTable(const QList<iANModalLabel>& labels)
{
	int i;
	for (i = 0; i < labels.size(); i++)
	{
		auto label = labels[i];
		float opacity = containsLabel(i) ? getOpacity(m_rows[i].opacity) : label.opacity;
		insertLabel(i, label, opacity);
	}
	int size = i;
	for (; i < m_rows.size(); i++)
	{
		removeLabel(i);
	}
	m_labels.resize(size);
	m_rows.resize(size);
}

void iANModalLabelsWidget::insertLabel(int rowIndex, iANModalLabel label, float opacity)
{
	if (rowIndex >= m_labels.size())
	{
		m_labels.resize(rowIndex + 1);
		m_labels[rowIndex] = label;
		addRow(rowIndex, label, opacity);
	}
	else
	{
		m_labels[rowIndex] = label;
		updateRow(rowIndex, label);
	}
}

void iANModalLabelsWidget::removeLabel(int rowIndex)
{
	assert(containsLabel(rowIndex));
	Row row = m_rows[rowIndex];
	if (row.row != -1)
	{
		m_rows.removeAt(rowIndex);
		m_labels.removeAt(rowIndex);

		//m_layout->removeWidget(row.color);
		//m_layout->removeWidget(row.opacity);
		//row.color->setParent(nullptr);
		//row.opacity->setParent(nullptr);
		row.color->deleteLater();
		row.opacity->deleteLater();
	}
}

bool iANModalLabelsWidget::containsLabel(int rowIndex)
{
	return rowIndex < m_rows.size();
}

float iANModalLabelsWidget::opacity(int row)
{
	assert(containsLabel(row));
	return getOpacity(m_rows[row].opacity);
}

int iANModalLabelsWidget::row(int labelId)
{
	//return m_labels.lastIndexOf(label);
	for (int i = 0; i < m_labels.size(); ++i)
	{
		auto label = m_labels[i];
		if (label.id == labelId)
			return i;
	}
	return -1;
}

void iANModalLabelsWidget::addRow(int rowIndex, iANModalLabel label, float opacity)
{
	assert(rowIndex <= m_layout->columnCount());
	//int labelId = label.id;

	//QLabel *lName = new QLabel(label.name);

	QLabel* lColor = new QLabel();
	setLabelColor(lColor, label.color);

	QSlider* sOpacity = new QSlider(Qt::Orientation::Vertical);
	sOpacity->setMinimum(0);
	sOpacity->setMaximum(SLIDER_MAX);
	setOpacity(sOpacity, opacity);

	m_layout->addWidget(lColor, OPACITY, rowIndex);
	m_layout->addWidget(sOpacity, COLOR, rowIndex);

	if (rowIndex >= m_rows.size())
	{
		m_rows.resize(rowIndex + 1);
	}
	m_rows[rowIndex] = Row(rowIndex, lColor, sOpacity);
	updateRow(rowIndex, label);
}

void iANModalLabelsWidget::updateRow(int rowIndex, iANModalLabel label)
{
	assert(containsLabel(rowIndex));
	Row row = m_rows[rowIndex];
	setLabelColor(row.color, label.color);
	setOpacity(row.opacity, label.opacity);

	auto labelId = label.id;
	row.opacity->disconnect();
	connect(row.opacity, &QSlider::valueChanged, this, [this, labelId]() { emit labelOpacityChanged(labelId); });
}
