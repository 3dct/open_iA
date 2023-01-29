// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAFileIO.h"

#include "iALog.h"

#include <QElapsedTimer>
#include <QFileInfo>

const QString iAFileIO::CompressionStr("Compression");

iAFileIO::iAFileIO(iADataSetTypes loadTypes, iADataSetTypes saveTypes) :
	m_dataSetTypes{ loadTypes, saveTypes }
{
	addAttr(m_params[Load], iADataSet::FileNameKey, iAValueType::FileNameOpen, "");
}

iAFileIO::~iAFileIO() = default;

std::shared_ptr<iADataSet> iAFileIO::load(QString const& fileName, QVariantMap const& paramValues, iAProgress const& progress)
{
	try
	{
		QElapsedTimer t; t.start();
		QVariantMap checkedValues(paramValues);
		checkParams(checkedValues, Operation::Load, fileName);
		auto dataSet = loadData(fileName, checkedValues, progress);
		if (!dataSet)
		{
			return {};
		}
		dataSet->setMetaData(iADataSet::FileNameKey, fileName);
		dataSet->setMetaData(iADataSet::NameKey, QFileInfo(fileName).completeBaseName());
		for (auto k : checkedValues.keys())
		{
			dataSet->setMetaData(k, checkedValues[k]);
		}
		LOG(lvlInfo, QString("Loaded dataset %1 in %2 ms.").arg(fileName).arg(t.elapsed()));
		return dataSet;
	}
	// TODO NEWIO: unify exception handling? maybe move somewhere else?
	catch (itk::ExceptionObject& e)
	{
		LOG(lvlError, QString("Error loading file %1: %2").arg(fileName).arg(e.GetDescription()));
	}
	catch (std::exception& e)
	{
		LOG(lvlError, QString("Error loading file %1: %2").arg(fileName).arg(e.what()));
	}
	catch (...)
	{
		LOG(lvlError, QString("Unknown error while loading file %1!").arg(fileName));
	}
	return {};
}

QStringList iAFileIO::filterExtensions()
{
	auto extCpy = extensions();
	for (auto& ext : extCpy)
	{
		ext = "*." + ext;
	}
	return extCpy;
}

QString iAFileIO::filterString()
{
	return QString("%1 (%2)").arg(name()).arg(filterExtensions().join(" "));
}

std::shared_ptr<iADataSet> iAFileIO::loadData(QString const& fileName, QVariantMap const& paramValues, iAProgress const& progress)
{
	Q_UNUSED(fileName);
	Q_UNUSED(paramValues);
	Q_UNUSED(progress);
	return {};
}

void iAFileIO::saveData(QString const& fileName, std::shared_ptr<iADataSet> dataSet, QVariantMap const& paramValues, iAProgress const& progress)
{
	Q_UNUSED(fileName);
	Q_UNUSED(dataSet);
	Q_UNUSED(paramValues);
	Q_UNUSED(progress);
}

iAAttributes const& iAFileIO::parameter(Operation op) const
{
	return m_params[op];
}

iADataSetTypes iAFileIO::supportedDataSetTypes(Operation op) const
{
	return m_dataSetTypes[op];
}

bool iAFileIO::isDataSetSupported(std::shared_ptr<iADataSet> dataSet, QString const& fileName) const
{
	Q_UNUSED(dataSet);
	Q_UNUSED(fileName);
	return true;
}

void iAFileIO::save(QString const& fileName, std::shared_ptr<iADataSet> dataSet, QVariantMap const& paramValues, iAProgress const& progress)
{
	QElapsedTimer t; t.start();
	QVariantMap checkedValues(paramValues);
	checkParams(checkedValues, Save, fileName);
	saveData(fileName, dataSet, checkedValues, progress);
	dataSet->setMetaData(iADataSet::FileNameKey, fileName);
	LOG(lvlInfo, QString("Saved %1 in %2 ms.").arg(fileName).arg(t.elapsed()));
}

bool iAFileIO::checkParams(QVariantMap & paramValues, Operation op, QString const& fileName)
{
	bool result = true;
	paramValues[iADataSet::FileNameKey] = fileName;
	for (auto p : m_params[op])
	{
		// TODO NEWIO: maybe also check whether the values fulfill the conditions in the iAAttributeDescriptor? see iAFilter::checkParameters
		if (!paramValues.contains(p->name()))
		{
			LOG(lvlWarn, QString("Missing parameter %1 when %2 file %3; setting to default %4!")
				.arg(p->name()).arg(op == Save ? "saving" : "loading").arg(fileName)
				.arg(p->defaultValue().toString())
			);
			result = false;
		}
	}
	return result;
}
