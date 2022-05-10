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
#include "iAFilter.h"

#include "iAAttributeDescriptor.h"
#include "iAConnector.h"
#include "iALog.h"
#include "iAProgress.h"
#include "iAStringHelper.h"

#include <vtkImageData.h>

#include <QColor>
#include <QFileInfo>

namespace
{
	// TODO: maybe move to iAConnector class itself!
	std::shared_ptr<iAConnector> createConnector(itk::ImageBase<3>* itkImg)
	{
		auto con = std::make_shared<iAConnector>();
		con->setImage(itkImg);
		return con;
	}
	std::shared_ptr<iAConnector> createConnector(vtkSmartPointer<vtkImageData> vtkImg)
	{
		auto con = std::make_shared<iAConnector>();
		con->setImage(vtkImg);
		return con;
	}
}

iAFilter::iAFilter(QString const & name, QString const & category, QString const & description,
	unsigned int requiredInputs, unsigned int outputCount, bool supportsAbort) :
	m_progress(std::make_unique<iAProgress>()),
	m_log(iALog::get()),
	m_name(name),
	m_category(category),
	m_description(description),
	m_requiredInputs(requiredInputs),
	m_outputCount(outputCount),
	m_firstInputChannels(1),
	m_canAbort(supportsAbort)
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

iAAttributes const & iAFilter::parameters() const
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
	if (m_outputValueNames.indexOf(name) == -1)
	{
		LOG(lvlDebug,QString("Please consider calling addOutputValue('%1') "
			"in filter constructor, otherwise some filters (e.g. patch filter) "
			"might not work as expected!").arg(name));
	}
	m_outputValues.push_back(qMakePair(name, value));
}

QVector<QPair<QString, QVariant> > const & iAFilter::outputValues() const
{
	return m_outputValues;
}

void iAFilter::clearOutput()
{
	m_output.clear();
}

void iAFilter::addOutput(itk::ImageBase<3>* itkImg)
{
	m_output.push_back(createConnector(itkImg));
}

void iAFilter::addOutput(vtkSmartPointer<vtkImageData> vtkImg)
{
	m_output.push_back(createConnector(vtkImg));
}

void iAFilter::setPolyOutput(vtkSmartPointer<vtkPolyData> mesh)
{
	m_outputMesh = mesh;
}

vtkSmartPointer<vtkPolyData> iAFilter::polyOutput() const
{
	return m_outputMesh;
}

iAConnector const * iAFilter::output(size_t idx) const
{
	return m_output[idx].get();
}

size_t iAFilter::finalOutputCount() const
{
	return m_output.size();
}

unsigned int iAFilter::plannedOutputCount() const
{
	return m_outputCount;
}

void iAFilter::clearInput()
{
	m_input.clear();
	m_fileNames.clear();
	m_isAborted = false;
}

// TODO: Allow to check type of input files
// (e.g. to check if input images are of a specific type,
// or all of the same type), e.g. in addInput or checkParameters.
void iAFilter::addInput(itk::ImageBase<3>* itkImg, QString const& fileName)
{
	addInput(createConnector(itkImg), fileName);
}

void iAFilter::addInput(vtkSmartPointer<vtkImageData> vtkImg, QString const& fileName)
{
	addInput(createConnector(vtkImg), fileName);
}

void iAFilter::addInput(std::shared_ptr<iAConnector> con, QString const& fileName)
{
	m_input.push_back(con);
	m_fileNames.push_back(fileName);
}

iAConnector const * iAFilter::input(size_t idx) const
{
	return m_input[idx].get();
}

size_t iAFilter::inputCount() const
{
	return m_input.size();
}

QVector<QString> const& iAFilter::fileNames() const
{
	return m_fileNames;
}

itk::ImageIOBase::IOComponentType iAFilter::inputPixelType() const
{
	return m_input[0]->itkScalarPixelType();
}

void iAFilter::setLogger(iALogger* log)
{
	m_log = log;
}

bool iAFilter::run(QMap<QString, QVariant> const & parameters)
{
	if (m_input.size() < m_requiredInputs)
	{
		addMsg(QString("Not enough inputs specified. Filter %1 requires %2 input images, but only %3 given!")
			.arg(m_name).arg(m_requiredInputs).arg(m_input.size()));
		return false;
	}
	clearOutput();
	m_outputValues.clear();
	performWork(parameters);
	return true;
}

void iAFilter::adaptParametersToInput(QMap<QString, QVariant>& parameters, vtkSmartPointer<vtkImageData> img)
{
	Q_UNUSED(parameters);
	Q_UNUSED(img);
}

bool iAFilter::checkParameters(QMap<QString, QVariant> const & parameters)
{
	for (auto param: m_parameters)
	{
		if (!defaultParameterCheck(param, parameters[param->name()]))
		{
			return false;
		}
	}
	return true;
}

