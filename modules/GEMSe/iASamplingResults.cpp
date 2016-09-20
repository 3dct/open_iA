/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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

#include "iASamplingResults.h"

#include "iAAttributes.h"
#include "iAAttributeDescriptor.h"
#include "iAFileUtils.h"
#include "iAGEMSeConstants.h"
#include "iASingleResult.h"

#include "iAConsole.h"

#include <QFile>
#include <QTextStream>
#include <QFileInfo>


iASamplingResults::iASamplingResults(
	QSharedPointer<iAAttributes> attr,
	QString const & samplingMethod,
	int id
):
	m_attributes(attr),
	m_samplingMethod(samplingMethod),
	m_id(id)
{
	NewID = (id >= NewID)? id + 1: NewID;
}

// TODO: replace with QSettings?
namespace
{
	bool GetNameValue(QString const & name, QString & value, QTextStream & in)
	{
		QString currentLine = in.readLine();
		QStringList nameValue = currentLine.split(Output::NameSeparator);
		if (nameValue.size() < 2 || nameValue[0] != name || in.atEnd())
		{
			DEBUG_LOG(QString("Unexpected name '%1', expected '%2'!\n").arg(nameValue[0]).arg(name));
			return false;
		}
		value = nameValue[1];
		return true;
	}
}

QSharedPointer<iASamplingResults> iASamplingResults::Load(QString const & smpFileName, int datasetID)
{
	QFile file(smpFileName);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		DEBUG_LOG(QString("Couldn't open file '%1'\n").arg(smpFileName));
		return QSharedPointer<iASamplingResults>();
	}
	// TODO: replace with QSettings?
	QTextStream in(&file);
	QFileInfo fileInfo(file);
	if (in.atEnd())
	{
		DEBUG_LOG("Invalid Sampling descriptor!\n");
		return QSharedPointer<iASamplingResults>();
	}

	QString currentLine = in.readLine();
	if (currentLine != SMPFileFormatVersion)
	{
		DEBUG_LOG(QString("Unexpected file format/version identifier '%1', expected was '%2'!\n")
			.arg(currentLine)
			.arg(SMPFileFormatVersion));
	}
	QString parameterSetFileName, characteristicsFileName, samplingMethod;
	if (!GetNameValue("ParameterSet", parameterSetFileName, in) ||
		!GetNameValue("DerivedOutput", characteristicsFileName, in) ||
		!GetNameValue("SamplingMethod", samplingMethod, in))
	{
		DEBUG_LOG("Invalid Sampling descriptor!");
		return QSharedPointer<iASamplingResults>();
	}

	QSharedPointer<iAAttributes> attributes = iAAttributes::Create(in);
	QSharedPointer<iASamplingResults> result(new iASamplingResults(attributes, samplingMethod, datasetID));
	file.close();
	if (result->LoadInternal(MakeAbsolute(fileInfo.absolutePath(), parameterSetFileName),
		MakeAbsolute(fileInfo.absolutePath(), characteristicsFileName)))
	{
		result->m_fileName = smpFileName;
		return result;
	}
	DEBUG_LOG("Sampling: Internal loading failed.");
	return QSharedPointer<iASamplingResults>();
}


bool iASamplingResults::Store(QString const & fileName,
	QString const & parameterSetFileName,
	QString const & characteristicsFileName)
{
	// write parameter ranges:
	QFile paramRangeFile(fileName);
	if (!paramRangeFile.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		DEBUG_LOG(QString("Could not open parameter range file '%1' for writing!").arg(fileName));
		return false;
	}
	QTextStream out(&paramRangeFile);
	QFileInfo fi(paramRangeFile);
	out << SMPFileFormatVersion << endl;
	out << "ParameterSet" << Output::NameSeparator << MakeRelative(fi.absolutePath(), parameterSetFileName) << endl;
	out << "DerivedOutput" << Output::NameSeparator << MakeRelative(fi.absolutePath(), characteristicsFileName) << endl;
	out << "SamplingMethod" << Output::NameSeparator << m_samplingMethod << endl;
	m_attributes->Store(out);
	paramRangeFile.close();

	m_fileName = fileName;
	
	return StoreAttributes(iAAttributeDescriptor::Parameter, parameterSetFileName, true) &&
		StoreAttributes(iAAttributeDescriptor::DerivedOutput, characteristicsFileName, false);
	

	return true;
}

bool iASamplingResults::StoreAttributes(int type, QString const & fileName, bool id)
{
	QFile paramSetFile(fileName);
	if (!paramSetFile.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		DEBUG_LOG(QString("Could not open sample parameter set file '%1' for writing!").arg(fileName));
		return false;
	}
	QTextStream outParamSet(&paramSetFile);
	for (int i = 0; i<m_results.size(); ++i)
	{
		if (id)
		{
			outParamSet << m_results[i]->GetID() << ValueSplitString;
		}
		outParamSet << m_results[i]->ToString(m_attributes, type) << endl;
	}
	paramSetFile.close();
	return true;
}

bool iASamplingResults::LoadInternal(QString const & parameterSetFileName, QString const & characteristicsFileName)
{
	QFile paramFile(parameterSetFileName);
	if (!paramFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		DEBUG_LOG(QString("Could not open sample parameter set file '%1' for reading!").arg(parameterSetFileName));
		return false;
	}
	QFile characFile(characteristicsFileName);
	bool charac = true;
	if (!characFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{

		DEBUG_LOG(QString("Could not open sample characteristics file '%1' for reading!").arg(characteristicsFileName));
		charac = false;
	}
	QTextStream paramIn(&paramFile);
	QTextStream* characIn = 0;
	if (charac)
	{
		characIn = new QTextStream(&characFile);
	}
	(&characFile);
	QFileInfo paramFileInfo(paramFile);
	int lineNr = 0;
	m_path = paramFileInfo.absolutePath();
	while (!paramIn.atEnd())
	{
		QString paramLine = paramIn.readLine();
		QString attribLine = paramLine;
		if (charac && characIn && !characIn->atEnd())
		{
			QString derivedOutLine = characIn->readLine();
			attribLine = paramLine + ValueSplitString + derivedOutLine;
		}
		lineNr++;
		QSharedPointer<iASingleResult> result = iASingleResult::Create(
			// for now, assemble attributes from two files (could be merged in one)
			attribLine,
			*this,
			m_attributes);
		if (!result)
		{
			DEBUG_LOG(QString("Invalid parameter set / derived output descriptor at line  %1: %2").arg(lineNr).arg(attribLine));
			return false;
		}
		m_results.push_back(result);
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


QSharedPointer<iASingleResult> iASamplingResults::Get(int i) const
{
	return m_results[i];
}


void iASamplingResults::AddResult(QSharedPointer<iASingleResult> result)
{
	m_results.push_back(result);
}

QVector<QSharedPointer<iASingleResult> > const & iASamplingResults::GetResults() const
{
	return m_results;
}

QSharedPointer<iAAttributes> iASamplingResults::GetAttributes() const
{
	return m_attributes;
}


QString iASamplingResults::GetFileName() const
{
	return m_fileName;
}


QString iASamplingResults::GetPath(int id) const
{
	return m_path + "/sample" + QString::number(id);
}

int iASamplingResults::GetID() const
{
	return m_id;
}

int iASamplingResults::NewID = 0;

int iASamplingResults::GetNewID()
{
	return NewID;
}