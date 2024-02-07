// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// Qt
#include <QVector>

class iAVisModule;

class iAVisModuleItem
{
public:
					iAVisModuleItem( iAVisModule * module, QString name, int id = 0 );

	iAVisModule *	module;
	QString			name;
	QVector<int>	stages;
	int				id;
};
