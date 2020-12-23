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
#include "iASampleBuiltInFilterOperation.h"

#include <iAConnector.h>
#include <iALog.h>
#include <iAFilter.h>
#include <iAFilterRegistry.h>
#include <iAProgress.h>
#include <iAITKIO.h>

#include <QFileInfo>

iASampleBuiltInFilterOperation::iASampleBuiltInFilterOperation(
	QString const& filterName,
	bool compressOutput,
	bool overwriteOutput,
	QMap<QString, QVariant> parameters,
	QVector<iAConnector*> input,
	QVector<QString> inputFileNames,
	QString const& outputFileName,
	iALogger* logger) :
	m_filterName(filterName),
	m_compressOutput(compressOutput),
	m_overwriteOutput(overwriteOutput),
	m_parameters(parameters),
	m_input(input),
	m_inputFileNames(inputFileNames),
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
	assert(m_input.size() == m_inputFileNames.size());
	for (int i=0; i<m_input.size(); ++i)
	{
		filter->addInput(m_input[i], m_inputFileNames[i]);
	}
	for (auto param: filter->parameters())
	{
		if (param->valueType() == iAValueType::FileNameSave)
		{	// all output file names need to be adapted to output file name
			auto value = pathFileBaseName(m_outputFileName) + param->defaultValue().toString();
			if (QFile::exists(value) && !m_overwriteOutput)
			{
				LOG(lvlError, QString("Output file '%1' already exists! Aborting. "
					"Check 'Overwrite output' to overwrite existing files.").arg(value));
				return;
			}
			m_parameters[param->name()] = value;
		}
	}
	iAProgress p;	// dummy progress swallowing progress from filter which we don't want to propagate
	filter->setProgress(&p);
	filter->setLogger(m_logger);
	filter->run(m_parameters);
	// adapted from iACommandLineProcessor; maybe this could be merged?
	for (int o = 0; o < filter->output().size(); ++o)
	{
		QString outFileName;
		if (filter->output().size() == 1)
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
		LOG(lvlInfo, QString("Writing output %1 to file: '%2' (compression: %3)")
				.arg(o).arg(outFileName).arg(m_compressOutput ? "on" : "off"))
		iAITKIO::writeFile(outFileName, filter->output()[o]->itkImage(), filter->output()[o]->itkScalarPixelType(), m_compressOutput);
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
