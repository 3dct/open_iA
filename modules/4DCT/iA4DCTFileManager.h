// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iA4DCTFileData.h"
// std
#include <map>
// vtk
#include <vtkSmartPointer.h>

class vtkAlgorithmOutput;
class vtkMetaImageReader;
class vtkImageData;

class iA4DCTFileManager
{
private:
	typedef vtkSmartPointer<vtkMetaImageReader> ReaderType;

public:
	static iA4DCTFileManager&	getInstance( );
	vtkImageData*				getImage( iA4DCTFileData file );
	vtkAlgorithmOutput*			getOutputPort( iA4DCTFileData file );

private:
			iA4DCTFileManager( ) { }
			~iA4DCTFileManager( ) { }

			iA4DCTFileManager( iA4DCTFileManager const& ) = delete;
	void	operator=( iA4DCTFileManager const& ) = delete;

	ReaderType	findOrCreateImage( iA4DCTFileData file );
	ReaderType	findReader( iA4DCTFileData file );

	std::map<std::string, ReaderType> m_map;
};
