// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// Qt
#include <QString>

class iA4DCTMainWin;

class iA4DCTProjectReaderWriter
{
public:
	static void		save( iA4DCTMainWin * mainWin, QString path );
	static bool		load( iA4DCTMainWin * mainWin, QString path );
};
