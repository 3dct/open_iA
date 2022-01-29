#pragma once

#include "iALog.h"

#include <QMap>
#include <QString>

#include <memory>

class iADataSet;
class iAProgress;

class iAFileIO
{
public:
	virtual ~iAFileIO();
	virtual std::unique_ptr<iADataSet> load(QString const& fileName, iAProgress* p) = 0;
};


//! Generic factory class with shared pointers (unique pointers -> trouble in static context?). TODO: Replace iAGenericFactory with this one!
template <typename BaseType>
class iAUPFactory
{
public:
	virtual std::shared_ptr<BaseType> create() = 0;
	virtual ~iAUPFactory()
	{
	}
};

//! Factory for a specific typed derived from BaseType.
template <typename DerivedType, typename BaseType>
class iASpecUPFactory : public iAUPFactory<BaseType>
{
public:
	std::shared_ptr<BaseType> create() override
	{
		return std::make_shared<DerivedType>();
	}
};

using iAIFileIOFactory = iAUPFactory<iAFileIO>;

class iAbase_API iAFileTypeRegistry
{
public:
	//! Adds a given file type to the registry.
	//! TODO: add one handler for multiple extensions!
	template <typename FileIOType>
	static void addFileType(QString const& fileExtension);
	//static QList<QString> const fileTypeKeys();
	static std::shared_ptr<iAFileIO> createIO(QString const& fileExtension);

private:
	static QMap<QString, std::shared_ptr<iAIFileIOFactory>> m_fileTypes;
	iAFileTypeRegistry() = delete;  //!< class is meant to be used statically only, prevent creation of objects
};


template <typename FileIOType>
using iAFileIOFactory = iASpecUPFactory<FileIOType, iAFileIO>;

template <typename FileIOType>
void iAFileTypeRegistry::addFileType(QString const& fileExtension)
{
	if (m_fileTypes.contains(fileExtension))
	{
		LOG(lvlWarn, QString("Trying to add a handler for already registered file type %1!").arg(fileExtension));
	}
	m_fileTypes.insert(fileExtension, std::make_shared<iAFileIOFactory<FileIOType>>());
}

class iAITKFileIO : public iAFileIO
{
public:
	std::unique_ptr<iADataSet> load(QString const& fileName, iAProgress* p) override;
	//static std::shared_ptr<iAFileIO> create();
};
