/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenb�ck, *
*                     Artem & Alexander Amirkhanov, B. Fr�hler                        *
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
* Contact: FH O� Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstra�e 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
#include "iAMovieHelper.h"

#ifdef _WIN32
#include <vtkAVIWriter.h>
#endif
#ifdef VTK_USE_OGGTHEORA_ENCODER
#include <vtkOggTheoraWriter.h>
#endif

#include <QString>


vtkSmartPointer<vtkGenericMovieWriter> GetMovieWriter(QString const & fileName, int quality)
{
	vtkSmartPointer<vtkGenericMovieWriter> movieWriter;
	// Try to create proper video encoder based on given file name.
#ifdef VTK_USE_OGGTHEORA_ENCODER
	if (fileName.endsWith(".ogv")) {
		vtkSmartPointer<vtkOggTheoraWriter> oggwriter;
		oggwriter = vtkSmartPointer<vtkOggTheoraWriter>::New();
		oggwriter->SetQuality(quality);
		oggwriter->SetRate(25);
		movieWriter = oggwriter;
	}
#endif
#ifdef _WIN32
	if (fileName.endsWith(".avi")) {
		vtkSmartPointer<vtkAVIWriter> aviwriter;
		aviwriter = vtkSmartPointer<vtkAVIWriter>::New();
		aviwriter->SetCompressorFourCC("XVID");
		aviwriter->SetRate(25);
		aviwriter->PromptCompressionOptionsOn();
		aviwriter->SetQuality(quality);
		movieWriter = aviwriter;
	}
#endif
	return movieWriter;
}


QString GetAvailableMovieFormats()
{
	QString movie_file_types;
#ifdef VTK_USE_OGGTHEORA_ENCODER
	movie_file_types += "OGG (*.ogv);;";
#endif
#ifdef _WIN32
	movie_file_types += "AVI (*.avi);;";
#endif
	return movie_file_types;
}
