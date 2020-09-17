/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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

#include <QToolBar>

template <typename QtContainerType, typename uiType>
class iAQTtoUIConnector : public QtContainerType, public uiType
{
public:
#if QT_VERSION < QT_VERSION_CHECK(5,15,0)
	iAQTtoUIConnector( QWidget * parent = nullptr, Qt::WindowFlags f = 0 ) : QtContainerType( parent, f )
#else
	iAQTtoUIConnector(QWidget* parent = nullptr, Qt::WindowFlags f = QFlags<Qt::WindowType>()) : QtContainerType(parent, f)
#endif
	{
		this->setupUi(this);
	}
};

template <typename uiType>
class iAQTtoUIConnector<QToolBar, uiType> : public QToolBar, public uiType
{
public:
	iAQTtoUIConnector(QString const & title, QWidget * parent = nullptr) : QToolBar(title, parent)
	{
		this->setupUi(this);
	}
};
