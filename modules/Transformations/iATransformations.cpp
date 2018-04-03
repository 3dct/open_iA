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
#include "pch.h"
#include "iATransformations.h"

#include "defines.h"    // for DIM
#include "iAConnector.h"
#include "iAProgress.h"
#include "mdichild.h"

#include <itkResampleImageFilter.h>
#include <itkAffineTransform.h>
//#include <itkChangeInformationImageFilter.h>
#include <itkPermuteAxesImageFilter.h>
#include <itkFlipImageFilter.h>

#include "iATypedCallHelper.h"

template <class TImageType>
static typename TImageType::PointType image_center(TImageType * image)
{
	typename TImageType::PointType origin = image->GetOrigin();
	typename TImageType::SizeType size = image->GetLargestPossibleRegion().GetSize();
	typename TImageType::SpacingType spacing = image->GetSpacing();
	typename TImageType::PointType center = origin;

	for (int k = 0; k < DIM; k++)
		center[k] += (spacing[k] * size[k]) / 2.0;
	return center;
}

template < class TImageType >
static typename TImageType::PointType center_image(TImageType * image, typename TImageType::PointType * oldOrigin = NULL)
{
	if (oldOrigin != NULL)
		*oldOrigin = image->GetOrigin();
	typename TImageType::PointType center = image_center<TImageType>(image);
	image->SetOrigin(center);
	return center;
}

template<class TPixelType> void flip(iAFilter* filter, QString const & axis)
{
	typedef itk::Image<TPixelType, DIM>         	ImageType;
	typedef itk::FlipImageFilter<ImageType>			FilterType;

	auto flipFilter = FilterType::New();
	typename FilterType::FlipAxesArrayType flip;
	typename ImageType::PointType origin;
	center_image<ImageType>(dynamic_cast<ImageType *>(filter->Input()[0]->GetITKImage()), &origin);
	flipFilter->SetInput(dynamic_cast<ImageType *>(filter->Input()[0]->GetITKImage()));
	flip[0] = (axis == "X");
	flip[1] = (axis == "Y");
	flip[2] = (axis == "Z");
	flipFilter->SetFlipAxes(flip);
	filter->Progress()->Observe(flipFilter);
	flipFilter->Update();
	ImageType * outImage = flipFilter->GetOutput();
	outImage->SetOrigin(origin);
	filter->AddOutput(outImage);
}

void iAFlipAxis::PerformWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(flip, InputPixelType(), this, parameters["Flip axis"].toString());
}

IAFILTER_CREATE(iAFlipAxis)

iAFlipAxis::iAFlipAxis() :
	iAFilter("Flip Axis", "Transformations",
		"Flip the image across one of the three coordinate axes.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1FlipImageFilter.html\">"
		"Flip Filter</a> in the ITK documentation.")
{
	QStringList flipAxis = { "X", "Y", "Z" };
	AddParameter("Flip axis", Categorical, flipAxis);
}



template<class TPixelType, class TPrecision>
static void affine(iAFilter* filter, itk::AffineTransform<TPrecision, DIM> * transform)
{
	typedef itk::Image<TPixelType, DIM>         			ImageType;
	typedef itk::ResampleImageFilter<ImageType, ImageType, TPrecision>	FilterType;

	ImageType * inpImage = dynamic_cast<ImageType *>(filter->Input()[0]->GetITKImage());
	auto inpOrigin = inpImage->GetOrigin();
	auto inpSize = inpImage->GetLargestPossibleRegion().GetSize();
	auto inpSpacing = inpImage->GetSpacing();

	auto resample = FilterType::New();
	resample->SetSize(inpSize);
	resample->SetOutputSpacing(inpSpacing);
	resample->SetOutputOrigin(inpOrigin);
	resample->SetOutputDirection(inpImage->GetDirection());
	resample->SetInput(inpImage);
	resample->SetTransform(transform);
	filter->Progress()->Observe(resample);
	resample->Update();
	filter->AddOutput(resample->GetOutput());
}

typedef double TPrecision;

