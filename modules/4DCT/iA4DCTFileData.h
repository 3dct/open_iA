// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// Qt
#include <QString>

struct iA4DCTFileData
{
	iA4DCTFileData( QString path = 0, QString name = 0 ) :
		Path( path ),
		Name( name )
	{ /* not implemented */	}
	QString Path;
	QString Name;
};
