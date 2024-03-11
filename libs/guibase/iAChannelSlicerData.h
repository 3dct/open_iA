// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iaguibase_export.h"

#include <vtkSmartPointer.h>

#include <QString>


class iAChannelData;

class vtkAbstractTransform;
class vtkActor;
class vtkImageActor;
class vtkImageData;
class vtkImageMapToColors;
class vtkImageReslice;
class vtkLookupTable;
class vtkMarchingContourFilter;
class vtkPolyDataMapper;
class vtkPiecewiseFunction;
class vtkRenderer;
class vtkScalarsToColors;

//! Class storing required data for visualizing a "channel" (iAChannelData) in a slicer
class iAguibase_API iAChannelSlicerData
{
public:
	iAChannelSlicerData(iAChannelData const & chData, int mode);
	void update(iAChannelData const & chData);
	void setResliceAxesOrigin(double x, double y, double z);
	void resliceAxesOrigin(double * origin);
	//! Get lookup table (combined color transfer function + piecewise function for opacity)
	//vtkScalarsToColors* getLookupTable();
	//! Get color transfer function (only the colors, fully opaque)
	vtkScalarsToColors * colorTF();
	//! Get opacity transfer function (if not used for this channel data, nullptr is returned!)
	vtkPiecewiseFunction * opacityTF();

	void updateMapper();
	void updateResliceAxesDirectionCosines(int mode);
	void setTransform(vtkAbstractTransform * transform);
	void updateReslicer();
	void updateLUT();
	bool isEnabled() const;        //!< whether this channel is currently shown
	QString const & name() const;  //!< the name of the channel
	vtkImageActor * imageActor();  //! TODO: should be removed
	vtkImageData * input() const;  // TODO: returned vtkImageData should be const
	vtkImageData * output() const; // TODO: returned vtkImageData should be const
	vtkImageReslice * reslicer() const; // check if that can be kept private somehow
	double const * actorPosition() const;
	void setActorPosition(double x, double y, double z);
	void setActorOpacity(double opacity);
	void setMovable(bool movable);
	void setInterpolate(bool interpolate);
	void setEnabled(vtkRenderer* ren, bool enable);
	void setSlabNumberOfSlices(int slices);
	void setSlabMode(int mode);

	// TODO: contour functionality should be moved into separate class:
	// {
	void setContours(int numberOfContours, double contourMin, double contourMax);
	void setContours(int numberOfContours, double const * contourValues);
	void setShowContours(vtkRenderer* ren, bool show);
	void setContourLineParams(double lineWidth, bool dashed = false);
	void setContoursColor(double * rgb);
	void setContoursOpacity(double opacity);
	// }
private:
	Q_DISABLE_COPY_MOVE(iAChannelSlicerData);

	void assign(vtkSmartPointer<vtkImageData> imageData);
	void setupOutput(vtkScalarsToColors* ctf, vtkPiecewiseFunction* otf);

	vtkSmartPointer<vtkImageActor>  m_imageActor;
	vtkSmartPointer<vtkImageReslice> m_reslicer;
	vtkSmartPointer<vtkImageMapToColors> m_colormapper;
	vtkSmartPointer<vtkLookupTable> m_lut;   //! used for combining given ctf and otf into a single transfer function for both color and opacity accepted by imageMapToColors
	vtkScalarsToColors *            m_cTF;   //! the color transfer function - should be const (as soon as VTK supports it)
	vtkPiecewiseFunction *          m_oTF;   //! the opacity function - nullptr if not used - should be const (as soon as VTK supports it)
	QString                         m_name;  //! name of the channel
	bool                            m_enabled;//! whether this channel is enabled

	//! @{ for contours / iso lines
	void initContours();    // TODO: contour functionality should be moved into separate class
	vtkSmartPointer<vtkMarchingContourFilter> m_contourFilter;
	vtkSmartPointer<vtkPolyDataMapper> m_contourMapper;
	vtkSmartPointer<vtkActor>       m_contourActor;
	//! @}
};
