#pragma once

#include "iaio_export.h"

#include "iAAutoRegistration.h"
#include "iAFileIO.h"
#include "iAFileTypeRegistry.h"
#include <vector>


class iAio_API iAVglProjectFile :
	public iAFileIO,
	private iAAutoRegistration<iAFileIO, iAVglProjectFile, iAFileTypeRegistry>
{
public:
	static const QString Name;
	iAVglProjectFile();
	std::shared_ptr<iADataSet> loadData(
		QString const& fileName, QVariantMap const& paramValues, iAProgress const& progress) override;
	QString name() const override;
	QStringList extensions() const override;

	std::vector<char> unzip(QString filename);

};

