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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 

/*
	author: ALMA
*/

#pragma once
#ifndef __iaBlobManager_h
#define __iaBlobManager_h

#include <vtkSmartPointer.h>
#include <QList>

class QString;
class QWidget;

class vtkActor;
class vtkAppendPolyData;
class vtkCamera;
class vtkDepthSortPolyData;
class vtkImageData;
class vtkLookupTable;
class vtkPolyDataMapper;
class vtkPolyDataSilhouette;
class vtkRenderer;
class vtkRenderWindow;
class vtkRenderWindowInteractor;

class iARenderer;
class MdiChild;

class iABlobCluster;

class iABlobManager
{
public:

	iABlobManager (void);

	~iABlobManager (void);

	// Description:
	// Update each blob belongs to manager
	void					Update (void);

	// Description:
	// Add blob to manager. You need control that all blob have same data type
	// (extent, dimension) 
	void					AddBlob (iABlobCluster* blob);

	// Description:
	// Remove blob from manager
	void					RemoveBlob (iABlobCluster* blob);

	// Description:
	// Return list of blobs which contains manager
	QList<iABlobCluster*>*	GetListObBlobClusters (void);

	void					SetOverlapThreshold (double overlapThreshold);

	double					GetOverlapThreshold (void);

	void					SetBlobResolution (int resolution);

	void					SetGaussianBlurVariance (double variance);

	double					GetGaussianBlurVariance (void);

	void					SetOverlappingEnabled (bool isEnable);

	bool					OverlappingIsEnabled (void);

	void					SetRange (double range);

	double					GetRange (void);

	void					SetBounds (double* bounds);

	void					GetBounds (double* bounds);

	void					SetProtrusion (double protrusion);

	double					GetProtrusion ();

	double*					GetBoundsProtrusion ();

	void					SetDimensions (int* dimens);
	int*					GetDimensions ();

	void					SetSmoothing(bool isOn);
	bool					GetSmoothing() const;

	void					SetGaussianBlur(bool isOn);
	bool					GetGaussianBlur() const;

	void					SetSilhouettes(bool isOn);
	bool					GetSilhouettes() const;

	void					SetLabeling(bool isOn);
	bool					GetLabeling() const;

	void					SetShowBlob(bool showBlob);
	bool					GetShowBlob() const;

	void					SetBlobOpacity(double blobOpacity);
	double					GetBlobOpacity() const;

	void					SetSilhouetteOpacity(double silhouetteOpacity);
	double					GetSilhouetteOpacity() const;

	void					SetUseDepthPeeling(bool enabled);
	bool					GetUseDepthPeeling() const;

	void					SetRenderers(vtkRenderer* blobRenderer, vtkRenderer* labelRenderer);

	void					InitRenderers();

	void					SaveMovie(
								QWidget *activeChild,
								iARenderer * raycaster,
								vtkCamera * cam,
								vtkRenderWindowInteractor * interactor,
								vtkRenderWindow * renWin,
								size_t numberOfFrames,
								const double range[2],
								const double blobOpacity[2],
								const double silhouetteOpacity[2],
								const double overlapThreshold[2],
								const double gaussianBlurVariance[2],
								const int dimX[2], const int dimY[2], const int dimZ[2],
								const QString& fileName, int mode, int qual = 2
							);

	void					UpdateBlobSettings(iABlobCluster* blob);

private:
	// Description:
	// Change vktImageData for each blob for overlapping
	bool					SmartOverlapping (void);

	void					InitializeMask (int extent[6]);

	void					AddBlobToMask (vtkImageData* imageData);

	void					OverlapWithMask (vtkImageData* imageData);

	QList<iABlobCluster*>	m_blobsList;
	vtkImageData*			m_imageMask;
	double					m_blurVariance;
	double					m_overlappingEnabled;
	bool					m_isSmoothingEnabled;
	bool					m_isGaussianBlurEnabled;
	bool					m_isSilhoetteEnabled;
	bool					m_isLabelingEnabled;
	bool					m_isBlobBodyEnabled;
	double					m_range;
	double					m_bounds [6];
	double					m_boundsProtrusionCoef;
	double					m_boundsProtrusion [6];
	int						m_dimension [3];
	bool					m_depthPeelingEnabled;

	double					m_overlapThreshold;
	double					m_blobOpacity;
	double					m_silhouetteOpacity;

	vtkRenderer*			m_blobRen;
	vtkRenderer*			m_labelRen;


	//depth peeling alternative
	vtkSmartPointer<vtkAppendPolyData>		m_appendedBlobsPD;
	vtkSmartPointer<vtkLookupTable>			m_blobsLT;
	vtkSmartPointer<vtkDepthSortPolyData>	m_blobsDepthSort;
	vtkSmartPointer<vtkPolyDataMapper>		m_blobsMapper;
	vtkSmartPointer<vtkActor>				m_blobsActor;

	vtkSmartPointer<vtkPolyDataSilhouette>	m_silhouette;
	vtkSmartPointer<vtkPolyDataMapper>		m_silhouetteMapper;
	vtkSmartPointer<vtkActor>				m_silhouetteActor;
};

#endif // __iaBlobManager_h
