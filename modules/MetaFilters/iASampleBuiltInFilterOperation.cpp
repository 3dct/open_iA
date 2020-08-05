
/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iASampleBuiltInFilterOperation.h"

#include "iASampleParameterNames.h"

#include "iAConnector.h"
#include "iAConsole.h"
#include "iAFilter.h"
#include "iAFilterRegistry.h"
#include "io/iAITKIO.h"

#include <QFileInfo>

iASampleBuiltInFilterOperation::iASampleBuiltInFilterOperation(
	QMap<QString, QVariant> const& parameters,
	QVector<iAConnector*> input,
	QString const & outputFileName):
	m_parameters(parameters),
	m_input(input),
	m_outputFileName(outputFileName),
	m_success(false)
{
	assert(m_parameters[spnAlgorithmType].toString() == atBuiltIn);
}

QString iASampleBuiltInFilterOperation::output() const
{
	return "";
}

void iASampleBuiltInFilterOperation::performWork()
{
	QString filterName = m_parameters[spnFilter].toString();
	bool compress = m_parameters[spnCompressOutput].toBool();
	auto filter = iAFilterRegistry::filter(filterName);
	if (!filter)
	{
		QString msg = QString("Filter '%1' does not exist!").arg(filterName);
		DEBUG_LOG(msg);
		return;
	}
	for (auto in : m_input)
	{
		filter->addInput(in);
	}
	//QObject::connect(&progress, &iAProgress::progress, ... , &::progress);
	//filter->setProgress(&progress);
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
		if (QFile(outFileName).exists())
		{
			// TODO: check at beginning to avoid aborting after long operation? But output count might not be known then...
			DEBUG_LOG(QString("Output file '%1' already exists! Aborting. "
				"Specify -f to overwrite existing files.").arg(outFileName));
			return;
		}
		DEBUG_LOG(QString("Writing output %1 to file: '%2' (compression: %3)")
				.arg(o).arg(outFileName).arg(compress ? "on" : "off"))
		iAITKIO::writeFile(outFileName, filter->output()[o]->itkImage(), filter->output()[o]->itkScalarPixelType(), compress);
	}
	for (auto outputValue : filter->outputValues())
	{
		std::cout << outputValue.first.toStdString() << ": "
			<< outputValue.second.toString().toStdString() << std::endl;
	}
	m_success = true;
}