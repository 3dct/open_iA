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

//! Dataset type; should probably be removed, since it somewhat duplicates the class hierarchy below iADataSet
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
	
	//! Type of dataset (used to determine the renderer used for the datasets)
	//! @deprecated should be removed; use polymorphy instead (or class type check if really required, e.g. factory method for renderers)
	iADataSetType type() const;
	//! The name of the dataset (defaults to the "basename" of the file)
	QString const& name() const;
	//! The name of the file in which this dataset is stored
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
	QString info() const override;

private:
	vtkSmartPointer<vtkPolyData> m_mesh;
};

class iAbase_API iAGraphData : public iADataSet
{
public:
	iAGraphData(QString const& name, QString const& fileName, vtkSmartPointer<vtkPolyData> mesh);
	vtkSmartPointer<vtkPolyData> poly();
	QString info() const override;

private:
	vtkSmartPointer<vtkPolyData> m_mesh;
};

class iAbase_API iAImageData : public iADataSet
{
public:
	iAImageData(QString const& name, QString const& fileName, vtkSmartPointer<vtkImageData> img);
	vtkSmartPointer<vtkImageData> image();
	QString info() const override;

private:
	vtkSmartPointer<vtkImageData> m_img;
};
