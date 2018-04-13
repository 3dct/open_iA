/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
#include "iACastImageFilter.h"

#include "defines.h"          // for DIM
#include "iAConnector.h"
#include "iAProgress.h"
#include "iAToolsVTK.h"    // for VTKDataTypeList
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

template <class InT, class OutT> void CastImage(iAFilter* filter)
{
	typedef itk::Image<InT, DIM > InputImageType;
	typedef itk::Image<OutT, DIM> OutputImageType;
	typedef itk::CastImageFilter<InputImageType, OutputImageType> OTIFType;
	typename OTIFType::Pointer castFilter = OTIFType::New();
	castFilter->SetInput(dynamic_cast<InputImageType *>(filter->Input()[0]->GetITKImage()));
	filter->Progress()->Observe(castFilter);
	castFilter->Update();
	filter->AddOutput(castFilter->GetOutput());
}

template<class T> void CastImage(iAFilter* filter, std::string datatype)
{
	if (datatype.compare("VTK_CHAR") == 0 ||  datatype.compare( "VTK_SIGNED_CHAR" ) == 0)
	{
		CastImage<T, char>(filter);
	}
	else if (datatype.compare( "VTK_UNSIGNED_CHAR" ) == 0)
	{
		CastImage<T, unsigned char>(filter);
	}
	else if (datatype.compare( "VTK_SHORT" ) == 0)
	{
		CastImage<T, short>(filter);
	}
	else if (datatype.compare( "VTK_UNSIGNED_SHORT" ) == 0)
	{
		CastImage<T, unsigned short>(filter);
	}
	else if (datatype.compare( "VTK_INT" ) == 0)
	{
		CastImage<T, int>(filter);
	}
	else if (datatype.compare( "VTK_UNSIGNED_INT" ) == 0)
	{
		CastImage<T, unsigned int>(filter);
	}
	else if (datatype.compare( "VTK_LONG" ) == 0)
	{
		CastImage<T, long>(filter);
	}
	else if (datatype.compare( "VTK_UNSIGNED_LONG" ) == 0)
	{
		CastImage<T, unsigned long>(filter);
	}
	else if (datatype.compare("VTK_LONG_LONG") == 0 || datatype.compare("VTK__INT64") == 0)
	{
		CastImage<T, long long>(filter);
	}
	else if (datatype.compare("VTK_UNSIGNED_LONG_LONG") == 0 || datatype.compare("VTK_UNSIGNED__INT64") == 0)
	{
		CastImage<T, unsigned long long>(filter);
	}
	else if (datatype.compare( "VTK_FLOAT" ) == 0)
	{
		CastImage<T, float>(filter);
	}
	else if (datatype.compare( "VTK_DOUBLE" ) == 0)
	{
		CastImage<T, double>(filter);
	}
	else
	{
		throw std::runtime_error("Invalid datatype for casting!");
	}
}

template <class InT, class OutT>
void DataTypeConversion(iAFilter* filter, QMap<QString, QVariant> const & parameters)
{
	typedef itk::Image<InT, DIM>   InputImageType;
	typedef itk::Image<OutT, DIM> OutputImageType;
	typedef itk::FHWRescaleIntensityImageFilter<InputImageType, OutputImageType> RIIFType;
	typename RIIFType::Pointer rescaleFilter = RIIFType::New();
	rescaleFilter->SetInput(dynamic_cast<InputImageType *>(filter->Input()[0]->GetITKImage()));
	if (parameters["Automatic Input Range"].toBool())
	{
		typedef itk::StatisticsImageFilter<InputImageType> StatisticsImageFilterType;
		auto minMaxFilter = StatisticsImageFilterType::New();
		minMaxFilter->ReleaseDataFlagOff();
		minMaxFilter->SetInput(dynamic_cast<InputImageType *>(filter->Input()[0]->GetITKImage()));
		minMaxFilter->Update();
		rescaleFilter->SetInputMinimum(minMaxFilter->GetMinimum());
		rescaleFilter->SetInputMaximum(minMaxFilter->GetMaximum());
	}
	else
	{
		rescaleFilter->SetInputMinimum(parameters["Input Min"].toDouble());
		rescaleFilter->SetInputMaximum(parameters["Input Max"].toDouble());
	}
	if (parameters["Use Full Output Range"].toBool())
	{
		rescaleFilter->SetOutputMinimum(std::numeric_limits<OutT>::lowest());
		rescaleFilter->SetOutputMaximum(std::numeric_limits<OutT>::max());
	}
	else
	{
		rescaleFilter->SetOutputMinimum(parameters["Output Min"].toDouble());
		rescaleFilter->SetOutputMaximum(parameters["Output Max"].toDouble());
	}
	filter->Progress()->Observe(rescaleFilter);
	rescaleFilter->Update();
	filter->AddOutput(rescaleFilter->GetOutput());
}

