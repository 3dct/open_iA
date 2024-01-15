// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAMovieHelper.h"

// base
#include "iALog.h"
#include "iAFileUtils.h"

#include "iAAbortListener.h"

// with Visual Studio 2022 preview, using AVIWriter leads to unresolved externals...
// and _MSC_VER < 1930
// ... but the same problem also appeared on another VS 2019 PC.
// Also there seems to exist no way in CMake to determine whether vtkAVIWriter is available.
// So, disabling vtkAVIWriter for now, except if explicitly enabled via VTK_USE_AVIWRITER CMake option
#if defined(_WIN32) and defined(_MSC_VER)  and defined(VTK_USE_AVIWRITER)
#include <vtkAVIWriter.h>
#endif
#ifdef VTK_USE_OGGTHEORA_ENCODER
#include <vtkOggTheoraWriter.h>
#endif
#include <vtkGenericMovieWriter.h>

#include <QString>


vtkSmartPointer<vtkGenericMovieWriter> GetMovieWriter(QString const & fileName, int quality)
{
	std::string encodedFileName = getLocalEncodingFileName(fileName);
	if (encodedFileName.empty())
		return vtkSmartPointer<vtkGenericMovieWriter>();
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
#if defined(_WIN32) and defined(_MSC_VER) and defined(VTK_USE_AVIWRITER)
	if (fileName.endsWith(".avi")) {
		vtkSmartPointer<vtkAVIWriter> aviwriter;
		aviwriter = vtkSmartPointer<vtkAVIWriter>::New();
		aviwriter->SetCompressorFourCC("XVID");
		aviwriter->SetRate(25);
		//aviwriter->PromptCompressionOptionsOn();
		aviwriter->SetQuality(quality);
		movieWriter = aviwriter;
	}
#endif
	movieWriter->SetFileName(encodedFileName.c_str());
	return movieWriter;
}


QString GetAvailableMovieFormats()
{
	QString movie_file_types;
#ifdef VTK_USE_OGGTHEORA_ENCODER
	movie_file_types += "OGG (*.ogv);;";
#endif
#if defined(_WIN32) and defined(_MSC_VER) and defined(VTK_USE_AVIWRITER)
	movie_file_types += "AVI (*.avi);;";
#endif
	return movie_file_types;
}

void printFinalLogMessage(vtkGenericMovieWriter * movieWriter, iASimpleAbortListener const & aborter)
{
	if (movieWriter->GetError())
	{
		LOG(lvlError, "Movie export failed.");
	}
	else if (aborter.isAborted())
	{
		LOG(lvlWarn, "Movie export aborted; half-finished file needs to be deleted manually!");
	}
	else
	{
		LOG(lvlInfo, "Movie export completed.");
	}
}
