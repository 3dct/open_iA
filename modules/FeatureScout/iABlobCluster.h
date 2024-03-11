// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <vtkSmartPointer.h>

#include <QColor>
#include <QVector>

class iABlobImplicitFunction;
class iABlobManager;

class vtkActor;
class vtkCaptionActor2D;
class vtkContourFilter;
class vtkImageData;
class vtkPlane;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkPolyDataNormals;
class vtkProperty;
class vtkRenderer;
class vtkSampleFunction;
class vtkWindowedSincPolyDataFilter;

typedef struct {
	double	x1, y1, z1,
			x2, y2, z2,
			straightLength, curvedLength, diameter, surfaceArea, volume;
	bool	isSeperated, isCurved;
} FeatureInfo;

class vtkDepthSortPolyData;
class vtkPolyDataSilhouette;

class iABlobCluster
{
public:
	iABlobCluster ();
	virtual ~iABlobCluster ();
	virtual void SetDimension (int dimens[3]);
	void SetBounds (double bounds[6]);
	void GetDimension (int dimens[3]) const;  //!< Get dimension for blob
	int* GetDimensions();
	void GetBounds (double bounds[6]) const;  //!< Get bounds for blob
	void SetCluster (QVector<FeatureInfo> fibres) const; //!< Redefine fibres cluster
	void SetSilhouette (bool isOn);           //!< Set if to draw a silhouette
	bool GetSilhouette() const;
	void SilhouetteOn ();                     //!< Turn on drawing silhouette
	void SilhouetteOff ();                    //!< Turn off drawing silhouette
	void SetLabel (bool isOn);                //! Turn on/off draw label
	bool GetLabel () const;
	//! Set renderers which will be visualize blob and labels
	void AttachRenderers( vtkSmartPointer<vtkRenderer> blobRen,
						  vtkSmartPointer<vtkRenderer> labelRen );
	//! Get property from isosurface actor
	vtkProperty* GetSurfaceProperty ();
	// Get implicity function
	iABlobImplicitFunction* GetImplicitFunction ();
	//! Get property from silhouette actor
	vtkProperty* GetSilhouetteProperty ();
	//! Add clipping plane to mappers
	void AddClippingPlane (vtkPlane* plane);
	//! Fully update blob visualization
	void Update ();
	//! Set name for blob
	void SetName (QString name);
	//! Set color for blob
	void SetBlobColor(QColor blobColor);
	//! Get color for blob
	QColor GetBlobColor();
	//! Set object count and percentage
	void SetStats (const double count, const double percentage);
	void SetBlobManager (iABlobManager* blobManager);
	void CalculateImageData ();
	vtkImageData* GetImageData () const;
	double GetRange (void) const;
	void SetRange (double range);
	void ModifiedSampleFunction (void);
	void SetSmoothing(bool isOn);
	bool GetSmoothing() const;
	void SetShowBlob(bool showBlob);
	bool GetShowBlob() const;
	void SetBlobOpacity(double blobOpacity);
	double GetBlobOpacity() const;
	void SetSilhouetteOpacity(double silhouetteOpacity);
	double GetSilhouetteOpacity() const;
	void SetObjectType( QString type );
	vtkPolyData * GetBlobPolyData() const;
	void SetRenderIndividually(bool enabled);
	void SetGaussianBlurVariance(double blurVariance);
	double GetGaussianBlurVariance() const;
	void GaussianBlur();

private:
	void UpdatePipeline ();
	void ResetRenderers ();
	void UpdateRenderer ();
	void SetDefaultProperties ();

	QString   m_name;
	QColor	  m_blobColor;
	double    m_count;
	double    m_percentage;
	QString   m_objectType;
	int       m_dimens[3];
	double    m_bounds[6];
	int       m_countContours;
	double    m_range[2];
	bool      m_silhouetteIsOn;
	bool      m_blobIsOn;
	bool      m_labelIsOn;
	bool      m_isSmoothingOn;
	bool      m_renderIndividually;
	double    m_blurVariance;
	double    m_blobOpacity;
	double    m_silhouetteOpacity;
	iABlobManager* m_blobManager;
	// vtk members
	vtkSmartPointer<vtkRenderer>                m_blobRenderer;
	vtkSmartPointer<vtkRenderer>                m_labelRenderer;
	vtkSmartPointer<vtkPolyDataNormals>         m_polyDataNormals;
	vtkSmartPointer<vtkWindowedSincPolyDataFilter> m_smoother;
	iABlobImplicitFunction*	                    m_implicitFunction;
	vtkSmartPointer<vtkSampleFunction>          m_sampleFunction;
	vtkSmartPointer<vtkContourFilter>           m_contourFilter;
	vtkSmartPointer<vtkPolyDataMapper>          m_contourMapper;
	vtkSmartPointer<vtkActor>                   m_contourActor;
	vtkSmartPointer<vtkPolyDataSilhouette>      m_silhouette;
	vtkSmartPointer<vtkPolyDataMapper>          m_silhouetteMapper;
	vtkSmartPointer<vtkActor>                   m_silhouetteActor;
	vtkSmartPointer<vtkImageData>               m_imageData;
	vtkSmartPointer<vtkCaptionActor2D>			m_captionActor;
};
