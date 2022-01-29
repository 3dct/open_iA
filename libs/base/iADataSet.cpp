#include "iADataSet.h"
#include "iAFileTypeRegistry.h"

#include <QFileInfo>

#include <vtkImageData.h>
#include <vtkPolyData.h>

namespace iAio
{
	std::unique_ptr<iADataSet> loadFile(QString const& fileName, iAProgress* p, iADataSetTypes allowedTypes)
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
		// TODO: maybe include type in io already if known for which type file can contain

		auto result = io->load(fileName, p);  // error handling -> exceptions?
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
	}
}

iADataSet::iADataSet(iADataSetType type, QString const& name, QString const& fileName,
	vtkSmartPointer<vtkImageData> img, vtkSmartPointer<vtkPolyData> mesh) :
	m_type(type), m_name(name), m_fileName(fileName), m_img(img), m_mesh(mesh)
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

vtkSmartPointer<vtkImageData> iADataSet::image()
{
	return m_img;
 }
vtkSmartPointer<vtkPolyData> iADataSet::poly()
{
	return m_mesh;
}
