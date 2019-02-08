/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#include "ui_iASetPathWidget.h"
#include <QWidget>

class iASetPathWidget : public QWidget
{
	Q_OBJECT
public:
	enum Mode { openFile, saveFile, directory };

				iASetPathWidget(QWidget* parent = 0);
				~iASetPathWidget();
	void		setOptions(Mode mode, QString caption, QString filter, QString uniqueKey);

	Ui::SetPathWidget	ui;

protected:
	Mode		m_mode;
	QString		m_caption;
	QString		m_filter;
	QString		m_uniqueKey;
	const QString m_dirPostfix = QString("_dir");
	const QString m_valPostfix = QString("_val");

private slots:
	void		onBrowseButtonClicked();
};
