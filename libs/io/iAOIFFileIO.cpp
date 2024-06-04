// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAOIFFileIO.h"

#include "iAConnector.h"
#include "iAImageData.h"
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
			auto ds = std::make_shared<iAImageData>(reader.GetResult(channel));
			ds->setMetaData(paramValues);
			return ds;
		}
		else
		{
			throw std::runtime_error(QString("OIF reader: Extracting single channel failed: Given channel number %1 is outside of valid range (0..%2)!")
				.arg(channel).arg(reader.GetChanNum()).toStdString());
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
