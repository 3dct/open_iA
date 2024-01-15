// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// Qt
#include <QHash>
#include <QVector>

namespace iA4DCTDefects
{
	typedef unsigned long			TType;
	typedef QVector<TType>			VectorDataType;
	typedef QHash<TType, bool>		HashDataType;

	bool					save( VectorDataType defects, QString path );
	VectorDataType			load( QString path );
	HashDataType			DefectDataToHash( VectorDataType defects );
};
