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
	m_log(iAStdOutLogger::get()),
	m_requiredInputs(requiredInputs),
	m_outputCount(outputCount),
	m_firstInputChannels(1)
{}

iAFilter::~iAFilter()
{
	clearOutput();
}

QString iAFilter::name() const
{
	return m_name;
}

QString iAFilter::category() const
{
	int slashPos = m_category.indexOf("/");
	return slashPos > 0 ? m_category.left(slashPos) : m_category;
}

QString iAFilter::fullCategory() const
{
	return m_category;
}

QString iAFilter::description() const
{
	return m_description;
}

QVector<pParameter> const & iAFilter::parameters() const
{
	return m_parameters;
}

unsigned int iAFilter::requiredInputs() const
{
	return m_requiredInputs;
}

unsigned int iAFilter::firstInputChannels() const
{
	return m_firstInputChannels;
}

void iAFilter::setFirstInputChannels(unsigned int c)
{
	m_firstInputChannels = c;
}

void iAFilter::addOutputValue(QString const & name, QVariant value)
{
	m_outputValues.push_back(qMakePair(name, value));
}

QVector<QPair<QString, QVariant> > const & iAFilter::outputValues() const
{
	return m_outputValues;
}

void iAFilter::clearOutput()
{
	for (iAConnector* con : m_output)
	{
		delete con;
	}
	m_output.clear();
}

void iAFilter::addOutput(itk::ImageBase<3>* itkImg)
{
	iAConnector * con = new iAConnector();
	con->setImage(itkImg);
	con->modified();
	m_output.push_back(con);
}

void iAFilter::addOutput(vtkSmartPointer<vtkImageData> img)
{
	iAConnector * con = new iAConnector();
	con->setImage(img);
	con->modified();
	m_output.push_back(con);
}

void iAFilter::setPolyOutput(vtkSmartPointer<vtkPolyData> mesh)
{
	m_outputMesh = mesh;
}

vtkSmartPointer<vtkPolyData> iAFilter::polyOutput() const
{
	return m_outputMesh;
}

QVector<iAConnector*> const & iAFilter::output()
{
	return m_output;
}

int iAFilter::outputCount() const
{
	return m_outputCount;
}

void iAFilter::clearInput()
{
	m_input.clear();
}

void iAFilter::addInput(iAConnector* con)
{
	m_input.push_back(con);
}

QVector<iAConnector*> const & iAFilter::input()
{
	return m_input;
}

itk::ImageIOBase::IOComponentType iAFilter::inputPixelType() const
{
	return m_input[0]->itkScalarPixelType();
}

void iAFilter::setLogger(iALogger* log)
{
	m_log = log;
}

void iAFilter::setProgress(iAProgress* progress)
{
	m_progress = progress;
}

bool iAFilter::run(QMap<QString, QVariant> const & parameters)
{
	if (m_input.size() < m_requiredInputs)
	{
		addMsg(QString("Not enough inputs specified. Filter %1 requires %2 input images, but only %3 given!").arg(m_name).arg(m_requiredInputs).arg(m_input.size()));
		return false;
	}
	clearOutput();
	m_outputValues.clear();
	performWork(parameters);
	return true;
}

