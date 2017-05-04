/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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
#include "pch.h"
#include "iAOIFReader.h"

#include "iAConnector.h"
#include "iAConsole.h"

#include "oif_reader.h"

#include <vtkImageData.h>

namespace IO
{
namespace OIF
{

Reader::Reader()
{}

void Reader::read(QString const & filename, iAConnector* con, int channel,
	std::vector<vtkSmartPointer<vtkImageData> > * volumes)
{
	OIFReader* reader = new OIFReader();
	std::string fnStr(filename.toStdString());
	reader->SetFile(fnStr);
	reader->SetSliceSeq(false);
	std::wstring timeId(L"_T");
	reader->SetTimeId(timeId);
	reader->Preprocess();
	reader->Load();
	if (volumes)
	{
		con->SetImage(reader->GetResult(0));
		con->Modified();
		for (int i = 0; i < reader->GetChanNum(); ++i)
		{
			vtkSmartPointer<vtkImageData> image = vtkSmartPointer<vtkImageData>::New();
			iAConnector con;
			con.SetImage(reader->GetResult(i));
			image->DeepCopy(con.GetVTKImage());
			volumes->push_back(image);
		}
	}
	else if (channel >= 0 && channel < reader->GetChanNum())
	{
		con->SetImage(reader->GetResult(channel));
		con->Modified();
	}
	else
	{
		DEBUG_LOG("OIF reader: Neither channel number nor volume vector given!");
	}
}

}
}
