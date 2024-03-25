// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

class iAMhdFileInfo
{
public:
				iAMhdFileInfo( QString fileName );
	void		getFileDimSize( double* dimSize );
	void		getElementSpacing( double* spacing );

protected:
	void		parseFile( QString fileName );

	double		m_dimSize[3];
	double		m_elemSpacing[3];
};
