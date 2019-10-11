/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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

#include "open_iA_Core_export.h"

class QString;

/**
* class which (at some point in the future)
* provides simple access to asynchronous,
* pluggable IO operations.
*
* For the moment, collection of helper methods
* around IO.
*/
class open_iA_Core_API iAIOProvider
{
public:
	static QString GetSupportedSaveFormats();
	static QString GetSupportedLoadFormats();
	static QString GetSupportedImageStackFormats();
	static QString GetSupportedVolumeStackFormats();
	static QString GetSupportedImageFormats();


	static const QString ProjectFileTypeFilter;
	static const QString ProjectFileExtension;
	static const QString NewProjectFileTypeFilter;
	static const QString NewProjectFileExtension;
	static const QString MetaImages;
	static const QString VTKFiles; 
};
