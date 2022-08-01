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

	QString getRegisteredFileTypes(iADataSetTypes allowedTypes)
	{
		Q_UNUSED(allowedTypes);
		// TODO: put together from list of available file loaders!
		return QString("Any supported format (*.mhd *.mha *.stl *.txt);;"
				"Meta Images (*.mhd *.mha);;"
				"STL file (*.stl);;"
				"Graph files (*.txt *.pdb);;");  // (Brookhaven "Protein Data Bank" format (?)
	}
}

namespace
{
	QString meshInfo(vtkPolyData* mesh)
	{
		return QString("Points: %1; Lines: %2; Cells: %3\n")
			.arg(mesh->GetNumberOfPoints()).arg(mesh->GetNumberOfLines()).arg(mesh->GetNumberOfCells()) +
			QString("Polygons: %1; Strips: %2; Pieces: %3")
			.arg(mesh->GetNumberOfPolys()).arg(mesh->GetNumberOfStrips()).arg(mesh->GetNumberOfPieces());
	}}

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

QString iAPolyData::info() const
{
	return meshInfo(m_mesh);
}



iAGraphData::iAGraphData(QString const& name, QString const& fileName, vtkSmartPointer<vtkPolyData> mesh) :
	iADataSet(iADataSetType::dstGraph, name, fileName), m_mesh(mesh)
{
}

vtkSmartPointer<vtkPolyData> iAGraphData::poly()
{
	return m_mesh;
}

QString iAGraphData::info() const
{
	return meshInfo(m_mesh);
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

QString iAImageData::info() const
{
	return
		QString("Extent (pixel): x=%1..%2; y=%3..%4 z=%5..%6\n")
			.arg(m_img->GetExtent()[0]).arg(m_img->GetExtent()[1])
			.arg(m_img->GetExtent()[2]).arg(m_img->GetExtent()[3])
			.arg(m_img->GetExtent()[4]).arg(m_img->GetExtent()[5]) +
		QString("Origin: %1 %2 %3; Spacing: %4 %5 %6\n")
			.arg(m_img->GetOrigin()[0]).arg(m_img->GetOrigin()[1]).arg(m_img->GetOrigin()[2])
			.arg(m_img->GetSpacing()[0]).arg(m_img->GetSpacing()[1]).arg(m_img->GetSpacing()[2]) +
		QString("Data type: %1\n").arg(mapVTKTypeToReadableDataType(m_img->GetScalarType())) +
		QString("Components: %1").arg(m_img->GetNumberOfScalarComponents());
	/*
	if (m_img->GetNumberOfScalarComponents() == 1)  //No histogram statistics for rgb, rgba or vector pixel type images
	{
		if (info.isComputing())
		{
			lWidget->addItem("    Statistics are currently computing...");
		}
		else if (info.voxelCount() == 0)
		{
			lWidget->addItem("    Statistics not computed yet. Activate modality (by clicking on it) to do so.");
		}
		else
		{
			lWidget->addItem(tr("    VoxelCount: %1;  Min: %2;  Max: %3;  Mean: %4;  StdDev: %5;")
								 .arg(info.voxelCount())
								 .arg(info.min())
								 .arg(info.max())
								 .arg(info.mean())
								 .arg(info.standardDeviation()));
		}
	}
	*/
}
