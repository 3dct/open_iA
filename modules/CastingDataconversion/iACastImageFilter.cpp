/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#include "pch.h"
#include "iACastImageFilter.h"

#include "defines.h"          // for DIM
#include "iAConnector.h"
#include "iAProgress.h"
#include "iATypedCallHelper.h"
#include <itkFHWRescaleIntensityImageFilter.h>

#include <itkCastImageFilter.h>
#include <itkImageRegionConstIterator.h>
#include <itkImageRegionIterator.h>
#include <itkLabelToRGBImageFilter.h>
#include <itkRGBPixel.h>
#include <itkRGBAPixel.h>
#include <itkStatisticsImageFilter.h>

class myRGBATypeException : public std::exception
{
	virtual const char* what() const throw()
	{
		return "RGBA Conversion Error: UNSIGNED LONG type needed.";
	}
} myRGBATypeExcep;

template <class InT, class OutT>
void CastImage_template2(iAProgress* p, iAConnector* con)
{
	typedef itk::Image<InT, DIM > InputImageType;
	typedef itk::Image<OutT, DIM> OutputImageType;
	typedef itk::CastImageFilter<InputImageType, OutputImageType> OTIFType;
	typename OTIFType::Pointer filter = OTIFType::New();
	filter->SetInput(dynamic_cast<InputImageType *>(con->GetITKImage()));
	p->Observe(filter);
	filter->Update();
	con->SetImage(filter->GetOutput());
	con->Modified();
	filter->ReleaseDataFlagOn();
}

template<class T> void CastImage_template(std::string datatype, iAProgress* p, iAConnector* con)
{
	if (datatype.compare("VTK_CHAR") == 0 ||  datatype.compare( "VTK_SIGNED_CHAR" ) == 0)
	{
		CastImage_template2<T, char>(p, con);
	}
	else if (datatype.compare( "VTK_UNSIGNED_CHAR" ) == 0)
	{
		CastImage_template2<T, unsigned char>(p, con);
	}
	else if (datatype.compare( "VTK_SHORT" ) == 0)
	{
		CastImage_template2<T, short>(p, con);
	}
	else if (datatype.compare( "VTK_UNSIGNED_SHORT" ) == 0)
	{
		CastImage_template2<T, unsigned short>(p, con);
	}
	else if (datatype.compare( "VTK_INT" ) == 0)
	{
		CastImage_template2<T, int>(p, con);
	}
	else if (datatype.compare( "VTK_UNSIGNED_INT" ) == 0)
	{
		CastImage_template2<T, unsigned int>(p, con);
	}
	else if (datatype.compare( "VTK_LONG" ) == 0)
	{
		CastImage_template2<T, long>(p, con);
	}
	else if (datatype.compare( "VTK_UNSIGNED_LONG" ) == 0)
	{
		CastImage_template2<T, unsigned long>(p, con);
	}
	else if (datatype.compare("VTK_LONG_LONG") == 0 || datatype.compare("VTK__INT64") == 0)
	{
		CastImage_template2<T, long long>(p, con);
	}
	else if (datatype.compare("VTK_UNSIGNED_LONG_LONG") == 0 || datatype.compare("VTK_UNSIGNED__INT64") == 0)
	{
		CastImage_template2<T, unsigned long long>(p, con);
	}
	else if (datatype.compare( "VTK_FLOAT" ) == 0)
	{
		CastImage_template2<T, float>(p, con);
	}
	else if (datatype.compare( "VTK_DOUBLE" ) == 0)
	{
		CastImage_template2<T, double>(p, con);
	}
	else
	{
		throw std::runtime_error("Invalid datatype for casting!");
	}
}

template <class InT, class OutT>
void DataTypeConversion_template2(bool autoInMinMax, double inMin, double inMax,
	bool autoOutMinMax, double outMin, double outMax,
	iAProgress* p, iAConnector* con)
{
	typedef itk::Image<InT, DIM>   InputImageType;
	typedef itk::Image<OutT, DIM> OutputImageType;
	typedef itk::FHWRescaleIntensityImageFilter<InputImageType, OutputImageType> RIIFType;
	typename RIIFType::Pointer filter = RIIFType::New();
	filter->SetInput(dynamic_cast<InputImageType *>(con->GetITKImage()));
	if (autoInMinMax)
	{
		typedef itk::StatisticsImageFilter<InputImageType> StatisticsImageFilterType;
		auto minMaxFilter = StatisticsImageFilterType::New();
		minMaxFilter->ReleaseDataFlagOff();
		minMaxFilter->SetInput(dynamic_cast<InputImageType *>(con->GetITKImage()));
		minMaxFilter->Update();
		filter->SetInputMinimum(minMaxFilter->GetMinimum());
		filter->SetInputMaximum(minMaxFilter->GetMaximum());
	}
	else
	{
		filter->SetInputMinimum(inMin);
		filter->SetInputMaximum(inMax);
	}
	if (autoOutMinMax)
	{
		filter->SetOutputMinimum(std::numeric_limits<OutT>::lowest());
		filter->SetOutputMaximum(std::numeric_limits<OutT>::max());
	}
	else
	{
		filter->SetOutputMinimum(outMin);
		filter->SetOutputMaximum(outMax);
	}
	p->Observe(filter);
	filter->Update();
	con->SetImage(filter->GetOutput());
	con->Modified();
	filter->ReleaseDataFlagOn();
}

