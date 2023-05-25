// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAVolumeRenderer.h"

#include <iAAABB.h>
#include <iAToolsVTK.h>
#include <iATransferFunction.h>
#include <iAValueTypeVectorHelpers.h>

#include <iAMainWindow.h>    // for default volume settings

#include <vtkCallbackCommand.h>
#include <vtkImageData.h>
#include <vtkRenderer.h>
#include <vtkSmartVolumeMapper.h>
#include <vtkVersion.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>

namespace
{
	constexpr const char* Spacing = "Spacing";
	constexpr const char* FinalColorLevel = "Final Color Level";
	constexpr const char* FinalColorWindow = "Final Color Window";
	constexpr const char* InterpolateNearest = "Nearest";
	constexpr const char* InterpolateLinear = "Linear";
#if VTK_VERSION_NUMBER >= VTK_VERSION_CHECK(9, 2, 0)
	constexpr const char* GlobalIlluminationReach = "Global Illumination Reach";
	constexpr const char* VolumetricScatteringBlending = "Volumetric Scattering Blending";
#endif
}

constexpr const char VolumeRendererName[] = "Default Settings/Volume Renderer";
class iAguibase_API iAVolumeRendererSettings : iASettingsObject<VolumeRendererName, iAVolumeRendererSettings>
{
public:
	static iAAttributes& defaultAttributes() {
		static iAAttributes attr;
		if (attr.isEmpty())
		{
			attr = cloneAttributes(iADataSetRenderer::defaultAttributes());
			// volumes properties:
			QStringList volInterpolationTypes = QStringList() << InterpolateNearest << InterpolateLinear;
			selectOption(volInterpolationTypes, InterpolateLinear);
			addAttr(attr, iAVolumeRenderer::Interpolation, iAValueType::Categorical, volInterpolationTypes);
			addAttr(attr, iAVolumeRenderer::Shading, iAValueType::Boolean, true);
			addAttr(attr, iAVolumeRenderer::ScalarOpacityUnitDistance, iAValueType::Continuous, -1.0);

			// mapper properties:
			QStringList renderTypes = RenderModeMap().keys();
			selectOption(renderTypes, renderTypes[0]);
			addAttr(attr, iAVolumeRenderer::RendererType, iAValueType::Categorical, renderTypes);
			addAttr(attr, iAVolumeRenderer::InteractiveAdjustSampleDistance, iAValueType::Boolean, true);   // maybe only enable for large datasets?
			addAttr(attr, iAVolumeRenderer::AutoAdjustSampleDistance, iAValueType::Boolean, false);
			addAttr(attr, iAVolumeRenderer::SampleDistance, iAValueType::Continuous, 1.0);
			addAttr(attr, iAVolumeRenderer::InteractiveUpdateRate, iAValueType::Continuous, 1.0);
			addAttr(attr, FinalColorLevel, iAValueType::Continuous, 0.5);
			addAttr(attr, FinalColorWindow, iAValueType::Continuous, 1.0);
#if VTK_VERSION_NUMBER >= VTK_VERSION_CHECK(9, 2, 0)
			addAttr(attr, GlobalIlluminationReach, iAValueType::Continuous, 0.0, 0.0, 1.0);
			addAttr(attr, VolumetricScatteringBlending, iAValueType::Continuous, -1.0, 0.0, 2.0);
#endif
		}
		return attr;
	}
};

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
	applyAttributes(extractValues(defaultAttributes()));
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
	if (!m_renderer)
	{
		return;
	}
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

	auto pos = variantToVector<double>(values[Position]);
	auto ori = variantToVector<double>(values[Orientation]);
	if (pos.size() == 3)
	{
		m_volume->SetPosition(pos.data());
	}
	if (ori.size() == 3)
	{
		m_volume->SetOrientation(ori.data());
	}
	m_volume->SetPickable(values[Pickable].toBool());

	auto spc = variantToVector<double>(values[Spacing]);
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

iAAttributes const& iAVolumeRenderer::attributes() const
{
	static iAAttributes attr;
	if (attr.isEmpty())
	{
		attr = cloneAttributes(defaultAttributes());
		// data-specific property:
		addAttr(attr, Spacing, iAValueType::Vector3, variantVector<double>({1.0, 1.0, 1.0}));
	}
	return attr;
}

iAAttributes& iAVolumeRenderer::defaultAttributes()
{
	static iAAttributes& attr = iAVolumeRendererSettings::defaultAttributes();
	return attr;
}

int iAVolumeRenderer::string2VtkVolInterpolationType(QString const& interpType)
{
	return (interpType == InterpolateNearest)
		? VTK_NEAREST_INTERPOLATION
		: VTK_LINEAR_INTERPOLATION;
}

