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
	dstMesh = 2
};

Q_DECLARE_FLAGS(iADataSetTypes, iADataSetType)
Q_DECLARE_OPERATORS_FOR_FLAGS(iADataSetTypes)

//! abstract interface for datasets
class iAbase_API iADataSet
{
public:
	iADataSet(iADataSetType type, QString const& name, QString const& fileName, vtkSmartPointer<vtkImageData> img, vtkSmartPointer<vtkPolyData> mesh);
	//iAVec3d bounds();
	//virtual double const* bounds() const = 0;
	//virtual QString information() const =0;
	iADataSetType type() const;
	QString const& name() const;
	QString const& fileName() const;

	// TODO: move to descendant classes, make collections of images(?):
	// {
	vtkSmartPointer<vtkImageData> image();
	vtkSmartPointer<vtkPolyData> poly();
	// }

private:
	iADataSetType m_type;
	QString m_name;
	QString m_fileName;
	// TODO: move to descendant classes, make collections of images(?):
	// {
	vtkSmartPointer<vtkImageData> m_img;
	vtkSmartPointer<vtkPolyData> m_mesh;
	// }
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