#pragma once

#include "iAAttributes.h"
#include "iADataSet.h"    // for iADataSetType...
#include "iALog.h"

#include <QMap>
#include <QString>

#include <memory>

class iAProgress;

class iAbase_API iAFileIO
{
public:
	iAFileIO(iADataSetType type);
	void setup(QString const& fileName);  // TODO: make possible to also use e.g. folder name or list of files
	virtual ~iAFileIO();
	virtual std::shared_ptr<iADataSet> load(iAProgress* p, QMap<QString, QVariant> const& paramValues) = 0;
	// copied from iAFilter - maybe reuse? move to new common base class iAParameterizedSomething ...?
	iAAttributes const& parameters() const;
	iADataSetType type() const;

protected:
	//! Adds the description of a parameter to the filter.
	//! @param name the parameter's name
	//! @param valueType the type of value this parameter can have
	//! @param defaultValue the default value of the parameter; for Categorical
	//!     valueTypes, this should be the list of possible values
	//! @param min the minimum value this parameter can have (inclusive).
	//! @param max the maximum value this parameter can have (inclusive)
	void addParameter(QString const& name, iAValueType valueType, QVariant defaultValue = 0.0,
		double min = std::numeric_limits<double>::lowest(), double max = std::numeric_limits<double>::max());

	QString m_fileName;

private:
	iAAttributes m_parameters;
	iADataSetType m_type;					// TODO: make possible to reflect files which can contain multipe types
};


//! Generic factory class with shared pointers
// (unique pointers -> trouble in static context?). TODO: Replace iAGenericFactory with this one!
// error C2280: "std::unique_ptr<iAIFileIOFactory,std::default_delete<iAIFileIOFactory>> &std::unique_ptr<iAIFileIOFactory,std::default_delete<iAIFileIOFactory>>::operator =(const std::unique_ptr<iAIFileIOFactory,std::default_delete<iAIFileIOFactory>> &)" : Es wurde versucht, auf eine gelöschte Funktion zu verweisen
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
	iAITKFileIO();
	std::shared_ptr<iADataSet> load(iAProgress* p, QMap<QString, QVariant> const& parameters) override;
};

class iAGraphFileIO: public iAFileIO
{
public:
	iAGraphFileIO();
	std::shared_ptr<iADataSet> load(iAProgress* p, QMap<QString, QVariant> const& parameters) override;
};