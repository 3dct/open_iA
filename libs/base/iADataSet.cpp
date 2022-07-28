#include "iADataSet.h"
#include "iAFileTypeRegistry.h"
#include "iAToolsVTK.h"

#include <QFileInfo>

#include <vtkImageData.h>
#include <vtkPolyData.h>

namespace iANewIO
{
	std::shared_ptr<iAFileIO> createIO(QString fileName, iADataSetTypes allowedTypes)
	{

	//iADataSet* loadFile(QString const& fileName, iAProgress* p, iAParamSource paramSource, iADataSetTypes allowedTypes)
	//{
		QFileInfo fi(fileName);
		// special handling for directory ? TLGICT-loader... -> fi.isDir();
		auto io = iAFileTypeRegistry::createIO(fi.suffix());
		if (!io)
		{
			LOG(lvlWarn,
				QString("Failed to load %1: There is no handler registered files with suffix '%2'")
					.arg(fileName)
					.arg(fi.suffix()));
			return {};
		}
		io->setup(fileName);
		// TODO: extend type check in io / only try loaders for allowedTypes (but how to handle file types that can contain multiple?)
		if (!allowedTypes.testFlag(io->type()))
		{
			LOG(lvlWarn,
				QString("Failed to load %1: The loaded dataset type %2 does not match allowed any allowed type.")
					.arg(fileName)
					.arg(io->type()));
			return {};
		}
		return io;
	}

	iAbase_API QString getRegisteredFileTypes(iADataSetTypes allowedTypes)
	{
		Q_UNUSED(allowedTypes);
		// TODO: put together from list of available file loaders!
		return QString("Any supported format (*.mhd *.mha *.stl *.txt);;"
				"Meta Images (*.mhd *.mha);;"
				"STL file (*.stl);;"
				"Graph files (*.txt *.pdb);;");  // (Brookhaven "Protein Data Bank" format (?)
	}
}

iADataSet::iADataSet(iADataSetType type, QString const& name, QString const& fileName) :
	m_type(type), m_name(name), m_fileName(fileName)
{
}

iADataSetType iADataSet::type() const
{
	return m_type;
}

QString const& iADataSet::name() const
{
	return m_name;
}

QString const& iADataSet::fileName() const
{
	return m_fileName;
}

QString iADataSet::info() const
{
	return "";
}

iAPolyData::iAPolyData(QString const& name, QString const& fileName, vtkSmartPointer<vtkPolyData> mesh) :
	iADataSet(iADataSetType::dstMesh, name, fileName),
	m_mesh(mesh)
{
}

vtkSmartPointer<vtkPolyData> iAPolyData::poly()
{
	return m_mesh;
}



iAGraphData::iAGraphData(QString const& name, QString const& fileName, vtkSmartPointer<vtkPolyData> mesh) :
	iADataSet(iADataSetType::dstGraph, name, fileName), m_mesh(mesh)
{
}

vtkSmartPointer<vtkPolyData> iAGraphData::poly()
{
	return m_mesh;
}



iAImageData::iAImageData(QString const& name, QString const& fileName, vtkSmartPointer<vtkImageData> img):
	iADataSet(iADataSetType::dstVolume, name, fileName),
	m_img(img)
{
}

vtkSmartPointer<vtkImageData> iAImageData::image()
{
	return m_img;
}
