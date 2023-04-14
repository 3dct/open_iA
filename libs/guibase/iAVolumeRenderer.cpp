// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAVolumeRenderer.h"

#include <iAAABB.h>
#include <iAToolsVTK.h>
#include <iATransferFunction.h>
#include <iAValueTypeVectorHelpers.h>
#include <iAVolumeSettings.h>

#include <iAMainWindow.h>    // for default volume settings

#include <vtkCallbackCommand.h>
#include <vtkImageData.h>
#include <vtkRenderer.h>
#include <vtkSmartVolumeMapper.h>
#include <vtkVersion.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>

const QString iAVolumeRenderer::Interpolation("Interpolation");
const QString iAVolumeRenderer::ScalarOpacityUnitDistance("Scalar Opacity Unit Distance");
const QString iAVolumeRenderer::RendererType("Renderer type");
const QString iAVolumeRenderer::SampleDistance("Sample distance");

namespace
{
	const QString Spacing = "Spacing";
	const QString InteractiveAdjustSampleDistance = "Interactively Adjust Sample Distances";
	const QString AutoAdjustSampleDistance = "Auto-Adjust Sample Distances";
	const QString InteractiveUpdateRate = "Interactive Update Rate";
	const QString FinalColorLevel = "Final Color Level";
	const QString FinalColorWindow = "Final Color Window";
#if VTK_VERSION_NUMBER >= VTK_VERSION_CHECK(9, 2, 0)
	const QString GlobalIlluminationReach = "Global Illumination Reach";
	const QString VolumetricScatteringBlending = "Volumetric Scattering Blending";
#endif
	const QString InterpolateNearest = "Nearest";
	const QString InterpolateLinear = "Linear";

	int string2VtkVolInterpolationType(QString const & interpType)
	{
		return (interpType == InterpolateNearest)
			? VTK_NEAREST_INTERPOLATION
			: VTK_LINEAR_INTERPOLATION;
	
	}
}

iAVolumeRenderer::iAVolumeRenderer(vtkRenderer* renderer, vtkImageData* vtkImg, iATransferFunction* tf) :
	iADataSetRenderer(renderer),
	m_volume(vtkSmartPointer<vtkVolume>::New()),
	m_volProp(vtkSmartPointer<vtkVolumeProperty>::New()),
	m_volMapper(vtkSmartPointer<vtkSmartVolumeMapper>::New()),
	m_image(vtkImg)
{
	m_volMapper->SetBlendModeToComposite();
	m_volume->SetMapper(m_volMapper);
	m_volume->SetProperty(m_volProp);
	m_volume->SetVisibility(true);
	m_volMapper->SetInputData(vtkImg);
	if (vtkImg->GetNumberOfScalarComponents() > 1)
	{
		m_volMapper->SetBlendModeToComposite();
		m_volProp->IndependentComponentsOff();
	}
	else
	{
		m_volProp->SetColor(0, tf->colorTF());
	}
	m_volProp->SetScalarOpacity(0, tf->opacityTF());
	m_volProp->Modified();

	// properties specific to volumes:
	auto volumeSettings = iAMainWindow::get()->defaultVolumeSettings();
	QStringList volInterpolationTypes = QStringList() << InterpolateNearest << InterpolateLinear;
	selectOption(volInterpolationTypes, volumeSettings.LinearInterpolation ? InterpolateLinear : InterpolateNearest);
	addAttribute(Interpolation, iAValueType::Categorical, volInterpolationTypes);
	addAttribute(Shading, iAValueType::Boolean, volumeSettings.Shading);
	addAttribute(ScalarOpacityUnitDistance, iAValueType::Continuous, volumeSettings.ScalarOpacityUnitDistance);

	// mapper properties:
	QStringList renderTypes = RenderModeMap().values();
	selectOption(renderTypes, renderTypes[volumeSettings.RenderMode]);
	addAttribute(RendererType, iAValueType::Categorical, renderTypes);
	addAttribute(InteractiveAdjustSampleDistance, iAValueType::Boolean, true);   // maybe only enable for large datasets?
	addAttribute(AutoAdjustSampleDistance, iAValueType::Boolean, false);
	addAttribute(SampleDistance, iAValueType::Continuous, volumeSettings.SampleDistance);
	addAttribute(InteractiveUpdateRate, iAValueType::Continuous, 1.0);
	addAttribute(FinalColorLevel, iAValueType::Continuous, 0.5);
	addAttribute(FinalColorWindow, iAValueType::Continuous, 1.0);
#if VTK_VERSION_NUMBER >= VTK_VERSION_CHECK(9, 2, 0)
	addAttribute(GlobalIlluminationReach, iAValueType::Continuous, 0.0, 0.0, 1.0);
	addAttribute(VolumetricScatteringBlending, iAValueType::Continuous, -1.0, 0.0, 2.0);
#endif

	// volume properties:
	auto spc = vtkImg->GetSpacing();
	addAttribute(Spacing, iAValueType::Vector3, QVariant::fromValue(QVector<double>({ spc[0], spc[1], spc[2] })));

	// adapt bounding box to changes in position/orientation of volume:
	vtkNew<vtkCallbackCommand> modifiedCallback;
	modifiedCallback->SetCallback(
		[](vtkObject* vtkNotUsed(caller), long unsigned int vtkNotUsed(eventId), void* clientData,
			void* vtkNotUsed(callData))
		{
			reinterpret_cast<iAVolumeRenderer*>(clientData)->updateOutlineTransform();
		});
	modifiedCallback->SetClientData(this);
	m_volume->AddObserver(vtkCommand::ModifiedEvent, modifiedCallback);
}

