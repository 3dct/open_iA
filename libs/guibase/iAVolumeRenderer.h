// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iaguibase_export.h"

#include "iADataSetRenderer.h"

#include <vtkSmartPointer.h>

class iATransferFunction;

class vtkImageData;
class vtkRenderer;
class vtkSmartVolumeMapper;
class vtkVolume;
class vtkVolumeProperty;

//! Class for rendering a volume dataset.
//! Provides convenience functionality for adding it to a render window,
//! as well as for showing its bounding box
class iAguibase_API iAVolumeRenderer : public iADataSetRenderer
{
public:
	static constexpr const char Interpolation[] = "Interpolation";
	static constexpr const char InterpolateNearest[] = "Nearest";
	static constexpr const char InterpolateLinear[] = "Linear";
	static constexpr const char ScalarOpacityUnitDistance[] = "Scalar Opacity Unit Distance";
	static constexpr const char RendererType[] = "Renderer Type";
	static constexpr const char BlendMode[] = "Blend Mode";
	static constexpr const char SampleDistance[] = "Sample Distance";
	static constexpr const char InteractiveAdjustSampleDistance[] = "Interactively Adjust Sample Distances";
	static constexpr const char AutoAdjustSampleDistance[] = "Auto-Adjust Sample Distances";
	static constexpr const char InteractiveUpdateRate[] = "Interactive Update Rate";

	iAVolumeRenderer(vtkRenderer* renderer, vtkImageData* vtkImg, iATransferFunction* tf, QVariantMap const& overrideValues = QVariantMap());
	//! ensure that we get removed from the renderer
	~iAVolumeRenderer();
	void applyAttributes(QVariantMap const& values) override;
	iAAABB bounds() override;
	double const* orientation() const override;
	double const* position() const override;
	void setPosition(double pos[3]) override;
	void setOrientation(double ori[3]) override;
	vtkProp3D* vtkProp() override;
	QVariantMap attributeValues() const override;

	void addCuttingPlane(vtkPlane* p) override;
	void removeCuttingPlane(vtkPlane* p) override;

	static iAAttributes& defaultAttributes();
	static int string2VtkVolInterpolationType(QString const& interpType);
private:
	Q_DISABLE_COPY(iAVolumeRenderer);
	void showDataSet() override;
	void hideDataSet() override;
	iAAttributes const& attributes() const override;
	vtkSmartPointer<vtkVolume> m_volume;
	vtkSmartPointer<vtkVolumeProperty> m_volProp;
	vtkSmartPointer<vtkSmartVolumeMapper> m_volMapper;
	vtkImageData* m_image;
};


// ----- Setting Details -----

//! maps the names of render modes available for volumes (Smart/Volume/GPU) to their IDs
iAguibase_API QMap<QString, int> const& RenderModeMap();
//! map the given render mode name to the matching VTK enum value
iAguibase_API int mapRenderModeToEnum(QString const&);
