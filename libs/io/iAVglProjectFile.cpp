#include "iAVglProjectFile.h"

#include <iALog.h>
#include <iASettings.h>
#include <iAValueTypeVectorHelpers.h>
#include <iARawFileIO.h>
#include <iAToolsVTK.h>

#include <zlib.h>
#include <map>

#include <qdom.h>
#include <qxmlstream.h>
#include <QSettings>
#include <qfileinfo.h>
#include <qdir.h>





class parameterVGL
{
	public:
	QString Filename;
	int Gridsize[3];
	float sampleDistance[3];
	float Origin[3];
	QString Datatype;

};


namespace
{
	QMap<QString, QString> const& readableDataTypeMapVGL()
	{
		static QMap<QString, QString> nameVTKTypeMap{{"UInt8", mapVTKTypeToReadableDataType(VTK_UNSIGNED_CHAR)},
			{"Int8", mapVTKTypeToReadableDataType(VTK_SIGNED_CHAR)},
			{"UInt16", mapVTKTypeToReadableDataType(VTK_UNSIGNED_SHORT)},
			{"Int16", mapVTKTypeToReadableDataType(VTK_SHORT)},
			{"UInt32", mapVTKTypeToReadableDataType(VTK_UNSIGNED_INT)},
			{"Int32", mapVTKTypeToReadableDataType(VTK_INT)},
			{"UInt64", mapVTKTypeToReadableDataType(VTK_UNSIGNED_LONG_LONG)},
			{"Int64", mapVTKTypeToReadableDataType(VTK_LONG_LONG)},
			{"Float", mapVTKTypeToReadableDataType(VTK_FLOAT)}, 
			{"Double", mapVTKTypeToReadableDataType(VTK_DOUBLE)}};
		return nameVTKTypeMap;
	}
}

std::shared_ptr<iADataSet> iAVglProjectFile::loadData(
	QString const& fileName, QVariantMap const& paramValues, iAProgress const& progress)
{
	auto XMLdata = unzip(fileName);
	QByteArray data(XMLdata.data(), XMLdata.size()); 

	QDomDocument doc;
	doc.setContent(data);

	auto divNodeList = doc.elementsByTagName("object");

	std::map<QString, parameterVGL> dataset; 

	for (int i = 0; i < divNodeList.size(); ++i)
	{
		QDomElement domElement = divNodeList.at(i).toElement();
		QDomAttr attribute = domElement.attributeNode("class");
		if (attribute.value() == "VGLSampleGridImportRaw")
		{

			parameterVGL param;

			auto VGLSampleGridImportRaw = domElement.elementsByTagName("property");
			for (int j = 0; j < VGLSampleGridImportRaw.size(); ++j)
			{
				QDomElement domElementRAW = VGLSampleGridImportRaw.at(j).toElement();
				QDomAttr attributeRAW = domElementRAW.attributeNode("name");
				if (attributeRAW.value() == "FileName")
				{
					auto pathRawFile = domElementRAW.firstChild().firstChild().nodeValue();
					;
					QFileInfo check_file(pathRawFile);
					if (check_file.exists())
					{
						param.Filename = domElementRAW.firstChild().firstChild().nodeValue();
					}
					else
					{
						auto nameOfFile = check_file.fileName();
						QFileInfo infoVgl(fileName);
						auto dir = infoVgl.dir(); 
						param.Filename = dir.absolutePath() + "/" + nameOfFile;
						
					}
					

				}
				else if (attributeRAW.value() == "GridSize")
				{
					auto arrayIntStrings = domElementRAW.firstChild().firstChild().nodeValue().split(" ");
					if (arrayIntStrings[0].toInt() != 0)
					{
						param.Gridsize[0] = arrayIntStrings[0].toInt();
						param.Gridsize[1] = arrayIntStrings[1].toInt();
						param.Gridsize[2] = arrayIntStrings[2].toInt();
					}
				}
				else if (attributeRAW.value() == "SamplingDistance")
				{
					auto arrayIntStrings = domElementRAW.firstChild().firstChild().nodeValue().split(" ");
					if (param.sampleDistance[0] >= 1.0 || arrayIntStrings[0].toFloat() < 1.0)
					{
						param.sampleDistance[0] = arrayIntStrings[0].toFloat();
						param.sampleDistance[1] = arrayIntStrings[1].toFloat();
						param.sampleDistance[2] = arrayIntStrings[2].toFloat();
					}
				}
				else if (attributeRAW.value() == "SampleDataType")
				{
					if (readableDataTypeMapVGL().contains(domElementRAW.firstChild().firstChild().nodeValue()))
					{
						param.Datatype = domElementRAW.firstChild().firstChild().nodeValue();
					}
				}
				else if (attributeRAW.value() == "Origin")
				{
					auto arrayIntStrings = domElementRAW.firstChild().firstChild().nodeValue().split(" ");

						param.Origin[0] = arrayIntStrings[0].toFloat();
						param.Origin[1] = arrayIntStrings[1].toFloat();
						param.Origin[2] = arrayIntStrings[2].toFloat();

				}

			}

			dataset[param.Filename] = param;

		}
	}

	auto s = std::make_shared<QSettings>(fileName, QSettings::IniFormat);
	auto result = std::make_shared<iADataCollection>(dataset.size(), s);



	for (auto data : dataset)
	{

		QVariantMap iAparam;

		auto param = data.second;

		iAparam.insert(
			iARawFileIO::SizeStr, variantVector<int>({param.Gridsize[0], param.Gridsize[1], param.Gridsize[2]}));
		iAparam.insert(iARawFileIO::OriginStr, variantVector<double>({param.Origin[0], param.Origin[1], param.Origin[2]}));
		iAparam.insert(iARawFileIO::SpacingStr,
			variantVector<double>({param.sampleDistance[0], param.sampleDistance[1], param.sampleDistance[2]}));
		iAparam.insert(iARawFileIO::HeadersizeStr,  0);
		iAparam.insert(iARawFileIO::DataTypeStr, readableDataTypeMapVGL().value(param.Datatype,""));
		iAparam.insert(iARawFileIO::ByteOrderStr,  "Little Endian");

		auto io = iAFileTypeRegistry::createIO(data.second.Filename, iAFileIO::Load);
		if (io)
		{
			auto currentDataSet = io->load(data.second.Filename, iAparam);
			if (currentDataSet != nullptr)
			{
				currentDataSet->setMetaData(iAparam);
				result->addDataSet(currentDataSet);
			}
		}


	}

	return result;
}

iAVglProjectFile::iAVglProjectFile(): iAFileIO(iADataSetType::Collection, iADataSetType::Collection)
{
}



std::vector<char> iAVglProjectFile::unzip(QString filename)
{
	gzFile inFileZ = gzopen(filename.toLocal8Bit().data(), "rb");
	if (inFileZ == NULL)
	{
		LOG(lvlError, QString("Error: Failed to gzopen %1!").arg(filename));
		exit(0);
	}
	char unzipBuffer[8192];
	unsigned int unzippedBytes;
	std::vector<char> unzippedData;
	while (true)
	{
		unzippedBytes = gzread(inFileZ, unzipBuffer, 8192);
		if (unzippedBytes > 0)
		{
			unzippedData.insert(unzippedData.end(), unzipBuffer, unzipBuffer + unzippedBytes);
		}
		else
		{
			break;
		}
	}
	gzclose(inFileZ);

	return unzippedData;
}

const QString iAVglProjectFile::Name("VGL Project files");

QString iAVglProjectFile::name() const
{
	return Name;
}

QStringList iAVglProjectFile::extensions() const
{
	return QStringList{"vgl"};
}
