// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iADataSet.h"

#include <QFileInfo>

#include <array>

const QString iADataSet::NameKey("Name");
const QString iADataSet::FileNameKey("File");
const QString iADataSet::SkipSaveKey("SkipSave");

QString boundsStr(double const* bds)
{
	return QString("Bounds: x=%1..%2, y=%3..%4, z=%5..%6")
		.arg(bds[0]).arg(bds[1])
		.arg(bds[2]).arg(bds[3])
		.arg(bds[4]).arg(bds[5]);
}

iADataSet::iADataSet(iADataSetType type) :
	m_type(type)
{}

iADataSet::~iADataSet()
{}

QString iADataSet::name() const
{
	return metaData(iADataSet::NameKey).toString();
}

std::array<double, 3> iADataSet::unitDistance() const
{
	return { 1.0, 1.0, 1.0 };
}

QString iADataSet::info() const
{
	return "";
}

void iADataSet::setMetaData(QString const& key, QVariant const& value)
{
	m_metaData[key] = value;
}

void iADataSet::setMetaData(QVariantMap const& other)
{
	m_metaData.insert(other);
}

QVariant iADataSet::metaData(QString const& key) const
{
	return m_metaData[key];
}

bool iADataSet::hasMetaData(QString const& key) const
{
	return m_metaData.contains(key);
}

QVariantMap const& iADataSet::allMetaData() const
{
	return m_metaData;
}

iADataSetType iADataSet::type() const
{
	return m_type;
}



iADataCollection::iADataCollection(size_t capacity, std::shared_ptr<QSettings> settings):
	iADataSet(iADataSetType::Collection),
	m_settings(settings)
{
	m_dataSets.reserve(capacity);
}

std::vector<std::shared_ptr<iADataSet>> const & iADataCollection::dataSets() const
{
	return m_dataSets;
}

void iADataCollection::addDataSet(std::shared_ptr<iADataSet> dataSet)
{
	m_dataSets.push_back(dataSet);
}

QString iADataCollection::info() const
{
	return QString("Number of datasets: %1").arg(m_dataSets.size());
}

std::shared_ptr<QSettings> iADataCollection::settings() const
{
	return m_settings;
}
