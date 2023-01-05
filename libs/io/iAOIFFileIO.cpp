/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iAOIFFileIO.h"

#include "iAConnector.h"
#include "iAFileUtils.h"   // for getLocalEncodingFileName
#include "iAProgress.h"
#include "iAOIFReader.h"

#include <vtkImageData.h>

#include <QSettings>

const QString iAOIFFileIO::Name("Olympus FluoView");

namespace
{
	const QString SingleChannelStr("Single Channel");
	const QString AllChannelsStr("All Channels");
	const QString WhatToLoadStr("What to load");
	const QString ChannelNumberStr("Channel number");
}

iAOIFFileIO::iAOIFFileIO() : iAFileIO(iADataSetType::Volume, iADataSetType::None)
{
	QStringList whatToLoad = QStringList() << SingleChannelStr << AllChannelsStr;
	selectOption(whatToLoad, AllChannelsStr);
	addAttr(m_params[Load], WhatToLoadStr, iAValueType::Categorical, whatToLoad);
	addAttr(m_params[Load], ChannelNumberStr, iAValueType::Discrete, 0, 0);
}

std::shared_ptr<iADataSet> iAOIFFileIO::loadData(QString const& fileName, QVariantMap const& paramValues, iAProgress const& progress)
{
	Q_UNUSED(progress);
	//void readOIF(QString const& filename, iAConnector * con, int channel, std::vector<vtkSmartPointer<vtkImageData> > *volumes)
	iAOIFReaderHelper reader;
	auto wfn = fileName.toStdWString();
	reader.SetFile(wfn);
	std::wstring timeId(L"_T");
	reader.SetTimeId(timeId);
	reader.Preprocess();
	reader.Load();
	if (paramValues[WhatToLoadStr].toString() == AllChannelsStr)
	{
		auto result = std::make_shared<iADataCollection>(reader.GetChanNum(), std::shared_ptr<QSettings>());
		for (int i = 0; i < reader.GetChanNum(); ++i)
		{
			result->addDataSet(std::make_shared<iAImageData>(reader.GetResult(i)));
		}
		return result;

	}
	else
	{
		assert(paramValues[WhatToLoadStr].toString() == SingleChannelStr);
		auto channel = paramValues[ChannelNumberStr].toInt();
		if (channel >= 0 && channel < reader.GetChanNum())
		{
			return std::make_shared<iAImageData>(reader.GetResult(channel));
		}
		else
		{
			LOG(lvlError, QString("OIF reader: Extracting single channel from file %1 failed: Given channel number %2 is outside of valid range (0..%3)!")
				.arg(fileName).arg(channel).arg(reader.GetChanNum()));
			return {};
		}
	}
}

QString iAOIFFileIO::name() const
{
	return Name;
}

QStringList iAOIFFileIO::extensions() const
{
	return QStringList{ "oif" };
}
