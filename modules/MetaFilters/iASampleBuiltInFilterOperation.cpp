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
#include "iASampleBuiltInFilterOperation.h"

#include <iADataSet.h>
#include <iAFileTypeRegistry.h>
#include <iAFileUtils.h>    // for pathFileBaseName
#include <iAFilter.h>
#include <iAFilterRegistry.h>
#include <iALog.h>
#include <iAProgress.h>

#include <QFileInfo>

iASampleBuiltInFilterOperation::iASampleBuiltInFilterOperation(
	QString const& filterName,
	bool compressOutput,
	bool overwriteOutput,
	QVariantMap parameters,
	std::vector<std::shared_ptr<iADataSet>> input,
	QString const& outputFileName,
	iALogger* logger) :
	m_filterName(filterName),
	m_compressOutput(compressOutput),
	m_overwriteOutput(overwriteOutput),
	m_parameters(parameters),
	m_input(input),
	m_outputFileName(outputFileName),
	m_logger(logger)
{
}

QString iASampleBuiltInFilterOperation::output() const
{
	return "";
}

void iASampleBuiltInFilterOperation::performWork()
{
	auto filter = iAFilterRegistry::filter(m_filterName);
	if (!filter)
	{
		QString msg = QString("Filter '%1' does not exist!").arg(m_filterName);
		LOG(lvlError, msg);
		return;
	}
	for (int i=0; i<m_input.size(); ++i)
	{
		filter->addInput(m_input[i]);
	}
	filter->setLogger(m_logger);
	filter->run(m_parameters);
	// adapted from iACommandLineProcessor; maybe this could be merged?
	for (size_t o = 0; o < filter->finalOutputCount(); ++o)
	{
		QString outFileName;
		if (filter->finalOutputCount() == 1)
		{
			outFileName = m_outputFileName;
		}
		else
		{
			QFileInfo fi(m_outputFileName);
			outFileName = QString("%1/%2%3.%4").arg(fi.absolutePath()).arg(fi.baseName())
				.arg(o + 1).arg(fi.completeSuffix());
		}
		if (QFile(outFileName).exists() && !m_overwriteOutput)
		{
			// TODO: check at beginning to avoid aborting after long operation? But output count might not be known then...
			LOG(lvlError, QString("Output file '%1' already exists! Aborting. "
				"Check 'Overwrite output' to overwrite existing files.").arg(outFileName));
			return;
		}
		LOG(lvlInfo,
			QString("Writing output %1 to file: '%2' (compression: %3)")
				.arg(o)
				.arg(outFileName)
				.arg(m_compressOutput ? "on" : "off"));

		auto io = iAFileTypeRegistry::createIO(outFileName, iAFileIO::Save);
		QVariantMap writeParamValues;    // TODO: CHECK whether I/O requires other parameters and error in that case!
		writeParamValues[iAFileIO::CompressionStr] = m_compressOutput;
		io->save(outFileName, filter->output(o), writeParamValues);
	}
	/*
	// required options:
	//   - combined CSV / one CSV per sample
	//   - base CSV name
	//  (- append?)
	QString outputFile = m_parameters[spnOutputCSV].toString();
	if (!outputFile.isEmpty())
	{
		QFile file(outputFile);
		if (file.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			QTextStream textStream(&file);
		for (auto outputValue : filter->outputValues())
		{
			<< outputValue.first.toStdString() << ": "
				<< outputValue.second.toString().toStdString() << std::endl;
		}
	}
	*/
	setSuccess(true);
}