template<class T>
void DataTypeConversion_template(std::string datatype,
	bool autoInMinMax, double inMin, double inMax,
	bool autoOutMinMax, double outMin, double outMax,
	iAProgress* p, iAConnector* con)
{
	if (datatype.compare("VTK_UNSIGNED_CHAR") == 0)
	{
		DataTypeConversion_template2<T, unsigned char>(autoInMinMax, inMin, inMax, autoOutMinMax, outMin, outMax, p, con);
	}
	else if (datatype.compare("VTK_CHAR") == 0 || datatype.compare("VTK_SIGNED_CHAR") == 0)
	{
		DataTypeConversion_template2<T, char>(autoInMinMax, inMin, inMax, autoOutMinMax, outMin, outMax, p, con);
	}
	else if (datatype.compare("VTK_SHORT") == 0)
	{
		DataTypeConversion_template2<T, short>(autoInMinMax, inMin, inMax, autoOutMinMax, outMin, outMax, p, con);
	}
	else if (datatype.compare("VTK_UNSIGNED_SHORT") == 0)
	{
		DataTypeConversion_template2<T, unsigned short>(autoInMinMax, inMin, inMax, autoOutMinMax, outMin, outMax, p, con);
	}
	else if (datatype.compare("VTK_INT") == 0)
	{
		DataTypeConversion_template2<T, int>(autoInMinMax, inMin, inMax, autoOutMinMax, outMin, outMax, p, con);
	}
	else if (datatype.compare("VTK_UNSIGNED_INT") == 0)
	{
		DataTypeConversion_template2<T, unsigned int>(autoInMinMax, inMin, inMax, autoOutMinMax, outMin, outMax, p, con);
	}
	else if (datatype.compare("VTK_LONG") == 0)
	{
		DataTypeConversion_template2<T, long>(autoInMinMax, inMin, inMax, autoOutMinMax, outMin, outMax, p, con);
	}
	else if (datatype.compare("VTK_UNSIGNED_LONG") == 0)
	{
		DataTypeConversion_template2<T, unsigned long>(autoInMinMax, inMin, inMax, autoOutMinMax, outMin, outMax, p, con);
	}
	else if (datatype.compare("VTK_LONG_LONG") == 0 || datatype.compare("VTK__INT64") == 0)
	{
		DataTypeConversion_template2<T, long long>(autoInMinMax, inMin, inMax, autoOutMinMax, outMin, outMax, p, con);
	}
	else if (datatype.compare("VTK_UNSIGNED_LONG_LONG") == 0 || datatype.compare("VTK_UNSIGNED__INT64") == 0)
	{
		DataTypeConversion_template2<T, unsigned long long>(autoInMinMax, inMin, inMax, autoOutMinMax, outMin, outMax, p, con);
	}
	else if (datatype.compare("VTK_FLOAT") == 0)
	{
		DataTypeConversion_template2<T, float>(autoInMinMax, inMin, inMax, autoOutMinMax, outMin, outMax, p, con);
	}
	else if (datatype.compare("VTK_DOUBLE") == 0)
	{
		DataTypeConversion_template2<T, double>(autoInMinMax, inMin, inMax, autoOutMinMax, outMin, outMax, p, con);
	}
	else
	{
		throw std::runtime_error("Invalid datatype for rescale!");
	}
}

