// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QToolBar>

//! Helper for using .ui file
template <typename QtContainerType, typename uiType>
class iAQTtoUIConnector : public QtContainerType, public uiType
{
public:
	iAQTtoUIConnector(QWidget* parent = nullptr) : QtContainerType( parent)
	{
		this->setupUi(this);
	}
};

//! Helper for using a .ui file, specifically for modelling a QToolBar.
template <typename uiType>
class iAQTtoUIConnector<QToolBar, uiType> : public QToolBar, public uiType
{
public:
	iAQTtoUIConnector(QString const & title, QWidget* parent = nullptr) : QToolBar(title, parent)
	{
		this->setupUi(this);
	}
};
