/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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
#include "iATransformations.h"

#include <defines.h>    // for DIM
#include <iADataSet.h>
#include <iAProgress.h>
#include <iATypedCallHelper.h>

#include <itkResampleImageFilter.h>
#include <itkAffineTransform.h>
#include <itkPermuteAxesImageFilter.h>
#include <itkFlipImageFilter.h>

template <class TImageType>
static typename TImageType::PointType image_center(TImageType * image)
{
	typename TImageType::PointType origin = image->GetOrigin();
	typename TImageType::SizeType size = image->GetLargestPossibleRegion().GetSize();
	typename TImageType::SpacingType spacing = image->GetSpacing();
	typename TImageType::PointType center = origin;

	for (int k = 0; k < DIM; k++)
	{
		center[k] += (spacing[k] * size[k]) / 2.0;
	}
	return center;
}

template < class TImageType >
static typename TImageType::PointType center_image(TImageType * image, typename TImageType::PointType * oldOrigin = nullptr)
{
	if (oldOrigin != nullptr)
	{
		*oldOrigin = image->GetOrigin();
	}
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
	center_image<ImageType>(dynamic_cast<ImageType *>(filter->imageInput(0)->itkImage()), &origin);
	flipFilter->SetInput(dynamic_cast<ImageType *>(filter->imageInput(0)->itkImage()));
	flip[0] = (axis == "X");
	flip[1] = (axis == "Y");
	flip[2] = (axis == "Z");
	flipFilter->SetFlipAxes(flip);
	filter->progress()->observe(flipFilter);
	flipFilter->Update();
	ImageType * outImage = flipFilter->GetOutput();
	outImage->SetOrigin(origin);
	filter->addOutput(outImage);
}

void iAFlipAxis::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(flip, inputPixelType(), this, parameters["Flip axis"].toString());
}

iAFlipAxis::iAFlipAxis() :
	iAFilter("Flip Axis", "Geometric Transformations",
		"Flip the image across one of the three coordinate axes.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1FlipImageFilter.html\">"
		"Flip Filter</a> in the ITK documentation.")
{
	QStringList flipAxis = { "X", "Y", "Z" };
	addParameter("Flip axis", iAValueType::Categorical, flipAxis);
}



template<class TPixelType, class TPrecision>
static void affine(iAFilter* filter, itk::AffineTransform<TPrecision, DIM> * transform)
{
	typedef itk::Image<TPixelType, DIM>         			ImageType;
	typedef itk::ResampleImageFilter<ImageType, ImageType, TPrecision>	FilterType;

	ImageType * inpImage = dynamic_cast<ImageType *>(filter->imageInput(0)->itkImage());
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
	filter->progress()->observe(resample);
	resample->Update();
	filter->addOutput(resample->GetOutput());
}

template <class TPixelType>
void permute(iAFilter* filter, QString const& orderStr)
{
	typedef itk::Image<TPixelType, DIM> ImageType;
	typedef itk::PermuteAxesImageFilter<ImageType> FilterType;

	auto permFilter = FilterType::New();
	typename FilterType::PermuteOrderArrayType order;
	permFilter->SetInput(dynamic_cast<ImageType*>(filter->imageInput(0)->itkImage()));
	for (int k = 0; k < 3; k++)
	{
		char axes = orderStr.at(k).toUpper().toLatin1();
		order[k] = axes - QChar('X').toLatin1();
	}
	permFilter->SetOrder(order);
	filter->progress()->observe(permFilter);
	permFilter->Update();
	filter->addOutput(permFilter->GetOutput());
}

void iAPermuteAxes::performWork(QVariantMap const& parameters)
{
	ITK_TYPED_CALL(permute, inputPixelType(), this, parameters["Order"].toString());
}

iAPermuteAxes::iAPermuteAxes() :
	iAFilter("Permute Axes", "Geometric Transformations",
		"Permutes the image axes according to a user specified order.<br/>"
		"The i-th axis of the output image corresponds with the order[i]-th "
		"axis of the input image.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1PermuteAxesImageFilter.html\">"
		"Permute Axes Filter</a> in the ITK documentation.")
{
	QStringList permutationOrder = QStringList() << "XZY"
												 << "YXZ"
												 << "YZX"
												 << "ZXY"
												 << "ZYX";
	addParameter("Order", iAValueType::Categorical, permutationOrder);
}


typedef double TPrecision;

template<class TPixelType>
static void rotate(iAFilter* filter, QVariantMap const & parameters)
{
	typedef itk::Image<TPixelType, DIM> ImageType;
	typedef itk::AffineTransform<TPrecision, DIM> TransformType;

	auto transform = TransformType::New();
	typename ImageType::PointType center;
	typename TransformType::OutputVectorType translation1;
	typename TransformType::OutputVectorType translation2;
	typename TransformType::OutputVectorType rotationAxis;

	//setup rotation axis
	rotationAxis[0] = parameters["Rotation axis"].toString() == "Rotation along X" ? 1 : 0;
	rotationAxis[1] = parameters["Rotation axis"].toString() == "Rotation along Y" ? 1 : 0;
	rotationAxis[2] = parameters["Rotation axis"].toString() == "Rotation along Z" ? 1 : 0;

	//get rotation center
	ImageType * inpImage = dynamic_cast<ImageType *>(filter->imageInput(0)->itkImage());
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

void iARotate::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(rotate, inputPixelType(), this, parameters);
}

iARotate::iARotate() :
	iAFilter("Rotate", "Geometric Transformations",
		"Rotate the image around one of the three coordinate axes.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1AffineTransform.html\">"
		"Affine Transform</a> and the <a href=\""
		"https://itk.org/Doxygen/html/classitk_1_1ResampleImageFilter.html\">"
		"Resample Image Filter</a> in the ITK documentation.")
{
	addParameter("Rotation angle", iAValueType::Continuous, 0.0, 0.0, 360.0);
	QStringList rotAxes = QStringList() << "Rotation along X" << "Rotation along Y" << "Rotation along Z";
	addParameter("Rotation axis", iAValueType::Categorical, rotAxes);
	QStringList rotCenter = QStringList() << "Image center" << "Origin" << "Specify coordinate";
	addParameter("Rotation center", iAValueType::Categorical, rotCenter);
	addParameter("Center X", iAValueType::Continuous, 0);
	addParameter("Center Y", iAValueType::Continuous, 0);
	addParameter("Center Z", iAValueType::Continuous, 0);
}



template<class TPixelType>
static void translate(iAFilter* filter, QVariantMap const & parameters)
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

void iATranslate::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(translate, inputPixelType(), this, parameters);
}

iATranslate::iATranslate() :
	iAFilter("Translate", "Geometric Transformations",
		"Translate the image.<br/>"
		".<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1AffineTransform.html\">"
		"Affine Transform</a> and the <a href=\""
		"https://itk.org/Doxygen/html/classitk_1_1ResampleImageFilter.html\">"
		"Resample Image Filter</a> in the ITK documentation.")
{
	addParameter("Translate X", iAValueType::Continuous, 0);
	addParameter("Translate Y", iAValueType::Continuous, 0);
	addParameter("Translate Z", iAValueType::Continuous, 0);
}
