// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iA4DCTStageData.h"

iA4DCTStageData::iA4DCTStageData( )
	: Force( -1 )
{ /* not yet implemented */ }

/*============

	getFilePath

============*/
// get path of a file using name
// the function returns true if the file found. Otherwise it returns false
bool iA4DCTStageData::getFilePath( QString fileName, QString& path )
{
	for( auto f : Files ) {
		if( fileName == f.Name ) {
			path = f.Path;
			return true;	// file found
		}
	}
	return false;			// file not found
}