void ConvertToRGB(iAProgress* p, iAConnector* image)
{
	if (image->GetITKScalarPixelType() != itk::ImageIOBase::ULONG)
		throw  myRGBATypeExcep;

	typedef itk::Image< unsigned long, DIM > LongImageType;
	typedef itk::RGBPixel< unsigned char > RGBPixelType;
	typedef itk::Image< RGBPixelType, DIM > RGBImageType;
	typedef itk::RGBAPixel< unsigned char > RGBAPixelType;
	typedef itk::Image< RGBAPixelType, DIM>  RGBAImageType;

	typedef itk::LabelToRGBImageFilter<LongImageType, RGBImageType> RGBFilterType;
	RGBFilterType::Pointer labelToRGBFilter = RGBFilterType::New();
	labelToRGBFilter->SetInput(dynamic_cast<LongImageType *>(image->GetITKImage()));
	labelToRGBFilter->Update();

	RGBImageType::RegionType region;
	region.SetSize(labelToRGBFilter->GetOutput()->GetLargestPossibleRegion().GetSize());
	region.SetIndex(labelToRGBFilter->GetOutput()->GetLargestPossibleRegion().GetIndex());

	RGBAImageType::Pointer rgbaImage = RGBAImageType::New();
	rgbaImage->SetRegions(region);
	rgbaImage->SetSpacing(labelToRGBFilter->GetOutput()->GetSpacing());
	rgbaImage->Allocate();

	itk::ImageRegionConstIterator< RGBImageType > cit(labelToRGBFilter->GetOutput(), region);
	itk::ImageRegionIterator< RGBAImageType >     it(rgbaImage, region);
	for (cit.GoToBegin(), it.GoToBegin(); !it.IsAtEnd(); ++cit, ++it)
	{
		it.Value().SetRed(cit.Value().GetRed());
		it.Value().SetBlue(cit.Value().GetBlue());
		it.Value().SetGreen(cit.Value().GetGreen());
		it.Value().SetAlpha(255);
	}
	image->SetImage(rgbaImage);
	image->Modified();
	labelToRGBFilter->ReleaseDataFlagOn();
}

IAFILTER_CREATE(iACastImageFilter)

void iACastImageFilter::Run(QMap<QString, QVariant> const & parameters)
{
	iAConnector::ITKScalarPixelType pixelType = m_con->GetITKScalarPixelType();
	if (parameters["Data Type"].toString() == "Label image to color-coded RGBA image")
	{
		ConvertToRGB(m_progress, m_con);
	}
	if (parameters["Rescale Range"].toBool())
	{
		ITK_TYPED_CALL(DataTypeConversion_template, pixelType,
			parameters["Data Type"].toString().toStdString(),
			parameters["Automatic Input Range"].toBool(),
			parameters["Input Min"].toDouble(),
			parameters["Input Max"].toDouble(),
			parameters["Output Min"].toDouble(),
			parameters["Output Max"].toDouble(),
			parameters["Use Full Output Range"].toBool(),
			m_progress, m_con);
	}
	else
	{
		ITK_TYPED_CALL(CastImage_template, pixelType,
			parameters["Data Type"].toString().toStdString(),
			m_progress, m_con);
	}
}

iACastImageFilter::iACastImageFilter() :
	iAFilter("Datatype Conversion", "",
		"Converts an image to another datatype.<br/>"
		"<em>Rescale Range</em> determines whether the intensity values are transformed to another range in that process."
		"All parameters below are only considered in the case that Rescale Ranges is enabled; if it is disabled, the "
		"intensity values stay the same (to the extent that they can be represented in the output datatype).<br/>"
		"<em>Input Minimum</em> and <em>Input Maximum</em> allow to specify the start and end of the input range; "
		"if you want to just use the minimum and maximum of the input image here, enable <em>Automatic Input Range</em>."
		"If <em>Use Full Output Range</em> is checked, then <em>Output Minimum</em> and <em>Maximum</em> are ignored, "
		"instead the minimum and maximum possible values of the chosen output datatype are used.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1CastImageFilter.html\">"
		"Cast Image Filter</a> and the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1RescaleIntensityImageFilter.html\">"
		"Rescale Image Filter</a> in the ITK documentation.")
{
	QStringList datatypes = (QStringList()
		<< QString("VTK_SIGNED_CHAR")
		<< QString("VTK_UNSIGNED_CHAR")
		<< QString("VTK_SHORT")
		<< QString("VTK_UNSIGNED_SHORT")
		<< QString("VTK_INT")
		<< QString("VTK_UNSIGNED_INT")
		<< QString("VTK_LONG")
		<< QString("VTK_UNSIGNED_LONG")
		<< QString("VTK_FLOAT")
		<< QString("VTK_DOUBLE")
		/*	// not yet currently supported by ITK and VTK!
		<< QString("VTK_LONG_LONG")
		<< QString("VTK_UNSIGNED_LONG_LONG")
		<< QString("VTK__INT64")
		<< QString("VTK_UNSIGNED__INT64")
		*/
		<< ("Label image to color-coded RGBA image"));
	AddParameter("Data Type", Categorical, datatypes);
	AddParameter("Rescale Range", Boolean, false);
	AddParameter("Automatic Input Range", Boolean, false);
	AddParameter("Input Min", Continuous, 0);
	AddParameter("Input Max", Continuous, 1);
	AddParameter("Use Full Output Range", Boolean, true);
	AddParameter("Output Min", Continuous, 0);
	AddParameter("Output Max", Continuous, 1);
}
