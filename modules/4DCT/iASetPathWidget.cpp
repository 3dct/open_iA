// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iASetPathWidget.h"

#include <QFileDialog>
#include <QDir>
#include <QSettings>

iASetPathWidget::iASetPathWidget(QWidget* parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	connect(ui.Browse, &QPushButton::clicked, this, &iASetPathWidget::onBrowseButtonClicked);
}

iASetPathWidget::~iASetPathWidget()
{}

void iASetPathWidget::setOptions(Mode mode, QString caption, QString filter, QString uniqueKey)
{
	m_mode = mode;
	m_caption = caption;
	m_filter = filter;
	m_uniqueKey = uniqueKey;

	QSettings settings;
	ui.Path->setText(settings.value(m_uniqueKey + m_valPostfix).toString());
}

void iASetPathWidget::onBrowseButtonClicked()
{
	QSettings settings;
	QString path;

	switch (m_mode)
	{
	case Mode::openFile:
		path = QFileDialog::getOpenFileName(this, m_caption, settings.value(m_uniqueKey + m_dirPostfix).toString(), m_filter);
		break;
	case Mode::saveFile:
		path = QFileDialog::getSaveFileName(this, m_caption, settings.value(m_uniqueKey + m_dirPostfix).toString(), m_filter);
		break;
	case Mode::directory:
		path = QFileDialog::getExistingDirectory(this, m_caption, settings.value(m_uniqueKey + m_dirPostfix).toString());
	default:
		break;
	}

	if (!path.isNull())
	{
		QDir dir(path);
		settings.setValue(m_uniqueKey + m_dirPostfix, dir.path());
		settings.setValue(m_uniqueKey + m_valPostfix, path);
	}

	ui.Path->setText(path);
}
