// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iASamplingResults.h"

#include "iAAttributes.h"
#include "iASingleResult.h"

#include <iAAttributeDescriptor.h>
#include <iALog.h>
#include <iAFileUtils.h>
#include <iAStringHelper.h>

#include <QFile>
#include <QTextStream>
#include <QFileInfo>

namespace
{
	const QString SMPFileVersion("v8");
	const QString SMPFileFormatVersion("Sampling File " + SMPFileVersion);
}

iASamplingResults::iASamplingResults(
	QSharedPointer<iAAttributes> attr,
	QString const & samplingMethod,
	QString const & path,
	QString const & executable,
	QString const & additionalArguments,
	QString const & name,
	int id
):
	m_attributes(attr),
	m_name(name),
	m_samplingMethod(samplingMethod),
	m_path(path),
	m_executable(executable),
	m_additionalArguments(additionalArguments),
	m_id(id)
{
}

// TODO: replace with QSettings?
namespace
{

	struct Output
	{
		static const QString NameSeparator;
		static const QString OptionalParamSeparator;
	};


	const QString Output::NameSeparator(": ");
	const QString Output::OptionalParamSeparator(" ");

	bool GetNameValue(QString const & name, QString & value, QTextStream & in)
	{
		QString currentLine = in.readLine();
		QStringList nameValue = currentLine.split(Output::NameSeparator);
		if (nameValue.size() < 2 || nameValue[0] != name || in.atEnd())
		{
			LOG(lvlError, QString("Unexpected name '%1', expected '%2'!\n").arg(nameValue[0]).arg(name));
			return false;
		}
		value = nameValue[1];
		return true;
	}
}

QSharedPointer<iASamplingResults> iASamplingResults::load(QString const & smpFileName, int datasetID)
{
	QFile file(smpFileName);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		LOG(lvlError, QString("Couldn't open file '%1'\n").arg(smpFileName));
		return QSharedPointer<iASamplingResults>();
	}
	// TODO: replace with QSettings?
	QTextStream in(&file);
	in.setEncoding(QStringConverter::Utf8);
	QFileInfo fileInfo(file);
	if (in.atEnd())
	{
		LOG(lvlError, "Invalid sampling descriptor!\n");
		return QSharedPointer<iASamplingResults>();
	}

	QString currentLine = in.readLine();
	if (currentLine != SMPFileFormatVersion)
	{
		LOG(lvlError, QString("Unexpected file format/version identifier '%1', expected was '%2'!\n")
			.arg(currentLine)
			.arg(SMPFileFormatVersion));
	}
	QString name, parameterSetFileName, derivedOutputFileName, samplingMethod;
	if (!GetNameValue("Name", name, in) ||
		!GetNameValue("ParameterSet", parameterSetFileName, in) ||
		!GetNameValue("DerivedOutput", derivedOutputFileName, in) ||
		!GetNameValue("SamplingMethod", samplingMethod, in))
	{
		LOG(lvlError, "Invalid sampling descriptor!");
		return QSharedPointer<iASamplingResults>();
	}
	QString executable, additionalArguments;
	if (!GetNameValue("Executable", executable, in) ||
		!GetNameValue("AdditionalArguments", additionalArguments, in))
	{
		LOG(lvlError, "Executable and/or AdditionalArguments missing in sampling descriptor!");
	}

	QSharedPointer<iAAttributes> attributes = createAttributes(in);
	QSharedPointer<iASamplingResults> result(new iASamplingResults(
		attributes, samplingMethod, fileInfo.absolutePath(), executable, additionalArguments, name, datasetID));
	file.close();
	if (result->loadInternal(MakeAbsolute(fileInfo.absolutePath(), parameterSetFileName),
		MakeAbsolute(fileInfo.absolutePath(), derivedOutputFileName)))
	{
		result->m_rangeFileName = smpFileName;
		return result;
	}
	LOG(lvlError, "Sampling: Internal loading failed.");
	return QSharedPointer<iASamplingResults>();
}


bool iASamplingResults::store(QString const & rangeFileName,
	QString const & parameterSetFileName,
	QString const & derivedOutputFileName)
{
	m_parameterSetFile = parameterSetFileName;
	m_derivedOutputFile = derivedOutputFileName;
	// write parameter ranges:
	QFile paramRangeFile(rangeFileName);
	if (!paramRangeFile.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		LOG(lvlError, QString("Could not open parameter range file '%1' for writing!").arg(rangeFileName));
		return false;
	}
	QTextStream out(&paramRangeFile);
	out.setEncoding(QStringConverter::Utf8);
	QFileInfo fi(paramRangeFile);
	out << SMPFileFormatVersion << Qt::endl;
	out << "Name" << Output::NameSeparator << m_name << Qt::endl;
	out << "ParameterSet" << Output::NameSeparator << MakeRelative(fi.absolutePath(), parameterSetFileName) << Qt::endl;
	out << "DerivedOutput" << Output::NameSeparator << MakeRelative(fi.absolutePath(), derivedOutputFileName) << Qt::endl;
	out << "SamplingMethod" << Output::NameSeparator << m_samplingMethod << Qt::endl;
	out << "Executable" << Output::NameSeparator << m_executable << Qt::endl;
	out << "AdditionalArguments" << Output::NameSeparator << m_additionalArguments << Qt::endl;
	::storeAttributes(out, *m_attributes.data());
	paramRangeFile.close();

	m_rangeFileName = rangeFileName;

	return storeAttributes(iAAttributeDescriptor::Parameter, parameterSetFileName, true) &&
		storeAttributes(iAAttributeDescriptor::DerivedOutput, derivedOutputFileName, false);
}

