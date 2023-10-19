// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "metafilters_export.h"

#include <iAAttributes.h>

#include <QString>
#include <QVector>

#include <memory>

class iASingleResult;

class MetaFilters_API iASamplingResults
{
public:
	iASamplingResults(
		std::shared_ptr<iAAttributes> attr,
		QString const & samplingMethod,
		QString const & path,
		QString const & executable,
		QString const & additionalArguments,
		QString const & name,
		int id
	);
	static std::shared_ptr<iASamplingResults> load(QString const & metaFileName, int datasetID);
	bool store(QString const& rangeFileName, QString const& parameterSetFileName,
		QString const& derivedOutputFileName);
	int size() const;
	std::shared_ptr<iASingleResult> get(int i) const;
	void addResult(std::shared_ptr<iASingleResult> result);
	QVector<std::shared_ptr<iASingleResult> > const & members() const;
	void setMembers(QVector<std::shared_ptr<iASingleResult> > const& members);
	std::shared_ptr<iAAttributes> attributes() const;
	QString name() const;
	QString fileName() const;
	QString path(int id) const;
	QString path() const;
	QString executable() const;
	QString additionalArguments() const;
	int id() const;
	bool storeAttributes(int type, QString const & fileName, bool id);
private:
	std::shared_ptr<iAAttributes> m_attributes;
	QVector<std::shared_ptr<iASingleResult> > m_results;
	QString m_name;           //!< name of this sampling
	QString m_parameterSetFile;//!<the name of the file containing the parameter sets
	QString m_derivedOutputFile;//!<the name of the file containing the derived outputs
	QString m_samplingMethod; //!< the name of the applied sampling method (Latin Hypercube, Random, ...)
	QString m_rangeFileName;  //!< the name of the file containing parameter ranges that were sampled
	QString m_path;           //!< base filename for the sampling results
	QString m_executable;     //!< executable used to create this sampling
	QString m_additionalArguments; //!< additional parameters passed to the executable
	int m_id;

	bool loadInternal(QString const & parameterSetFileName, QString const & derivedOutputFileName);
};

typedef std::shared_ptr<iASamplingResults> SamplingResultPtr;
typedef std::shared_ptr<QVector<SamplingResultPtr> > SamplingVectorPtr;
