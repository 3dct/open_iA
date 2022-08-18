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
#pragma once

#include "iAAttributes.h"
#include "iADataSet.h"
#include "iALog.h"

#include <QMap>
#include <QString>

#include <memory>
#include <vector>

class iAProgress;

//! Dataset type contained in files
enum class iADataSetType
{
	Volume = 0x1,
	Mesh   = 0x2,
	Graph  = 0x4,
	All    = Volume | Mesh | Graph
};
Q_DECLARE_FLAGS(iADataSetTypes, iADataSetType)
Q_DECLARE_OPERATORS_FOR_FLAGS(iADataSetTypes)

class iAbase_API iAFileIO
{
public:
	iAFileIO(iADataSetTypes type);
	void setup(QString const& fileName);  // TODO: make possible to also use e.g. folder name or list of files
	virtual ~iAFileIO();
	//! The name of the file type that this IO supports
	virtual QString name() const = 0;
	//! The file extensions that this file IO should be used for
	virtual QStringList extensions() const = 0;
	//! Load the dataset
	virtual std::shared_ptr<iADataSet> load(iAProgress* p, QMap<QString, QVariant> const& paramValues) = 0;
	//! Required parameters for loading the file
	//! Copied from iAFilter - maybe reuse? move to new common base class iAParameterizedSomething ...?
	iAAttributes const& parameters() const;
	//! Types of dataset that the the file format which this IO is for delivers
	iADataSetTypes supportedDataSetTypes() const;

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
	iADataSetTypes m_dataSetTypes;
};


//! Generic factory class with shared pointers
//! TODO: Replace iAGenericFactory with this one!
// (unique pointers -> trouble in static context?).
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

class iAbase_API iAFileTypeRegistry final
{
public:
	//! Adds a given file type to the registry.
	template <typename FileIOType> static void addFileType();
	static std::shared_ptr<iAFileIO> createIO(QString const& fileExtension);
	
	//! set up default IO factories included by default in open_iA
	static void setupDefaultIOFactories();

	//! retrieve list of file types for file open dialog
	static QString registeredFileTypes(iADataSetTypes allowedTypes = iADataSetType::All);

private:
	static std::vector<std::shared_ptr<iAIFileIOFactory>> m_fileIOs;
	static QMap<QString, size_t> m_fileTypes;
	iAFileTypeRegistry() = delete;  //!< class is meant to be used statically only, prevent creation of objects
};


template <typename FileIOType>
using iAFileIOFactory = iASpecUPFactory<FileIOType, iAFileIO>;

template <typename FileIOType>
void iAFileTypeRegistry::addFileType()
{
	auto ioFactory = std::make_shared<iAFileIOFactory<FileIOType>>();
	m_fileIOs.push_back(ioFactory);
	auto io = ioFactory->create();
	for (auto extension : io->extensions())
	{
		if (m_fileTypes.contains(extension))
		{
			LOG(lvlWarn, QString("File IO %1 tries to add a handler for file extension %2, already registered to file IO %3!")
				.arg(io->name())
				.arg(extension)
				.arg(m_fileIOs[m_fileTypes[extension]]->create()->name()));
		}
		m_fileTypes.insert(extension, m_fileIOs.size() - 1);
	}
}

namespace iANewIO
{
	//! get a I/O object for a file with the given filename
	iAbase_API std::shared_ptr<iAFileIO> createIO(QString fileName);
}

/*
std::unique_ptr<iAVolumeDataSet> loadVolumeFile(QString const& fileName)
{
	return loadFile(fileName, dstVolume);
}

std::unique_ptr<iAMeshDataSet> loadMeshFile(QString const& fileName)
{
	return loadFile(fileName, dstMesh);
}
*/
