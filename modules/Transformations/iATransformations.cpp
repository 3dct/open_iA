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
#include "iATransformations.h"

#include "iAConnector.h"
#include "iAProgress.h"
#include "mdichild.h"

#include <itkResampleImageFilter.h>
#include <itkAffineTransform.h>
#include <itkChangeInformationImageFilter.h>
#include <itkPermuteAxesImageFilter.h>
#include <itkFlipImageFilter.h>
#include <itkImageRegion.h>

#include <vtkImageData.h>

#include "iATypedCallHelper.h"

template <class TImageType>
static typename TImageType::PointType image_center(TImageType * image)
{
	typename TImageType::PointType origin = image->GetOrigin();
	typename TImageType::SizeType size = image->GetLargestPossibleRegion().GetSize();
	typename TImageType::SpacingType spacing = image->GetSpacing();
	typename TImageType::PointType center = origin;

	for (int k = 0; k < TImageType::ImageDimension; k++)
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

template<class TPixelType, class TPrecision>
static void flip_template(iATransformations * caller)
{
	const int Dim = iAConnector::ImageBaseType::ImageDimension;
	typedef itk::Image<TPixelType, Dim>         	ImageType;
	typedef itk::FlipImageFilter<ImageType>			FilterType;

	ImageType * inpImage = dynamic_cast<ImageType *>(caller->getConnector()->GetITKImage());
	typename FilterType::Pointer filter = FilterType::New();
	typename FilterType::FlipAxesArrayType flip;

	//center image
	typename ImageType::PointType origin;
	center_image<ImageType>(inpImage, &origin);

	filter->SetInput(inpImage);
	flip[0] = caller->getFlipAxes() == iATransformations::FlipAxesX;
	flip[1] = caller->getFlipAxes() == iATransformations::FlipAxesY;
	flip[2] = caller->getFlipAxes() == iATransformations::FlipAxesZ;
	filter->SetFlipAxes(flip);

	//run pipeline
	caller->getItkProgress()->Observe(filter);
	filter->Update();

	ImageType * outImage = filter->GetOutput();
	outImage->SetOrigin(origin);
	caller->getConnector()->SetImage(outImage);
	caller->getConnector()->Modified();

	filter->ReleaseDataFlagOn();
}

template<class TPixelType, class TPrecision>
static void affine_template(iAProgress * progress, iAConnector* connector,
	itk::AffineTransform<TPrecision, iAConnector::ImageBaseType::ImageDimension> * transform)
{
	const int Dim = iAConnector::ImageBaseType::ImageDimension;
	typedef itk::Image<TPixelType, Dim>         			ImageType;
	typedef itk::ResampleImageFilter<ImageType, ImageType, TPrecision>	FilterType;

	ImageType * inpImage = dynamic_cast<ImageType *>(connector->GetITKImage());
	typename ImageType::PointType inpOrigin = inpImage->GetOrigin();
	typename ImageType::SizeType inpSize = inpImage->GetLargestPossibleRegion().GetSize();
	typename ImageType::SpacingType inpSpacing = inpImage->GetSpacing();

	auto resample = FilterType::New();
	resample->SetSize(inpSize);
	resample->SetOutputSpacing(inpSpacing);
	resample->SetOutputOrigin(inpOrigin);
	resample->SetOutputDirection(inpImage->GetDirection());
	resample->SetInput(inpImage);
	resample->SetTransform(transform);
	progress->Observe(resample);
	resample->Update();
	connector->SetImage(resample->GetOutput());
	connector->Modified();
	resample->ReleaseDataFlagOn();
}

typedef double TPrecision;

template<class TPixelType>
static void rotate_template(QMap<QString, QVariant> const & parameters, iAProgress* progress, iAConnector* connector)
{
	const int Dim = iAConnector::ImageBaseType::ImageDimension;
	typedef itk::Image<TPixelType, Dim>         			ImageType;
	typedef itk::AffineTransform<TPrecision, Dim>			TransformType;
	
	//set center of transformation
	typename TransformType::Pointer transform = TransformType::New();
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
	ImageType * inpImage = dynamic_cast<ImageType *>(connector->GetITKImage());
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
	for (int k = 0; k < Dim; k++)
	{
		translation1[k] = -center[k];
		translation2[k] =  center[k];
	}
	transform->Translate(translation1);
	transform->Rotate3D(rotationAxis, (parameters["Rotation angle"].toDouble() * vnl_math::pi / 180.0), false);
	transform->Translate(translation2);
	affine_template<TPixelType, TPrecision>(progress, connector, transform);
}

void iARotate::Run(QMap<QString, QVariant> const & parameters)
{
	iAConnector::ITKScalarPixelType pixelType = m_con->GetITKScalarPixelType();
	ITK_TYPED_CALL(rotate_template, pixelType, parameters, m_progress, m_con);
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

template<class TPixelType, class TPrecision>
static void translate_template(iATransformations * caller)
{
	const int Dim = iAConnector::ImageBaseType::ImageDimension;
	typedef itk::AffineTransform<TPrecision, Dim>			TransformType;

	//set translation
	typename TransformType::Pointer transform = TransformType::New();
	typename TransformType::OutputVectorType translation;
	for (int k = 0; k < Dim; k++)
		translation[k] = caller->getTranslation()[k];
	transform->Translate(translation);

	affine_template<TPixelType, TPrecision>(caller->getItkProgress(), caller->getConnector(), transform);
}

template<class TPixelType, class TPrecision>
static void permute_template(iATransformations * caller)
{
	const int Dim = iAConnector::ImageBaseType::ImageDimension;
	typedef itk::Image<TPixelType, Dim>         			ImageType;
	typedef itk::PermuteAxesImageFilter<ImageType>			FilterType;
	
	ImageType * inpImage = dynamic_cast<ImageType *>(caller->getConnector()->GetITKImage());
	typename FilterType::Pointer filter = FilterType::New();
	typename FilterType::PermuteOrderArrayType order;
	
	//order XYY --> ZXY: 2, 0, 1
	const int * porder = caller->getPermuteAxesOrder();
	filter->SetInput(inpImage);
	order[0] = porder[0];
	order[1] = porder[1];
	order[2] = porder[2];
	filter->SetOrder(order);

	//run pipeline
	caller->getItkProgress()->Observe(filter);
	filter->Update();

	caller->getConnector()->SetImage(filter->GetOutput());
	caller->getConnector()->Modified();

	filter->ReleaseDataFlagOn();
}

template <class TPixelType, class TPrecision>
static void transform_template(iATransformations * caller)
{
	auto type = caller->getTransformationType();
	switch (type)
	{
	case iATransformations::Translation:
		translate_template<TPixelType, TPrecision>(caller);
	case iATransformations::Flip:
		flip_template<TPixelType, TPrecision>(caller);
	case iATransformations::PermuteAxes:
		permute_template<TPixelType, TPrecision>(caller);
	}
}

iATransformations::iATransformations( QString fn, vtkImageData* i, vtkPolyData* p, iALogger* logger, QObject *parent )
	: iAAlgorithm( fn, i, p, logger, parent )
{
	for (int k = 0; k < Dim; k++)
	{
		m_permuteOrder[k] = k;
		m_rotCenterCoord[k] = 0.0;
		m_translation[k] = 0.0;
	}
	m_rotAngle = 0;
	m_transType = iATransformations::Unknown;
	m_flipAxesType = iATransformations::FlipAxesNone;
}

iATransformations::TransformationType iATransformations::getTransformationType() const
{
	return m_transType;
}
void iATransformations::setTransformationType(TransformationType transType)
{
	m_transType = transType;
}
iATransformations::FlipAxesType iATransformations::getFlipAxes() const
{
	return m_flipAxesType;
}
void iATransformations::setFlipAxes(FlipAxesType axes)
{
	m_flipAxesType = axes;
}
void iATransformations::setFlipAxes(const QChar & axes)
{
	QChar ax = axes.toUpper();
	if (ax == QChar('X'))
		m_flipAxesType = FlipAxesX;
	else if (ax == QChar('Y'))
		m_flipAxesType = FlipAxesY;
	else if (ax == QChar('Z'))
		m_flipAxesType = FlipAxesZ;
	else
		m_flipAxesType = FlipAxesNone;
}
const qreal * iATransformations::getTranslation() const
{
	return &m_translation[0];
}
void iATransformations::setTranslation(qreal tx, qreal ty, qreal tz)
{
	m_translation[0] = tx;
	m_translation[1] = ty;
	m_translation[2] = tz;
}

const int * iATransformations::getPermuteAxesOrder() const
{
	return &m_permuteOrder[0];
}
void iATransformations::setPermuteAxesOrder(int ox, int oy, int oz)
{
	m_permuteOrder[0] = ox;
	m_permuteOrder[1] = oy;
	m_permuteOrder[2] = oz;
}

void iATransformations::setPermuteAxesOrder(const QString &order)
{
	if (order.size() >= 3)
	{
		for (int k = 0; k < 3; k++) {
			char axes = order.at(k).toUpper().toLatin1();
			m_permuteOrder[k] = axes - QChar('X').toLatin1();			
		}
	}
}

void iATransformations::performWork()
{
	switch (getConnector()->GetVTKImage()->GetScalarType()) // This filter handles all types
	{
	case VTK_UNSIGNED_CHAR:
		transform_template<unsigned char, double>(this); break;
	case VTK_CHAR:
		transform_template<char, double>(this); break;
	case VTK_UNSIGNED_SHORT:
		transform_template<unsigned short, double>(this); break;
	case VTK_SHORT:
		transform_template<short, double>(this); break;
	case VTK_UNSIGNED_INT:
		transform_template<unsigned int, double>(this);  break;
	case VTK_INT:
		transform_template<int, double>(this); break;
	case VTK_UNSIGNED_LONG:
		transform_template<unsigned long, double>(this); break;
	case VTK_LONG:
		transform_template<long, double>(this); break;
	case VTK_FLOAT:
		transform_template<float, double>(this); break;
	case VTK_DOUBLE:
		transform_template<double, double>(this); break;
	default:
		addMsg(tr("  unknown component type"));
		return;
	}
}