template<class T>
void DataTypeConversion(iAFilter* filter, QMap<QString, QVariant> const & parameters)
{
	std::string datatype = parameters["Data Type"].toString().toStdString();
	if (datatype.compare("VTK_UNSIGNED_CHAR") == 0)
	{
		DataTypeConversion<T, unsigned char>(filter, parameters);
	}
	else if (datatype.compare("VTK_CHAR") == 0 || datatype.compare("VTK_SIGNED_CHAR") == 0)
	{
		DataTypeConversion<T, char>(filter, parameters);
	}
	else if (datatype.compare("VTK_SHORT") == 0)
	{
		DataTypeConversion<T, short>(filter, parameters);
	}
	else if (datatype.compare("VTK_UNSIGNED_SHORT") == 0)
	{
		DataTypeConversion<T, unsigned short>(filter, parameters);
	}
	else if (datatype.compare("VTK_INT") == 0)
	{
		DataTypeConversion<T, int>(filter, parameters);
	}
	else if (datatype.compare("VTK_UNSIGNED_INT") == 0)
	{
		DataTypeConversion<T, unsigned int>(filter, parameters);
	}
	else if (datatype.compare("VTK_LONG") == 0)
	{
		DataTypeConversion<T, long>(filter, parameters);
	}
	else if (datatype.compare("VTK_UNSIGNED_LONG") == 0)
	{
		DataTypeConversion<T, unsigned long>(filter, parameters);
	}
	else if (datatype.compare("VTK_LONG_LONG") == 0 || datatype.compare("VTK__INT64") == 0)
	{
		DataTypeConversion<T, long long>(filter, parameters);
	}
	else if (datatype.compare("VTK_UNSIGNED_LONG_LONG") == 0 || datatype.compare("VTK_UNSIGNED__INT64") == 0)
	{
		DataTypeConversion<T, unsigned long long>(filter, parameters);
	}
	else if (datatype.compare("VTK_FLOAT") == 0)
	{
		DataTypeConversion<T, float>(filter, parameters);
	}
	else if (datatype.compare("VTK_DOUBLE") == 0)
	{
		DataTypeConversion<T, double>(filter, parameters);
	}
	else
	{
		throw std::runtime_error("Invalid datatype for rescale!");
	}
}

void ConvertToRGB(iAFilter * filter)
{
	if (filter->InputPixelType() != itk::ImageIOBase::ULONG)
		throw  myRGBATypeExcep;

	typedef itk::Image< unsigned long, DIM > LongImageType;
	typedef itk::RGBPixel< unsigned char > RGBPixelType;
	typedef itk::Image< RGBPixelType, DIM > RGBImageType;
	typedef itk::RGBAPixel< unsigned char > RGBAPixelType;
	typedef itk::Image< RGBAPixelType, DIM>  RGBAImageType;

	typedef itk::LabelToRGBImageFilter<LongImageType, RGBImageType> RGBFilterType;
	RGBFilterType::Pointer labelToRGBFilter = RGBFilterType::New();
	labelToRGBFilter->SetInput(dynamic_cast<LongImageType *>(filter->Input()[0]->GetITKImage()));
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
	filter->AddOutput(rgbaImage);
}

IAFILTER_CREATE(iACastImageFilter)

void iACastImageFilter::PerformWork(QMap<QString, QVariant> const & parameters)
{
	if (parameters["Data Type"].toString() == "Label image to color-coded RGBA image")
	{
		ConvertToRGB(this);
	}
	if (parameters["Rescale Range"].toBool())
	{
		ITK_TYPED_CALL(DataTypeConversion, InputPixelType(), this, parameters);
	}
	else
	{
		ITK_TYPED_CALL(CastImage, InputPixelType(), this, parameters["Data Type"].toString().toStdString());
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
	QStringList datatypes = VTKDataTypeList();
	datatypes
		/*	// not yet currently supported by ITK and VTK!
		<< QString("VTK_LONG_LONG")
		<< QString("VTK_UNSIGNED_LONG_LONG")
		<< QString("VTK__INT64")
		<< QString("VTK_UNSIGNED__INT64")
		*/
		<< ("Label image to color-coded RGBA image");
	AddParameter("Data Type", Categorical, datatypes);
	AddParameter("Rescale Range", Boolean, false);
	AddParameter("Automatic Input Range", Boolean, false);
	AddParameter("Input Min", Continuous, 0);
	AddParameter("Input Max", Continuous, 1);
	AddParameter("Use Full Output Range", Boolean, true);
	AddParameter("Output Min", Continuous, 0);
	AddParameter("Output Max", Continuous, 1);
}
