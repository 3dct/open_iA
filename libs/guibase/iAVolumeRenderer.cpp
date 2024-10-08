// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAVolumeRenderer.h"

#include <iAAABB.h>
#include <iAToolsVTK.h>
#include <iATransferFunction.h>
#include <iAValueTypeVectorHelpers.h>

#include "iADefaultSettings.h"
#include "iAMainWindow.h"    // for default volume settings

#include <vtkCallbackCommand.h>
#include <vtkImageData.h>
#include <vtkRenderer.h>
#include <vtkSmartVolumeMapper.h>
#include <vtkVersionMacros.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>

namespace
{
	constexpr const char* Spacing = "Spacing";
	constexpr const char* FinalColorLevel = "Final Color Level";
	constexpr const char* FinalColorWindow = "Final Color Window";
#if VTK_VERSION_NUMBER >= VTK_VERSION_CHECK(9, 2, 0)
	constexpr const char* GlobalIlluminationReach = "Global Illumination Reach";
	constexpr const char* VolumetricScatteringBlending = "Volumetric Scattering Blending";
#endif

	constexpr const char* DefaultBlendMode = "Composite";
	QMap<QString, int> const& BlendModeMap()
	{
		static QMap<QString, int> blendModeMap;
		if (blendModeMap.isEmpty())
		{
			blendModeMap.insert(DefaultBlendMode, vtkVolumeMapper::COMPOSITE_BLEND);
			blendModeMap.insert("Maximum Intensity", vtkVolumeMapper::MAXIMUM_INTENSITY_BLEND);
			blendModeMap.insert("Minimum Intensity", vtkVolumeMapper::MINIMUM_INTENSITY_BLEND);
			blendModeMap.insert("Average Intensity", vtkVolumeMapper::AVERAGE_INTENSITY_BLEND);
			blendModeMap.insert("Additive", vtkVolumeMapper::ADDITIVE_BLEND);
			blendModeMap.insert("Isosurface", vtkVolumeMapper::ISOSURFACE_BLEND);
			blendModeMap.insert("Slice", vtkVolumeMapper::SLICE_BLEND);
		}
		return blendModeMap;
	}
}

QMap<QString, int> const& RenderModeMap()
{
	static QMap<QString, int> renderModeMap;
	if (renderModeMap.isEmpty())
	{
		renderModeMap.insert("Default (GPU if available, else Software)", vtkSmartVolumeMapper::DefaultRenderMode);
		renderModeMap.insert("Software Ray-Casting", vtkSmartVolumeMapper::RayCastRenderMode);
		renderModeMap.insert("GPU", vtkSmartVolumeMapper::GPURenderMode);
#if VTK_OSPRAY_AVAILABLE
		renderModeMap.insert("OSPRay", vtkSmartVolumeMapper::OSPRayRenderMode);
#endif
	}
	return renderModeMap;
}

int mapRenderModeToEnum(QString const& modeName)
{
	return RenderModeMap().contains(modeName) ? RenderModeMap()[modeName] : vtkSmartVolumeMapper::DefaultRenderMode;
}

inline constexpr char VolumeRendererName[] = "Default Settings/Dataset Renderer: Volume";
//! Encapsulates the specifics of the settings of a volume renderer.
//! Handles auto-registration of the settings with iASettingsManager (via deriving from iASettingsObject).
class iAVolumeRendererSettings : iASettingsObject<VolumeRendererName, iAVolumeRendererSettings>
{
public:
	static iAAttributes& defaultAttributes() {
		static iAAttributes attr;
		if (attr.isEmpty())
		{
			attr = cloneAttributes(iADataSetRenderer::defaultAttributes());
			// volumes properties:
			QStringList volInterpolationTypes = QStringList() << iAVolumeRenderer::InterpolateNearest << iAVolumeRenderer::InterpolateLinear;
			selectOption(volInterpolationTypes, iAVolumeRenderer::InterpolateLinear);
			addAttr(attr, iAVolumeRenderer::Interpolation, iAValueType::Categorical, volInterpolationTypes);
			addAttr(attr, iAVolumeRenderer::Shading, iAValueType::Boolean, true);
			addAttr(attr, iAVolumeRenderer::ScalarOpacityUnitDistance, iAValueType::Continuous, -1.0);

			// mapper properties:
			QStringList renderTypes = RenderModeMap().keys();
			selectOption(renderTypes, renderTypes[0]);
			addAttr(attr, iAVolumeRenderer::RendererType, iAValueType::Categorical, renderTypes);
			QStringList blendModes = BlendModeMap().keys();
			selectOption(blendModes, DefaultBlendMode);
			addAttr(attr, iAVolumeRenderer::BlendMode, iAValueType::Categorical, blendModes);
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
			selfRegister();
		}
		return attr;
	}
};

iAVolumeRenderer::iAVolumeRenderer(vtkRenderer* renderer, vtkImageData* vtkImg, iATransferFunction* tf, QVariantMap const& overrideValues) :
	iADataSetRenderer(renderer),
	m_volume(vtkSmartPointer<vtkVolume>::New()),
	m_volProp(vtkSmartPointer<vtkVolumeProperty>::New()),
	m_volMapper(vtkSmartPointer<vtkSmartVolumeMapper>::New()),
	m_image(vtkImg)
{
	m_volume->SetMapper(m_volMapper);
	m_volume->SetProperty(m_volProp);
	m_volume->SetVisibility(true);
	m_volMapper->SetInputData(vtkImg);
	if (vtkImg->GetNumberOfScalarComponents() > 1)
	{
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
	setDefaultAttributes(defaultAttributes(), overrideValues);  // use defaultAttributes and not attributes here; spacing does not get assigned a default value!
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
	m_volMapper->SetRequestedRenderMode(RenderModeMap()[values[RendererType].toString()]);
	m_volMapper->SetBlendMode(BlendModeMap()[values[BlendMode].toString()]);
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

	if (values.contains(Spacing))    // avoid warning if no spacing available (e.g. when setting default values)
	{
		auto spc = variantToVector<double>(values[Spacing]);
		if (spc.size() == 3)
		{
			m_image->SetSpacing(spc.data());
		}
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

void iAVolumeRenderer::setCuttingPlanes(std::array<vtkPlane*, 3> p)
{
	m_volMapper->AddClippingPlane(p[0]);
	m_volMapper->AddClippingPlane(p[1]);
	m_volMapper->AddClippingPlane(p[2]);
}

void iAVolumeRenderer::addCuttingPlane(vtkPlane* p)
{
	m_volMapper->AddClippingPlane(p);
}

void iAVolumeRenderer::removeCuttingPlane(vtkPlane* p)
{
	m_volMapper->RemoveClippingPlane(p);
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

