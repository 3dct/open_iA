/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iAFilter.h"

#include "iAAttributeDescriptor.h"
#include "iAConnector.h"
#include "iAConsole.h"
#include "iAStringHelper.h"

#include <QFileInfo>

iAFilter::iAFilter(QString const & name, QString const & category, QString const & description,
	unsigned int requiredInputs, unsigned int outputCount) :
	m_name(name),
	m_category(category),
	m_description(description),
	m_log(iAStdOutLogger::Get()),
	m_requiredInputs(requiredInputs),
	m_outputCount(outputCount),
	m_firstInputChannels(1)
{}

iAFilter::~iAFilter()
{
	ClearOutput();
}

QString iAFilter::Name() const
{
	return m_name;
}

QString iAFilter::Category() const
{
	int slashPos = m_category.indexOf("/");
	return slashPos > 0 ? m_category.left(slashPos) : m_category;
}

QString iAFilter::FullCategory() const
{
	return m_category;
}

QString iAFilter::Description() const
{
	return m_description;
}

QVector<pParameter> const & iAFilter::Parameters() const
{
	return m_parameters;
}

unsigned int iAFilter::RequiredInputs() const
{
	return m_requiredInputs;
}

unsigned int iAFilter::FirstInputChannels() const
{
	return m_firstInputChannels;
}

void iAFilter::SetFirstInputChannels(unsigned int c)
{
	m_firstInputChannels = c;
}

void iAFilter::AddOutputValue(QString const & name, QVariant value)
{
	m_outputValues.push_back(qMakePair(name, value));
}

QVector<QPair<QString, QVariant> > const & iAFilter::OutputValues() const
{
	return m_outputValues;
}

void iAFilter::ClearOutput()
{
	for (iAConnector* con: m_output)
		delete con;
	m_output.clear();
}

void iAFilter::AddOutput(itk::ImageBase<3>* itkImg)
{
	iAConnector * con = new iAConnector();
	con->SetImage(itkImg);
	con->Modified();
	m_output.push_back(con);
}

void iAFilter::AddOutput(vtkSmartPointer<vtkImageData> img)
{
	iAConnector * con = new iAConnector();
	con->SetImage(img);
	con->Modified();
	m_output.push_back(con);
}

QVector<iAConnector*> const & iAFilter::Output()
{
	return m_output;
}

int iAFilter::OutputCount()
{
	return m_outputCount;
}

void iAFilter::ClearInput()
{
	m_input.clear();
}

void iAFilter::AddInput(iAConnector* con)
{
	m_input.push_back(con);
}

QVector<iAConnector*> const & iAFilter::Input()
{
	return m_input;
}

itk::ImageIOBase::IOComponentType iAFilter::InputPixelType() const
{
	return m_input[0]->GetITKScalarPixelType();
}

void iAFilter::SetLogger(iALogger* log)
{
	m_log = log;
}

void iAFilter::SetProgress(iAProgress* progress)
{
	m_progress = progress;
}

bool iAFilter::Run(QMap<QString, QVariant> const & parameters)
{
	if (m_input.size() < m_requiredInputs)
	{
		AddMsg(QString("Not enough inputs specified. Filter %1 requires %2 input images, but only %3 given!").arg(m_name).arg(m_requiredInputs).arg(m_input.size()));
		return false;
	}
	ClearOutput();
	m_outputValues.clear();
	PerformWork(parameters);
	return true;
}

bool iAFilter::CheckParameters(QMap<QString, QVariant> & parameters)
{
	bool ok;
	for (auto param: m_parameters)
	{
		switch (param->ValueType())
		{
		case Discrete: {
			long long value = parameters[param->Name()].toLongLong(&ok);
			if (!ok)
			{
				AddMsg(QString("Parameter %1: Expected integer value, %2 given.").arg(param->Name()).arg(parameters[param->Name()].toString()));
				return false;
			}
			if (value < param->Min() || value > param->Max())
			{
				AddMsg(QString("Parameter %1: Given value %2 outside of valid range [%3..%4].")
					.arg(param->Name())
					.arg(parameters[param->Name()].toString())
					.arg(param->Min()).arg(param->Max()));
				return false;
			}
			break;
		}
		case Continuous:
		{
			double value = parameters[param->Name()].toDouble(&ok);
			if (!ok)
			{
				AddMsg(QString("Parameter %1: Expected double value, %2 given.").arg(param->Name()).arg(parameters[param->Name()].toString()));
				return false;
			}
			if (value < param->Min() || value > param->Max())
			{
				AddMsg(QString("Parameter %1: Given value %2 outside of valid range [%3..%4].")
					.arg(param->Name())
					.arg(parameters[param->Name()].toString())
					.arg(param->Min()).arg(param->Max()));
				return false;
			}
			break;
		}
		case Categorical:
		{
			QStringList values = param->DefaultValue().toStringList();
			if (!values.contains(parameters[param->Name()].toString()))
			{
				AddMsg(QString("Parameter %1: Given value '%2' not in the list of valid values (%3).")
					.arg(param->Name())
					.arg(parameters[param->Name()].toString())
					.arg(values.join(",")));
				return false;
			}
			break;
		}
		case FileNameOpen:
		{
			QFileInfo file(parameters[param->Name()].toString());
			if (!file.isFile() || !file.isReadable())
			{
				AddMsg(QString("Parameter %1: Given filename '%2' either doesn't reference a file, "
					"the file does not exist, or it is not readable!").arg(param->Name()).arg(parameters[param->Name()].toString()));
				return false;
			}
			break;
		}
		case FileNamesOpen:
		{
			QStringList files = SplitPossiblyQuotedString(parameters[param->Name()].toString());
			for (auto fileName : files)
			{
				QFileInfo file(fileName);
				if (!file.isFile() || !file.isReadable())
				{
					AddMsg(QString("Parameter %1: Filename '%2' out of the given list '%3' either doesn't reference a file, "
						"the file does not exist, or it is not readable!").arg(param->Name())
						.arg(fileName)
						.arg(parameters[param->Name()].toString()));
					return false;
				}
			}
			break;
		}
		case Folder:
		{
			// TODO: allow to specify whether the folder can be empty or not!
			QFileInfo file(parameters[param->Name()].toString());
			if (!parameters[param->Name()].toString().isEmpty() && !file.isDir())
			{
				AddMsg(QString("Parameter '%1': Given value '%2' doesn't reference a folder!")
					.arg(param->Name()).arg(parameters[param->Name()].toString()));
				return false;
			}
			break;
		}
		case Invalid:
			AddMsg(QString("Parameter '%1': Invalid parameter type (please contact developers!)!").arg(param->Name()));
			return false;
		default:  // no checks
			break;
		}
	}
	return true;
}

void iAFilter::AddMsg(QString msg)
{
	m_log->Log(msg);
}

iAProgress* iAFilter::Progress()
{
	return m_progress;
}

iALogger* iAFilter::Logger()
{
	return m_log;
}

void iAFilter::AddParameter(QString const & name, iAValueType valueType,
	QVariant defaultValue, double min, double max)
{
	m_parameters.push_back(iAAttributeDescriptor::CreateParam(name, valueType, defaultValue, min, max));
}

QVector<QString> const & iAFilter::OutputValueNames() const
{
	return m_outputValueNames;
}

void iAFilter::AddOutputValue(QString const & name)
{
	m_outputValueNames.push_back(name);
}
