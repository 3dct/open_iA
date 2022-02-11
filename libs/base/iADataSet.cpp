#include "iADataSet.h"
#include "iAFileTypeRegistry.h"
#include "iAToolsVTK.h"

#include <QFileInfo>

#include <vtkImageData.h>
#include <vtkPolyData.h>

namespace iANewIO
{
	iADataSet* loadFile(QString const& fileName, iAProgress* p, iADataSetTypes allowedTypes)
	{
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
		// TODO: include type in io already to indicate which type file can contain (and check here / only try loaders for allowedTypes)

		auto result = io->load(fileName, p);  // error handling -> exceptions?

		//storeImage(result->image(), "C:/fh/testnewio3.mhd", false);
		if (!allowedTypes.testFlag(result->type()))
		{
			LOG(lvlWarn,
				QString("Failed to load %1: The loaded dataset type %2 does not match allowed any allowed type.")
					.arg(fileName)
					.arg(result->type()));
			return {};
		}
		return result;
	}

	void setupDefaultIOFactories()
	{
		iAFileTypeRegistry::addFileType<iAITKFileIO>("mhd");
		iAFileTypeRegistry::addFileType<iAGraphFileIO>("txt");
	}

	iAbase_API QString getRegisteredFileTypes(iADataSetTypes allowedTypes)
	{
		Q_UNUSED(allowedTypes);
		// TODO: put together from list of available file loaders!
		return QString("Any supported format (*.mhd *.mha *.txt);;"
				"Meta Images (*.mhd *.mha);;"
			   "Graph files (*.txt *.pdb);;");  // (Brookhaven "Protein Data Bank" format (?)
	}
}

iADataSet::iADataSet(iADataSetType type, QString const& name, QString const& fileName,
	vtkSmartPointer<vtkImageData> img, vtkSmartPointer<vtkPolyData> mesh) :
	m_type(type), m_name(name), m_fileName(fileName),
	m_img(vtkSmartPointer<vtkImageData>::New()), m_mesh(vtkSmartPointer<vtkPolyData>::New())
{
	if (img)
	{
		m_img->DeepCopy(img);
		//storeImage(m_img, "C:/fh/testnewio2.5vtk.mhd", false);
	}
	if (mesh)
	{
		m_mesh->DeepCopy(mesh);
	}
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

vtkSmartPointer<vtkImageData> iADataSet::image()
{
	return m_img;
 }
vtkSmartPointer<vtkPolyData> iADataSet::poly()
{
	return m_mesh;
}
