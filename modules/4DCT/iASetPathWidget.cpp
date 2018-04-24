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
#include "iASetPathWidget.h"

#include <QFileDialog>
#include <QDir>
#include <QSettings>

iASetPathWidget::iASetPathWidget(QWidget* parent/* = 0*/)
	: QWidget(parent)
{
	ui.setupUi(this);
	connect(ui.Browse, SIGNAL(clicked()), SLOT(onBrowseButtonClicked()));
}

iASetPathWidget::~iASetPathWidget()
{ /* not implemented yet */ }

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