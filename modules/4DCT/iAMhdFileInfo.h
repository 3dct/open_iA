// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// Qt
#include <QFile>

class iAMhdFileInfo
{
public:
				iAMhdFileInfo( QString fileName );
	void		getFileDimSize( double* dimSize );
	void		getElementSpacing( double* spacing );

protected:
	void		parseFile( QString fileName );

	QFile		m_file;
	double		m_dimSize[3];
	double		m_elemSpacing[3];
};
