/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
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