bool iASamplingResults::storeAttributes(int type, QString const & fileName, bool id)
{
	QFile paramSetFile(fileName);
	if (!paramSetFile.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		LOG(lvlError, QString("Could not open sample parameter set file '%1' for writing!").arg(fileName));
		return false;
	}
	QTextStream outParamSet(&paramSetFile);
	// header:
	if (id)
	{
		outParamSet << "ID" << iASingleResult::ValueSplitString;
	}
	// filter attributes for type:
	iAAttributes typeAttribs;
	std::copy_if(m_attributes->begin(), m_attributes->end(), std::back_inserter(typeAttribs),
		[type](QSharedPointer<iAAttributeDescriptor> att) { return att->attribType() == type; });
	outParamSet << joinAsString((typeAttribs), iASingleResult::ValueSplitString,
		[](QSharedPointer<iAAttributeDescriptor> const& attrib) { return attrib->name(); });
	if (type == iAAttributeDescriptor::Parameter)
	{
		outParamSet << iASingleResult::ValueSplitString << "Filename";
	}
	outParamSet << Qt::endl;
	// values:
	for (int i = 0; i<m_results.size(); ++i)
	{
		if (id)
		{
			outParamSet << m_results[i]->id() << iASingleResult::ValueSplitString;
		}
		outParamSet << m_results[i]->toString(m_attributes, type) << Qt::endl;
	}
	paramSetFile.close();
	return true;
}

bool iASamplingResults::loadInternal(QString const & parameterSetFileName, QString const & derivedOutputFileName)
{
	m_parameterSetFile = parameterSetFileName;
	QFile paramFile(parameterSetFileName);
	if (!paramFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		LOG(lvlError, QString("Could not open sample parameter set file '%1' for reading!").arg(parameterSetFileName));
		return false;
	}
	m_derivedOutputFile = derivedOutputFileName;
	QFile characFile(derivedOutputFileName);
	bool charac = true;
	if (!characFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{

		LOG(lvlError, QString("Could not open sample derived output file '%1' for reading!").arg(derivedOutputFileName));
		charac = false;
	}
	QTextStream paramIn(&paramFile);
	QTextStream* characIn = 0;
	if (charac)
	{
		characIn = new QTextStream(&characFile);
	}
	int lineNr = 0;
	while (!paramIn.atEnd())
	{
		QString paramLine = paramIn.readLine();
		QString attribLine = paramLine;
		if (charac && characIn && !characIn->atEnd())
		{
			QString derivedOutLine = characIn->readLine();
			attribLine = paramLine + iASingleResult::ValueSplitString + derivedOutLine;
		}
		lineNr++;
		QSharedPointer<iASingleResult> result = iASingleResult::create(
			// for now, assemble attributes from two files (could be merged in one)
			attribLine,
			*this,
			m_attributes,
			lineNr != 0);   // for line 0, don't show error output (assume it's the header)
		if (result)
		{
			m_results.push_back(result);		}
		else
		{
			if (lineNr > 0) // skip potential header
			{
				LOG(lvlError, QString("Invalid parameter set / derived output descriptor at line  %1: %2").arg(lineNr).arg(attribLine));
				return false;
			}
		}
	}
	paramFile.close();
	if (charac)
	{
		delete characIn;
		characFile.close();
	}

	return true;
}


int iASamplingResults::size() const
{
	return m_results.size();
}


QSharedPointer<iASingleResult> iASamplingResults::get(int i) const
{
	return m_results[i];
}


void iASamplingResults::addResult(QSharedPointer<iASingleResult> result)
{
	m_results.push_back(result);
}

QVector<QSharedPointer<iASingleResult> > const & iASamplingResults::members() const
{
	return m_results;
}

void iASamplingResults::setMembers(QVector<QSharedPointer<iASingleResult> > const& members)
{
	m_results = members;
}

QSharedPointer<iAAttributes> iASamplingResults::attributes() const
{
	return m_attributes;
}


QString iASamplingResults::name() const
{
	return m_name;
}


QString iASamplingResults::fileName() const
{
	return m_rangeFileName;
}


QString iASamplingResults::path(int id) const
{
	return m_path + "/sample" + QString::number(id);
}

QString iASamplingResults::path() const
{
	return m_path;
}

QString iASamplingResults::executable() const
{
	return m_executable;
}

QString iASamplingResults::additionalArguments() const
{
	return m_additionalArguments;
}

int iASamplingResults::id() const
{
	return m_id;
}
