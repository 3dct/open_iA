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

//! abstract interface for datasets
class iAbase_API iADataSet
{
public:
	//iAVec3d bounds();
	//virtual double const* bounds() const = 0;
	//virtual QString information() const =0;
	
	//! The name of the dataset (defaults to the "basename" of the file)
	QString const& name() const;
	//! The name of the file in which this dataset is stored
	QString const& fileName() const;

	virtual QString info() const;

protected:
	iADataSet(QString const& name, QString const& fileName);

private:
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
