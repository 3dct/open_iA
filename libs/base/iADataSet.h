#pragma once

#include "iAbase_export.h"

#include "iALog.h"

#include <vtkSmartPointer.h>

#include <QFlags>
#include <QString>

class iADataSet;
class iAProgress;

class vtkPolyData;
class vtkImageData;

enum iADataSetType
{
	dstVolume = 1,
	dstMesh = 2,
	dstGraph = 3
};

Q_DECLARE_FLAGS(iADataSetTypes, iADataSetType)
Q_DECLARE_OPERATORS_FOR_FLAGS(iADataSetTypes)

//! abstract interface for datasets
class iAbase_API iADataSet
{
public:
	//iAVec3d bounds();
	//virtual double const* bounds() const = 0;
	//virtual QString information() const =0;
	iADataSetType type() const;
	QString const& name() const;
	QString const& fileName() const;
	virtual QString info() const;

protected:
	iADataSet(iADataSetType type, QString const& name, QString const& fileName);

private:
	iADataSetType m_type;
	QString m_name;
	QString m_fileName;
};

class iAbase_API iAPolyData : public iADataSet
{
public:
	iAPolyData(QString const& name, QString const& fileName, vtkSmartPointer<vtkPolyData> mesh);
	vtkSmartPointer<vtkPolyData> poly();
private:
	vtkSmartPointer<vtkPolyData> m_mesh;
};

class iAbase_API iAGraphData : public iADataSet
{
public:
	iAGraphData(QString const& name, QString const& fileName, vtkSmartPointer<vtkPolyData> mesh);
	vtkSmartPointer<vtkPolyData> poly();

private:
	vtkSmartPointer<vtkPolyData> m_mesh;
};

class iAbase_API iAImageData : public iADataSet
{
public:
	iAImageData(QString const& name, QString const& fileName, vtkSmartPointer<vtkImageData> img);
	vtkSmartPointer<vtkImageData> image();
private:
	vtkSmartPointer<vtkImageData> m_img;
};

class iAFileIO;

// maybe: vector of datasets?
// move to iAFileIO or similar?
namespace iANewIO
{
	//! get a I/O object for a file with the given filename
	iAbase_API std::shared_ptr<iAFileIO> createIO(
		QString fileName, iADataSetTypes allowedTypes = dstVolume | dstMesh);
	//! set up the default file loaders included in the base library
	iAbase_API void setupDefaultIOFactories();
	//! retrieve list of file types for file open dialog
	iAbase_API QString getRegisteredFileTypes(iADataSetTypes allowedTypes = dstVolume | dstMesh);
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