iAVolumeRenderer::~iAVolumeRenderer()
{
	if (isVisible())
	{
		hideDataSet();
	}
}

void iAVolumeRenderer::showDataSet()
{
	m_renderer->AddVolume(m_volume);
}

void iAVolumeRenderer::hideDataSet()
{
	m_renderer->RemoveVolume(m_volume);
}

void iAVolumeRenderer::applyAttributes(QVariantMap const& values)
{
	applyLightingProperties(m_volProp.Get(), values);
	m_volProp->SetInterpolationType(string2VtkVolInterpolationType(values[Interpolation].toString()));
	m_volProp->SetShade(values[Shading].toBool());
	if (values[ScalarOpacityUnitDistance].toDouble() > 0)
	{
		m_volProp->SetScalarOpacityUnitDistance(values[ScalarOpacityUnitDistance].toDouble());
	}
	/*
	else
	{
		m_volSettings.ScalarOpacityUnitDistance = m_volProp->GetScalarOpacityUnitDistance();
	}
	*/
	m_volMapper->SetRequestedRenderMode(values[RendererType].toInt());
	m_volMapper->SetInteractiveAdjustSampleDistances(values[InteractiveAdjustSampleDistance].toBool());
	m_volMapper->SetAutoAdjustSampleDistances(values[AutoAdjustSampleDistance].toBool());
	m_volMapper->SetSampleDistance(values[SampleDistance].toDouble());
	m_volMapper->SetInteractiveUpdateRate(values[InteractiveUpdateRate].toDouble());
	m_volMapper->SetFinalColorLevel(values[FinalColorLevel].toDouble());
	m_volMapper->SetFinalColorWindow(values[FinalColorWindow].toDouble());
#if VTK_VERSION_NUMBER >= VTK_VERSION_CHECK(9, 2, 0)
	m_volMapper->SetGlobalIlluminationReach(values[GlobalIlluminationReach].toFloat());
	m_volMapper->SetVolumetricScatteringBlending(values[VolumetricScatteringBlending].toFloat());
#endif

	auto pos = values[Position].value<QVector<double>>();
	auto ori = values[Orientation].value<QVector<double>>();
	if (pos.size() == 3)
	{
		m_volume->SetPosition(pos.data());
	}
	if (ori.size() == 3)
	{
		m_volume->SetOrientation(ori.data());
	}
	m_volume->SetPickable(values[Pickable].toBool());

	auto spc = values[Spacing].value<QVector<double>>();
	if (spc.size() == 3)
	{
		m_image->SetSpacing(spc.data());
	}
}

iAAABB iAVolumeRenderer::bounds()
{
	return iAAABB(m_image->GetBounds());
}

double const* iAVolumeRenderer::orientation() const
{
	return m_volume->GetOrientation();
}

double const* iAVolumeRenderer::position() const
{
	return m_volume->GetPosition();
}

void iAVolumeRenderer::setPosition(double pos[3])
{
	m_volume->SetPosition(pos);
}

void iAVolumeRenderer::setOrientation(double ori[3])
{
	m_volume->SetOrientation(ori);
}

vtkProp3D* iAVolumeRenderer::vtkProp()
{
	return m_volume;
}

void iAVolumeRenderer::setCuttingPlanes(vtkPlane* p1, vtkPlane* p2, vtkPlane* p3)
{
	m_volMapper->AddClippingPlane(p1);
	m_volMapper->AddClippingPlane(p2);
	m_volMapper->AddClippingPlane(p3);
}

void iAVolumeRenderer::removeCuttingPlanes()
{
	m_volMapper->RemoveAllClippingPlanes();
}

QVariantMap iAVolumeRenderer::attributeValues() const
{
	QVariantMap result = iADataSetRenderer::attributeValues();
	auto spc = m_image->GetSpacing();
	result[Spacing] = variantVector<double>({ spc[0], spc[1], spc[2] });
	return result;
}


// iAVolumeSettings implementation

iAVolumeSettings::iAVolumeSettings() :
	Shading(true),
	AmbientLighting(0.2),
	DiffuseLighting(0.5),
	SpecularLighting(0.7),
	SpecularPower(10.0),
	LinearInterpolation(true),
	SampleDistance(1.0),
	ScalarOpacityUnitDistance(-1.0),
	RenderMode(0) // 0 = DefaultRenderMode
{}
QVariantMap iAVolumeSettings::toMap() const
{
	QVariantMap result;
	result[iADataSetRenderer::Shading] = Shading;
	result[iADataSetRenderer::AmbientLighting] = AmbientLighting;
	result[iADataSetRenderer::DiffuseLighting] = DiffuseLighting;
	result[iADataSetRenderer::SpecularLighting] = SpecularLighting;
	result[iADataSetRenderer::SpecularPower] = SpecularPower;

	result[iAVolumeRenderer::Interpolation] = LinearInterpolation ? InterpolateLinear : InterpolateNearest;
	result[iAVolumeRenderer::ScalarOpacityUnitDistance] = ScalarOpacityUnitDistance;
	result[iAVolumeRenderer::RendererType] = RenderMode;
	result[iAVolumeRenderer::SampleDistance] = SampleDistance;

	return result;
}