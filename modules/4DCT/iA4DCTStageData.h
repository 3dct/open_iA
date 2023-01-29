// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// Qt
#include <QList>
// iA
#include "iA4DCTFileData.h"

class iA4DCTStageData
{
public:
							iA4DCTStageData( );
	bool					getFilePath( QString fileName, QString& path );

	int						Force;
	QList<iA4DCTFileData>	Files;
};