bool iAFilter::checkParameters(QMap<QString, QVariant> & parameters)
{
	bool ok;
	for (auto param: m_parameters)
	{
		switch (param->valueType())
		{
		case Discrete: {
			long long value = parameters[param->name()].toLongLong(&ok);
			if (!ok)
			{
				addMsg(QString("Parameter %1: Expected integer value, %2 given.").arg(param->name()).arg(parameters[param->name()].toString()));
				return false;
			}
			if (value < param->min() || value > param->max())
			{
				addMsg(QString("Parameter %1: Given value %2 outside of valid range [%3..%4].")
					.arg(param->name())
					.arg(parameters[param->name()].toString())
					.arg(param->min()).arg(param->max()));
				return false;
			}
			break;
		}
		case Continuous:
		{
			double value = parameters[param->name()].toDouble(&ok);
			if (!ok)
			{
				addMsg(QString("Parameter %1: Expected double value, %2 given.").arg(param->name()).arg(parameters[param->name()].toString()));
				return false;
			}
			if (value < param->min() || value > param->max())
			{
				addMsg(QString("Parameter %1: Given value %2 outside of valid range [%3..%4].")
					.arg(param->name())
					.arg(parameters[param->name()].toString())
					.arg(param->min()).arg(param->max()));
				return false;
			}
			break;
		}
		case Categorical:
		{
			QStringList values = param->defaultValue().toStringList();
			if (!values.contains(parameters[param->name()].toString()))
			{
				addMsg(QString("Parameter %1: Given value '%2' not in the list of valid values (%3).")
					.arg(param->name())
					.arg(parameters[param->name()].toString())
					.arg(values.join(",")));
				return false;
			}
			break;
		}
		case FileNameOpen:
		{
			QFileInfo file(parameters[param->name()].toString());
			if (!file.isFile() || !file.isReadable())
			{
				addMsg(QString("Parameter %1: Given filename '%2' either doesn't reference a file, "
					"the file does not exist, or it is not readable!").arg(param->name()).arg(parameters[param->name()].toString()));
				return false;
			}
			break;
		}
		case FileNamesOpen:
		{
			QStringList files = splitPossiblyQuotedString(parameters[param->name()].toString());
			for (auto fileName : files)
			{
				QFileInfo file(fileName);
				if (!file.isFile() || !file.isReadable())
				{
					addMsg(QString("Parameter %1: Filename '%2' out of the given list '%3' either doesn't reference a file, "
						"the file does not exist, or it is not readable!").arg(param->name())
						.arg(fileName)
						.arg(parameters[param->name()].toString()));
					return false;
				}
			}
			break;
		}
		case Folder:
		{
			// TODO: allow to specify whether the folder can be empty or not!
			QFileInfo file(parameters[param->name()].toString());
			if (!parameters[param->name()].toString().isEmpty() && !file.isDir())
			{
				addMsg(QString("Parameter '%1': Given value '%2' doesn't reference a folder!")
					.arg(param->name()).arg(parameters[param->name()].toString()));
				return false;
			}
			break;
		}
		case Invalid:
			addMsg(QString("Parameter '%1': Invalid parameter type (please contact developers!)!").arg(param->name()));
			return false;
		default:  // no checks
			break;
		}
	}
	return true;
}

void iAFilter::addMsg(QString msg)
{
	m_log->log(msg);
}

iAProgress* iAFilter::progress()
{
	return m_progress;
}

iALogger* iAFilter::logger()
{
	return m_log;
}

QString iAFilter::inputName(int i) const
{
	if (m_inputNames.contains(i))
	{
		return m_inputNames[i];
	}
	else
	{
		return QString("Input %1").arg(i);
	}
}

void iAFilter::setInputName(int i, QString const & name)
{
	m_inputNames.insert(i, name);
}

QString iAFilter::outputName(int i, QString defaultName=NULL) const
{
	if (m_outputNames.contains(i))
	{
		return m_outputNames[i];
	}
	else
	{
		if (defaultName != NULL)
		{
			return defaultName;
		}
		else
		{
			return QString("Out %1").arg(i);
		}
	}
}

void iAFilter::setOutputName(int i, QString const & name)
{
	m_outputNames.insert(i, name);
}

void iAFilter::addParameter(QString const & name, iAValueType valueType,
	QVariant defaultValue, double min, double max)
{
	m_parameters.push_back(iAAttributeDescriptor::createParam(name, valueType, defaultValue, min, max));
}

QVector<QString> const & iAFilter::outputValueNames() const
{
	return m_outputValueNames;
}

void iAFilter::addOutputValue(QString const & name)
{
	m_outputValueNames.push_back(name);
}