bool iAFilter::defaultParameterCheck(QSharedPointer<iAAttributeDescriptor> param, QVariant const& paramValue)
{
	bool ok;
	switch (param->valueType())
	{
	case iAValueType::Discrete:
	{
		long long value = paramValue.toLongLong(&ok);
		if (!ok)
		{
			addMsg(QString("Parameter %1: Expected integer value, %2 given.")
					   .arg(param->name())
					   .arg(paramValue.toString()));
			return false;
		}
		if (value < param->min() || value > param->max())
		{
			addMsg(QString("Parameter %1: Given value %2 outside of valid range [%3..%4].")
					   .arg(param->name())
					   .arg(paramValue.toString())
					   .arg(param->min())
					   .arg(param->max()));
			return false;
		}
		break;
	}
	case iAValueType::Continuous:
	{
		double value = paramValue.toDouble(&ok);
		if (!ok)
		{
			addMsg(QString("Parameter %1: Expected double value, %2 given.")
					   .arg(param->name())
					   .arg(paramValue.toString()));
			return false;
		}
		if (value < param->min() || value > param->max())
		{
			addMsg(QString("Parameter %1: Given value %2 outside of valid range [%3..%4].")
					   .arg(param->name())
					   .arg(paramValue.toString())
					   .arg(param->min())
					   .arg(param->max()));
			return false;
		}
		break;
	}
	case iAValueType::Categorical:
	{
		QStringList values = param->defaultValue().toStringList();
		if (!values.contains(paramValue.toString()))
		{
			addMsg(QString("Parameter %1: Given value '%2' not in the list of valid values (%3).")
					   .arg(param->name())
					   .arg(paramValue.toString())
					   .arg(values.join(",")));
			return false;
		}
		break;
	}
	case iAValueType::FileNameOpen:
	{
		QFileInfo file(paramValue.toString());
		if (!file.isFile() || !file.isReadable())
		{
			addMsg(QString("Parameter %1: Given filename '%2' either doesn't reference a file, "
						   "the file does not exist, or it is not readable!")
					   .arg(param->name())
					   .arg(paramValue.toString()));
			return false;
		}
		break;
	}
	case iAValueType::FileNamesOpen:
	{
		QStringList files = splitPossiblyQuotedString(paramValue.toString());
		for (auto fileName : files)
		{
			QFileInfo file(fileName);
			if (!file.isFile() || !file.isReadable())
			{
				addMsg(
					QString("Parameter %1: Filename '%2' out of the given list '%3' either doesn't reference a file, "
							"the file does not exist, or it is not readable!")
						.arg(param->name())
						.arg(fileName)
						.arg(paramValue.toString()));
				return false;
			}
		}
		break;
	}
	case iAValueType::Folder:
	{
		// TODO: allow to specify whether the folder can be empty or not!
		QFileInfo file(paramValue.toString());
		if (!paramValue.toString().isEmpty() && !file.isDir())
		{
			addMsg(QString("Parameter '%1': Given value '%2' doesn't reference a folder!")
					   .arg(param->name())
					   .arg(paramValue.toString()));
			return false;
		}
		break;
	}
	case iAValueType::Color:
	{
		QColor color(paramValue.toString());
		if (!color.isValid())
		{
			addMsg(QString("Parameter '%1': '%2' is not a valid color value; "
						   "please either give a color name (e.g. blue, green, ...) "
						   "or a hexadecimal RGB specifier, like #RGB, #RRGGBB!")
					   .arg(param->name())
					   .arg(paramValue.toString()));
			return false;
		}
		break;
	}
	case iAValueType::Invalid:
		addMsg(QString("Parameter '%1': Invalid parameter type (please contact developers!)!").arg(param->name()));
		return false;
	default:  // no checks
		break;
	}
	return true;
}

void iAFilter::addMsg(QString const & msg)
{
	m_log->log(lvlInfo, msg);
}

iAProgress* iAFilter::progress()
{
	return m_progress.get();
}

iALogger* iAFilter::logger()
{
	return m_log;
}

QString iAFilter::inputName(unsigned int i) const
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

void iAFilter::setInputName(unsigned int i, QString const & name)
{
	m_inputNames.insert(i, name);
}

QString iAFilter::outputName(unsigned int i, QString defaultName) const
{
	if (m_outputNames.contains(i))
	{
		return m_outputNames[i];
	}
	else
	{
		if (!defaultName.isEmpty())
		{
			return defaultName;
		}
		else
		{
			return QString("Out %1").arg(i);
		}
	}
}

void iAFilter::abort()
{
	m_isAborted = false;
}

bool iAFilter::canAbort() const
{
	return false;
}

bool iAFilter::isAborted() const
{
	return m_isAborted;
}

void iAFilter::setOutputName(unsigned int i, QString const & name)
{
	m_outputNames.insert(i, name);
}

iAAttributes& iAFilter::paramsWritable()
{
	return m_parameters;
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
