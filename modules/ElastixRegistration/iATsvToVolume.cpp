// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include <iAFilterDefault.h>
#include <iAImageData.h>
#include <iAProgress.h>
#include <iATypedCallHelper.h>

#include <itkImage.h>

#include <QFile>
#include <QTextStream>

IAFILTER_DEFAULT_CLASS(iATsvToVolume);

namespace
{
	QList<float> process_line(QStringList const & line)
	{
		QList<float> data;
		for(QString value : line)
		{
			std::string::size_type sz;
			data.append(std::stof(value.toStdString(), &sz));
		}
		return data;
	}

	QList<float> getMax(QList<QList<float>> const & image)
	{
		QList<float> data;
		float x=0, y=0, z=0;
		for(QList<float> values : image)
		{
			x = x < values[1] ? values[1] : x;
			y = y < values[2] ? values[2] : y;
			z = z < values[3] ? values[3] : z;
		}
		data.append(x);
		data.append(y);
		data.append(z);
		return data;
	}


	void createOutput(QList<QList<float>> const & data, QList<float> const & maxValues, float offset[3], float spacing[3], iAFilter* filter, int indexFile)
	{
		using ImageType = itk::Image<float, 3>;
		ImageType::Pointer image = ImageType::New();

		ImageType::IndexType start;
		start[0] = 0; // first index on X
		start[1] = 0; // first index on Y
		start[2] = 0; // first index on Z

		ImageType::SizeType size;
		size[0] = (maxValues[2] - offset[0]) / spacing[0] + 1; // size along X
		size[1] = (maxValues[1] - offset[1]) / spacing[1] + 1; // size along Y
		size[2] = (maxValues[0] - offset[2]) / spacing[2] + 1; // size along Z

		ImageType::RegionType region;
		region.SetSize(size);
		region.SetIndex(start);
		image->SetRegions(region);
		image->Allocate();
		image->SetSpacing(spacing);
		image->SetOrigin(offset);

		ImageType::IndexType pixelIndex;

		for(QList<float> var: data)
		{
			pixelIndex[0] = (var[1] - offset[0]) / spacing[0];
			pixelIndex[1] = (var[2] - offset[1]) / spacing[1];
			pixelIndex[2] = (var[3] - offset[2]) / spacing[2];

			image->SetPixel(pixelIndex, var[indexFile]);
		}

		filter->addOutput(std::make_shared<iAImageData>(image.GetPointer()));
	}
}

void iATsvToVolume::performWork(QVariantMap const & params)
{
	QFile file(params["File"].toString());
	if (!file.open(QIODevice::ReadOnly))
	{
		return;
	}

	QList<QList<float>> data;

	QTextStream in(&file);
	QString line = in.readLine();//Header

	line = in.readLine();
	while (!line.isNull())
	{
		data.append(process_line(line.split("\t")));
		line = in.readLine();
	}

	float offset[3], spacing[3];

	offset[0] = data.first()[3];
	offset[1] = data.first()[2];
	offset[2] = data.first()[1];

	spacing[0] = data[1][3] - data[0][3];
	spacing[1] = data[1][3] - data[0][3];
	spacing[2] = data[1][3] - data[0][3];

	QList<float> maxValues = getMax(data);

	createOutput(data, maxValues, offset, spacing, this, 4);
	createOutput(data, maxValues, offset, spacing, this, 5);
	createOutput(data, maxValues, offset, spacing, this, 6);

}

iATsvToVolume::iATsvToVolume() :
	iAFilter("TSV reader", "Input",
		"Creates a volume from a tab-separated value (TSV) file.", 0)
{
	addParameter("File", iAValueType::FileNameOpen, ".tsv");

	setOutputName(0u, "Z Displacement");
	setOutputName(1u, "Y Displacement");
	setOutputName(2u, "X Displacement");
}
