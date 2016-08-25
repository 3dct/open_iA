/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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
 

/*
	Autor: AMA2
*/

#pragma once
#ifndef __iaBlobCluster_h
#define __iaBlobCluster_h

class iABlobImplicitFunction;

#include <vtkPolyDataNormals.h>
#include <vtkWindowedSincPolyDataFilter.h>
#include <vtkSampleFunction.h>
#include <vtkContourFilter.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkOutlineFilter.h>
#include <vtkFeatureEdges.h>
#include <vtkStripper.h>
#include <vtkStructuredPoints.h>
#include <QVector>
#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkProperty.h>
#include <QImage>
#include <QPen>
#include <QPainter>
#include <QFont>
#include <QApplication>


#include "iAXmlFiberParser.h"	// will be removed in feature
#include "iALabel3D.h"
#include "iABlobImplicitFunction.h"

class vtkDepthSortPolyData;
class vtkPolyDataSilhouette;

class iABlobCluster
{
public:
								iABlobCluster (void);

								~iABlobCluster (void);

	virtual void				SetDimension (int dimens[3]);

	void						SetBounds (double bounds[6]);

								// Get dimension for blob
	void GetDimension (int dimens[3]) const;
	int*						GetDimensions();

								// Get bounds for blob
	void						GetBounds (double bounds[6]) const;

								// Redefine fibres cluster
	void						SetCluster (QVector<FiberInfo> fibres) const;

								// Set if to draw a silhouette
	void						SetSilhouette (bool isOn);
	bool						GetSilhouette() const;

								// Turn on drawing silhouette
	void						SilhouetteOn (void);

								// Turn off drawing silhouette
	void						SilhouetteOff (void);

								// Turn on draw label
	void						LabelOn (void);

								// Turn on/off draw label
	void						SetLabel (bool isOn);
	bool						GetLabel () const;

								// Turn off draw label
	void						LabelOff (void);

								// Set renderers which will be visualize blob and labels
	void						AttachRenderers (vtkRenderer* blobRen, vtkRenderer* labelRen);

								// Get property from isosurface actor
	vtkProperty*				GetSurfaceProperty (void);

								// Get implicity fuction
	iABlobImplicitFunction*		GetImplicitFunction (void);

								// Get property from silhouette actor
	vtkProperty*				GetSilhouetteProperty (void);

								// Add clipping plane to mappers
	void						AddClippingPlane (vtkPlane* plane);

								// Fully update blob visualization
	void						Update (void);

								// Set name for blob
	void						SetName (QString name);

								// Set fiber count and fiber percentage
	void						SetFiberStats (const double fiberCount, const double fiberPercentage);

	void						SetBlobManager (iABlobManager* blobManager);

	void						CalculateImageData (void);

	vtkImageData*				GetImageData (void) const;

	double GetRange (void) const;
	void						SetRange (double range);

	void						ModifiedSampleFunction (void);

	void						SetSmoothing(bool isOn);
	bool						GetSmoothing() const;

	void						SetLabelScale(double labelScale);
	double						GetLabelScale() const;

	void						SetShowBlob(bool showBlob);
	bool						GetShowBlob() const;

	void						SetBlobOpacity(double blobOpacity);
	double						GetBlobOpacity() const;

	void						SetSilhouetteOpacity(double silhouetteOpacity);
	double						GetSilhouetteOpacity() const;

	void						SetObjectType( QString type );

	vtkPolyData *				GetBlobPolyData() const;

	void						SetRenderIndividually(bool enabled);

	void						SetGaussianBlurVariance(double blurVariance);
	double						GetGaussianBlurVariance() const;
	void						GaussianBlur();

	//private members
private:
	void						UpdatePipeline (void);
	void						ResetRenderers (void);
	void						UpdateRenderer (void);
	void						SetDefaultProperties (void);
	void						DrawLabel (void);
	void						RemoveLabel (void);	// NOT IMPLEMENTED

	vtkRenderer*				m_blobRenderer;
	vtkRenderer*				m_labelRenderer;

	iALabel3D					m_label;
	QString						m_name;
	double						m_fiberCount;
	double						m_fiberPercentage;
	QString						m_objectType;

	// vtk members
	vtkPolyDataNormals*			m_polyDataNormals;
	vtkWindowedSincPolyDataFilter* m_smoother;
	iABlobImplicitFunction*		m_implicitFunction;
	vtkSampleFunction*			m_sampleFunction;
	vtkContourFilter*			m_contourFilter;
	vtkPolyDataMapper*			m_contourMapper;
	vtkActor*					m_contourActor;
	vtkPolyDataSilhouette*		m_silhouette;
	vtkPolyDataMapper*			m_silhouetteMapper;
	vtkActor*					m_silhouetteActor;
	vtkImageData*				m_imageData;
	iABlobManager*				m_blobManager;

	int							m_dimens[3];
	double						m_bounds[6];
	int							m_countContours;
	double						m_range[2];
	bool						m_silhouetteIsOn;
	bool						m_blobIsOn;
	bool						m_labelIsOn;
	bool						m_isSmoothingOn;
	bool						m_renderIndividually;
	double						m_blurVariance;

	double						m_labelScale;

	double						m_blobOpacity;
	double						m_silhouetteOpacity;	
};

#endif // __iaBlobCluster_h
