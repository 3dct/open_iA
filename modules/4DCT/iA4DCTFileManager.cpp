// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iA4DCTFileManager.h"

#include "iAFileUtils.h"

#include <vtkMetaImageReader.h>

iA4DCTFileManager& iA4DCTFileManager::getInstance( )
{
	static iA4DCTFileManager instance;
	return instance;
}

vtkImageData* iA4DCTFileManager::getImage( iA4DCTFileData file )
{
	return findOrCreateImage( file )->GetOutput( );
}

vtkAlgorithmOutput* iA4DCTFileManager::getOutputPort( iA4DCTFileData file )
{
	return findOrCreateImage( file )->GetOutputPort( );
}

iA4DCTFileManager::ReaderType iA4DCTFileManager::findOrCreateImage( iA4DCTFileData file )
{
	std::string key = file.Name.toStdString( );
	if( m_map.find( key ) == m_map.end( ) )
	{	// the key does not exist
		ReaderType reader = ReaderType::New( );
		reader->SetFileName( getLocalEncodingFileName(file.Path).c_str( ) );
		reader->Update( );
		m_map[ key ] = reader;
	}
	return m_map[ key ];
}