template<class TPixelType>
static void rotate(iAFilter* filter, QMap<QString, QVariant> const & parameters)
{
	typedef itk::Image<TPixelType, DIM> ImageType;
	typedef itk::AffineTransform<TPrecision, DIM> TransformType;

	auto transform = TransformType::New();
	typename ImageType::PointType center;
	typename TransformType::OutputVectorType rotation;
	typename TransformType::OutputVectorType translation1;
	typename TransformType::OutputVectorType translation2;
	typename TransformType::OutputVectorType rotationAxis;

	//setup rotation axis
	rotationAxis[0] = parameters["Rotation axis"].toString() == "Rotation along X" ? 1 : 0;
	rotationAxis[1] = parameters["Rotation axis"].toString() == "Rotation along Y" ? 1 : 0;
	rotationAxis[2] = parameters["Rotation axis"].toString() == "Rotation along Z" ? 1 : 0;

	//get rotation center
	ImageType * inpImage = dynamic_cast<ImageType *>(filter->Input()[0]->GetITKImage());
	if (parameters["Rotation center"] == "Image center")
	{
		center = image_center(inpImage);
	}
	else if (parameters["Rotation center"] == "Origin")
	{
		center = inpImage->GetOrigin();
	}
	else // == "Specify coordinate"
	{
		center[0] = parameters["Center X"].toDouble();
		center[1] = parameters["Center Y"].toDouble();
		center[2] = parameters["Center Z"].toDouble();
	}
	for (int k = 0; k < DIM; k++)
	{
		translation1[k] = -center[k];
		translation2[k] =  center[k];
	}
	transform->Translate(translation1);
	transform->Rotate3D(rotationAxis, (parameters["Rotation angle"].toDouble() * vnl_math::pi / 180.0), false);
	transform->Translate(translation2);
	affine<TPixelType, TPrecision>(filter, transform);
}

void iARotate::PerformWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(rotate, InputPixelType(), this, parameters);
}

IAFILTER_CREATE(iARotate)

iARotate::iARotate() :
	iAFilter("Rotate", "Transformations",
		"Rotate the image around one of the three coordinate axes.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1AffineTransform.html\">"
		"Affine Transform</a> and the <a href=\""
		"https://itk.org/Doxygen/html/classitk_1_1ResampleImageFilter.html\">"
		"Resample Image Filter</a> in the ITK documentation.")
{
	AddParameter("Rotation angle", Continuous, 0.0, 0.0, 360.0);
	QStringList rotAxes = QStringList() << "Rotation along X" << "Rotation along Y" << "Rotation along Z";
	AddParameter("Rotation axis", Categorical, rotAxes);
	QStringList rotCenter = QStringList() << "Image center" << "Origin" << "Specify coordinate";
	AddParameter("Rotation center", Categorical, rotCenter);
	AddParameter("Center X", Continuous, 0);
	AddParameter("Center Y", Continuous, 0);
	AddParameter("Center Z", Continuous, 0);
}



template<class TPixelType>
static void translate(iAFilter* filter, QMap<QString, QVariant> const & parameters)
{
	typedef itk::AffineTransform<TPrecision, DIM> TransformType;
	auto transform = TransformType::New();
	typename TransformType::OutputVectorType translation;
	translation[0] = parameters["Translate X"].toDouble();
	translation[1] = parameters["Translate Y"].toDouble();
	translation[2] = parameters["Translate Z"].toDouble();
	transform->Translate(translation);
	affine<TPixelType, TPrecision>(filter, transform);
}

void iATranslate::PerformWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(translate, InputPixelType(), this, parameters);
}

IAFILTER_CREATE(iATranslate)

iATranslate::iATranslate() :
	iAFilter("Translate", "Transformations",
		"Translate the image.<br/>"
		".<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1AffineTransform.html\">"
		"Affine Transform</a> and the <a href=\""
		"https://itk.org/Doxygen/html/classitk_1_1ResampleImageFilter.html\">"
		"Resample Image Filter</a> in the ITK documentation.")
{
	AddParameter("Translate X", Continuous, 0);
	AddParameter("Translate Y", Continuous, 0);
	AddParameter("Translate Z", Continuous, 0);
}


template<class TPixelType> void permute(iAFilter* filter, QString  const & orderStr)
{
	typedef itk::Image<TPixelType, DIM>         			ImageType;
	typedef itk::PermuteAxesImageFilter<ImageType>			FilterType;

	auto permFilter = FilterType::New();
	typename FilterType::PermuteOrderArrayType order;
	permFilter->SetInput(dynamic_cast<ImageType *>(filter->Input()[0]->GetITKImage()));
	for (int k = 0; k < 3; k++)
	{
		char axes = orderStr.at(k).toUpper().toLatin1();
		order[k] = axes - QChar('X').toLatin1();
	}
	permFilter->SetOrder(order);
	filter->Progress()->Observe(permFilter);
	permFilter->Update();
	filter->AddOutput(permFilter->GetOutput());
}

void iAPermuteAxes::PerformWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(permute, InputPixelType(), this, parameters["Order"].toString());
}

IAFILTER_CREATE(iAPermuteAxes)

iAPermuteAxes::iAPermuteAxes() :
	iAFilter("Permute Axes", "Transformations",
		"Permutes the image axes according to a user specified order.<br/>"
		"The i-th axis of the output image corresponds with the order[i]-th "
		"axis of the input image.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1PermuteAxesImageFilter.html\">"
		"Permute Axes Filter</a> in the ITK documentation.")
{
	QStringList permutationOrder = QStringList() << "XZY" << "YXZ" << "YZX" << "ZXY" << "ZYX";
	AddParameter("Order", Categorical, permutationOrder);
}
