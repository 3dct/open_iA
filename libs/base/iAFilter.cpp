// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAFilter.h"

#include "iAAttributeDescriptor.h"
#include "iAConnector.h"
#include "iADataSet.h"
#include "iALog.h"
#include "iAProgress.h"
#include "iAStringHelper.h"

#include <vtkImageData.h>

#include <QColor>
#include <QFileInfo>

namespace
{

	iAITKIO::ScalarType mapVTKtoITKPixelType(int vtkType)
	{
		switch (vtkType)
		{
		default:
			LOG(lvlError, QString("mapVTKtoITKPixelType: Invalid VTK type %1").arg(vtkType));
#if __cplusplus >= 201703L
			[[fallthrough]];
#endif
			// fall through
		case VTK_CHAR:
#if __cplusplus >= 201703L
			[[fallthrough]];
#endif
			// fall through
		case VTK_SIGNED_CHAR       : return iAITKIO::ScalarType::CHAR;
		case VTK_UNSIGNED_CHAR     : return iAITKIO::ScalarType::UCHAR;
		case VTK_SHORT             : return iAITKIO::ScalarType::SHORT;
		case VTK_UNSIGNED_SHORT    : return iAITKIO::ScalarType::USHORT;
		case VTK_INT               : return iAITKIO::ScalarType::INT;
		case VTK_UNSIGNED_INT      : return iAITKIO::ScalarType::UINT;
		case VTK_LONG              : return iAITKIO::ScalarType::LONG;
		case VTK_UNSIGNED_LONG     : return iAITKIO::ScalarType::ULONG;
		case VTK_LONG_LONG         : return iAITKIO::ScalarType::LONGLONG;
		case VTK_UNSIGNED_LONG_LONG: return iAITKIO::ScalarType::ULONGLONG;
		case VTK_FLOAT             : return iAITKIO::ScalarType::FLOAT;
		case VTK_DOUBLE            : return iAITKIO::ScalarType::DOUBLE;
		}
	}
}

iAFilter::iAFilter(QString const & name, QString const & category, QString const & description,
	unsigned int requiredInputs, unsigned int outputCount, bool supportsAbort) :
	m_progress(std::make_unique<iAProgress>()),
	m_log(iALog::get()),
	m_name(name),
	m_category(category),
	m_description(description),
	m_requiredImages(requiredInputs),
	m_outputCount(outputCount),
	m_requiredMeshes(0),
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

unsigned int iAFilter::requiredImages() const
{
	return m_requiredImages;
}

unsigned int iAFilter::requiredMeshes() const
{
	return m_requiredMeshes;
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
	addOutput(std::make_shared<iAImageData>(itkImg));
}

void iAFilter::addOutput(vtkImageData* vtkImg)
{
	addOutput(std::make_shared<iAImageData>(vtkImg));
}

void iAFilter::addOutput(vtkPolyData* vtkPoly)
{
	addOutput(std::make_shared<iAPolyData>(vtkPoly));
}

void iAFilter::addOutput(std::shared_ptr<iADataSet> dataSet)
{
	dataSet->setMetaData(iADataSet::NameKey, outputName(m_output.size()));
	m_output.push_back(dataSet);
}

std::shared_ptr<iADataSet> iAFilter::output(size_t idx) const
{
	return m_output[idx];
}

iAImageData* iAFilter::imageOutput(size_t idx) const
{
	return dynamic_cast<iAImageData*>(output(idx).get());
}

size_t iAFilter::finalOutputCount() const
{
	return m_output.size();
}

std::vector<std::shared_ptr<iADataSet>> iAFilter::outputs()
{
	return m_output;
}

unsigned int iAFilter::plannedOutputCount() const
{
	return m_outputCount;
}

void iAFilter::clearInput()
{
	m_input.clear();
	m_isAborted = false;
}

void iAFilter::addInput(std::shared_ptr<iADataSet> dataSet)
{
	m_input.push_back(dataSet);
}

std::shared_ptr<iADataSet> iAFilter::input(size_t idx) const
{
	return m_input[idx];
}

iAImageData const* iAFilter::imageInput(size_t idx) const
{
	if (idx >= m_input.size() || !dynamic_cast<iAImageData*>(m_input[idx].get()))
	{
		LOG(lvlError, QString("Invalid imageInput access: idx=%1 outside of valid range, or not an image dataset!").arg(idx));
	}
	return dynamic_cast<iAImageData*>(m_input[idx].get());

}

size_t iAFilter::inputCount() const
{
	return m_input.size();
}

iAITKIO::ScalarType iAFilter::inputScalarType() const
{
	return mapVTKtoITKPixelType(imageInput(0)->vtkImage()->GetScalarType());
}

void iAFilter::setLogger(iALogger* log)
{
	m_log = log;
}

bool iAFilter::run(QVariantMap const & parameters)
{
	if (m_input.size() < (m_requiredImages + m_requiredMeshes) )
	{
		addMsg(QString("Not enough inputs specified. Filter %1 requires %2 input images, but only %3 given!")
			.arg(m_name).arg(m_requiredImages).arg(m_input.size()));
		return false;
	}
	clearOutput();
	m_outputValues.clear();
	performWork(parameters);
	return true;
}

void iAFilter::adaptParametersToInput(QVariantMap& parameters, std::map<size_t, std::shared_ptr<iADataSet>> const& dataSets)
{
	Q_UNUSED(parameters);
	Q_UNUSED(dataSets);
}

bool iAFilter::checkParameters(QVariantMap const & parameters)
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

QString iAFilter::outputName(unsigned int i) const
{
	return m_outputNames.contains(i) ?
		m_outputNames[i] :
		QString("%1%2")
		.arg(name())
		.arg(m_outputCount > 1 ? QString("-Output%2").arg(i) : "");
}

void iAFilter::abort()
{
	m_isAborted = true;
}

bool iAFilter::canAbort() const
{
	return m_canAbort;
}

bool iAFilter::isAborted() const
{
	return m_isAborted;
}

void iAFilter::setOutputName(unsigned int i, QString const & name)
{
	m_outputNames.insert(i, name);
}

void iAFilter::setRequiredMeshInputs(unsigned int i)
{
	m_requiredMeshes = i;
}

iAAttributes& iAFilter::paramsWritable()
{
	return m_parameters;
}

void iAFilter::addParameter(QString const & name, iAValueType valueType,
	QVariant defaultValue, double min, double max)
{
	addAttr(m_parameters, name, valueType, defaultValue, min, max);
}

QVector<QString> const & iAFilter::outputValueNames() const
{
	return m_outputValueNames;
}

void iAFilter::addOutputValue(QString const & name)
{
	m_outputValueNames.push_back(name);
}
