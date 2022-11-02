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
#include "iAVolStackFileIO.h"

#include "iAFileTypeRegistry.h"
#include "iAFileUtils.h"
#include "iAMultiStepProgress.h"
#include "iAProgress.h"
#include "iASettings.h"    // for mapFromQSettings

#include <QFileInfo>
#include <QSettings>

namespace
{
	static const QString ProjectFileVersionKey("FileVersion");
	static const QString ProjectFileVersionValue("1.0");

	QString dataSetGroup(int idx)
	{   // for backwardss compatibility, let's keep "Modality" as identifier for now
		return QString("Modality") + QString::number(idx);
	}

	QMap<QString, QString> readSettingsFile(QString const& fileName, QString const & keyValueSeparator = ":")
	{
		QFile file(fileName);
		if (!file.open(QIODevice::ReadOnly))
		{
			throw std::runtime_error(QString("Could not open file %1").arg(fileName).toStdString());
		}
		QTextStream textStream(&file);
		QString currentSection;
		QMap<QString, QString> result;
		int lineNr = 0;
		while (!textStream.atEnd())
		{
			++lineNr;
			QString currentLine = textStream.readLine();
			if (currentLine.trimmed().isEmpty())
			{
				continue;
			}
			if (currentLine.indexOf("[") == 0 && currentLine.indexOf("]") > 0)
			{
				currentSection = currentLine.mid(1, currentLine.indexOf("]")-2);
			}
			else
			{
				auto keyValSepPos = currentLine.indexOf(keyValueSeparator);
				if (keyValSepPos == -1)
				{
					throw std::runtime_error(QString("Key/Value line (#%1) without separator: %2").arg(lineNr).arg(currentLine).toStdString());
				}
				auto key = (currentSection.isEmpty() ? "" : currentSection + "/") + currentLine.left(keyValSepPos-1).trimmed();
				result[key] = currentLine.right(keyValSepPos + 1);
			}
		}
		// file closed automatically by ~QFile
		return result;
	}
}

const QString iAVolStackFileIO::Name("Volume Stack descriptor");

iAVolStackFileIO::iAVolStackFileIO() : iAFileIO(iADataSetType::All, iADataSetType::None) // writing to a project file is specific (since it doesn't write the dataset itself...)
{}

std::vector<std::shared_ptr<iADataSet>> iAVolStackFileIO::loadData(QString const& fileName, QVariantMap const& paramValues, iAProgress const& progress)
{
	Q_UNUSED(paramValues);

	QFileInfo fi(fileName);
	auto volStackSettings = readSettingsFile(fileName);
	auto fileNameBase = fi.absolutePath() + "/" + volStackSettings["file_names_base"];
	auto extension = volStackSettings["extension"];
	bool ok;
	int digitsInIndex = volStackSettings["number_of_digits_in_index"].toInt(&ok);
	if (!ok || digitsInIndex < 0)
	{
		throw std::runtime_error(QString("VolStack I/O: Invalid value (%1) for number_of_digits_in_index - not a number or smaller 0!").arg(volStackSettings["number_of_digits_in_index"]).toStdString());
	}
	bool ok1, ok2;
	int minIdx = volStackSettings["minimum_index"].toInt(&ok1);
	int maxIdx = volStackSettings["maximum_index"].toInt(&ok2);
	if (!ok1 || !ok2 || minIdx >= maxIdx)
	{
		throw std::runtime_error(QString("VolStack I/O: Invalid index range %1 - %2 (invalid numbers or min >= max).")
			.arg(volStackSettings["minimum_index"]).arg(volStackSettings["maximum_index"]).toStdString());
	}
	// TODO: make single dataset and append additional entries to dataset!
	// "elementNames" "energy_range" ...
	
	std::vector<std::shared_ptr<iADataSet>> result;
	// use iAMultiStepProgress?
	for (int i = minIdx; i <= maxIdx; ++i)
	{
		QString curFileName = fileNameBase + QString("%1").arg(i, digitsInIndex, 10, QChar('0')) + extension;
		auto io = iAFileTypeRegistry::createIO(curFileName);
		if (!io)
		{
			throw std::runtime_error(QString("VolStack I/O: Cannot read file (%1) - no suitable reader found!").arg(curFileName).toStdString());
		}
		if (io->parameter(iAFileIO::Load).size() != 1 || io->parameter(iAFileIO::Load)[0]->name() != iADataSet::FileNameKey)
		{
			throw std::runtime_error(QString("VolStack I/O: Cannot read file (%1) - reader requires other parameters !").arg(curFileName).toStdString());
		}
		iAProgress dummyProgress;
		QVariantMap curParamValues;
		auto dataSet = io->load(curFileName, curParamValues, dummyProgress);
		if (dataSet.size() != 1)
		{
			throw std::runtime_error(QString("VolStack I/O: Sub-reader unexpectedly returned more or less than 1 dataset (%1)").arg(dataSet.size()).toStdString());
		}
		result.push_back(dataSet[0]);
		progress.emitProgress(100 * (i - minIdx) / (maxIdx - minIdx + 1));
	}
	return result;
}

void iAVolStackFileIO::saveData(QString const& fileName, std::vector<std::shared_ptr<iADataSet>>& dataSets, QVariantMap const& paramValues, iAProgress const& progress)
{
	// write .volstack file:
	QFile volstackFile(fileName);
	QFileInfo fi(fileName);
	size_t numOfVolumes = dataSets.size();
	int numOfDigits = static_cast<int>(std::log10(static_cast<double>(dataSets.size())) + 1);
	const QString Extension = ".mhd";
	auto fileNameBase = fi.completeBaseName();
	if (volstackFile.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		QTextStream out(&volstackFile);
		out << "file_names_base: " << fileNameBase << "\n"
			<< "extension: " << Extension << "\n"
			<< "number_of_digits_in_index: " << numOfDigits << "\n"
			<< "minimum_index: 0" << "\n"
			<< "maximum_index: " << numOfVolumes - 1 << "\n";
		//if (!m_additionalInfo.isEmpty())
		//{
		//	out << m_additionalInfo << "\n";
		//}
	}
	//// write mhd images:
	for (int m = 0; m <= dataSets.size(); m++)
	{
		QString curFileName = fileNameBase + QString("%1").arg(m, numOfDigits, 10, QChar('0')) + Extension;
		auto io = iAFileTypeRegistry::createIO(curFileName);
		std::vector<std::shared_ptr<iADataSet>> ds;
		ds.push_back(dataSets[m]);
		iAProgress dummyProgress;
		QVariantMap curParamValues;
		io->save(curFileName, ds, curParamValues, dummyProgress);
		progress.emitProgress(m * 100.0 / dataSets.size());
	}
}

QString iAVolStackFileIO::name() const
{
	return Name;
}

QStringList iAVolStackFileIO::extensions() const
{
	return QStringList{ "volstack" };
}
