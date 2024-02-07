// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAValueType.h"

#include <QApplication>
#include <QColorDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QToolButton>
#include <QWidget>

//! Input for a color value, combining a text box, a color indicator and a chooser button.
//! Currently only used in iAParameterDialog. If used somewhere else, method
//! definitions should be extracted to separate .cpp file to avoid according errors
class iAColorInput : public QWidget
{
	Q_OBJECT
public:
	iAColorInput(QWidget* parent, QVariant const& value) :
		QWidget(parent),
		m_lineEdit(new QLineEdit(this)),
		m_colorIndicator(new QLabel(this)),
		m_colorDialogButton(new QToolButton(this))
	{
		setLayout(new QHBoxLayout);
		setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
		layout()->setContentsMargins(0, 0, 0, 0);
		layout()->setSpacing(4);
		layout()->addWidget(m_lineEdit);
		m_colorIndicator->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
		m_colorIndicator->setAutoFillBackground(true);
		layout()->addWidget(m_colorIndicator);
		layout()->addWidget(m_colorDialogButton);
		m_colorDialogButton->setText("...");
		connect(m_colorDialogButton, &QToolButton::clicked, this,
			[this]() {
				QColorDialog dlg;
				QColor color(m_lineEdit->text());
				dlg.setCurrentColor(color);
				if (dlg.exec() != QDialog::Accepted || !dlg.selectedColor().isValid())
				{
					return;
				}
				QColor col = dlg.selectedColor();
				m_lineEdit->setText(col.name());
				updateIndicatorColor(col);
			});
		QColor col = variantToColor(value);
		if (col.isValid())
		{
			m_lineEdit->setText(col.name());
			updateIndicatorColor(col);
		}
		connect(m_lineEdit, &QLineEdit::textChanged, this,
			[this]() {
				updateIndicatorColor(QColor(m_lineEdit->text()));
			});
	}
	void updateIndicatorColor(QColor const & color)
	{
		QPalette pal;
		pal.setColor(QPalette::Window, color.isValid() ? color : QApplication::palette().color(QPalette::Window));
		m_colorIndicator->setPalette(pal);
	}
	QVariant value() const
	{
		return m_lineEdit->text();
	}
	void setValue(QVariant const& value)
	{
		QColor col = variantToColor(value);
		QSignalBlocker block(m_lineEdit);
		m_lineEdit->setText(col.name());
		updateIndicatorColor(col);
	}
private:
	QLineEdit* m_lineEdit;
	QLabel* m_colorIndicator;
	QToolButton* m_colorDialogButton;
};
