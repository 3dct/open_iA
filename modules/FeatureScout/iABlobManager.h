// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

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

class iABlobCluster;
class iARenderer;


class iABlobManager
{
public:
	iABlobManager ();

	//! Update each blob which belongs to the manager
	void					Update ();

	//! Add blob to manager. You need control that all blob have same data type (extent, dimension)
	void					AddBlob (iABlobCluster* blob);

	//! Remove blob from manager
	void					RemoveBlob (iABlobCluster* blob);
	//! Return list of blobs which contains manager
	QList<iABlobCluster*>*	GetListObBlobClusters ();
	void	SetOverlapThreshold (double overlapThreshold);
	double	GetOverlapThreshold ();
	void	SetGaussianBlurVariance (double variance);
	double	GetGaussianBlurVariance ();
	void	SetOverlappingEnabled (bool isEnable);
	bool	OverlappingIsEnabled ();
	void	SetRange (double range);
	double	GetRange ();
	void	SetBounds (double const * bounds);
	void	GetBounds (double* bounds);
	void	SetProtrusion (double protrusion);
	double	GetProtrusion ();
	double*	GetBoundsProtrusion ();
	void	SetDimensions (int* dimens);
	int*	GetDimensions ();
	void	SetSmoothing(bool isOn);
	bool	GetSmoothing() const;
	void	SetGaussianBlur(bool isOn);
	bool	GetGaussianBlur() const;
	void	SetSilhouettes(bool isOn);
	bool	GetSilhouettes() const;
	void	SetLabeling(bool isOn);
	bool	GetLabeling() const;
	void	SetShowBlob(bool showBlob);
	bool	GetShowBlob() const;
	void	SetBlobOpacity(double blobOpacity);
	double	GetBlobOpacity() const;
	void	SetSilhouetteOpacity(double silhouetteOpacity);
	double	GetSilhouetteOpacity() const;
	void	SetUseDepthPeeling(bool enabled);
	bool	GetUseDepthPeeling() const;
	void	SetRenderers(vtkRenderer* blobRenderer, vtkRenderer* labelRenderer);
	void	InitRenderers();
	void	SaveMovie(
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
				const QString& fileName, int mode, int qual, int fps
			);
	void	UpdateBlobSettings(iABlobCluster* blob);
private:
	//! Change vktImageData for each blob for overlapping
	bool	SmartOverlapping ();
	void	InitializeMask (int extent[6]);
	void	AddBlobToMask (vtkImageData* imageData);
	void	OverlapWithMask (vtkImageData* imageData);

	QList<iABlobCluster*>	m_blobsList;
	vtkSmartPointer<vtkImageData>	m_imageMask;
	double	m_blurVariance;
	double	m_overlappingEnabled;
	bool	m_isSmoothingEnabled;
	bool	m_isGaussianBlurEnabled;
	bool	m_isSilhoetteEnabled;
	bool	m_isLabelingEnabled;
	bool	m_isBlobBodyEnabled;
	double	m_range;
	double	m_bounds [6];
	double	m_boundsProtrusionCoef;
	double	m_boundsProtrusion [6];
	int		m_dimension [3];
	bool	m_depthPeelingEnabled;

	double	m_overlapThreshold;
	double	m_blobOpacity;
	double	m_silhouetteOpacity;

	vtkSmartPointer<vtkRenderer>	m_blobRen;
	vtkSmartPointer<vtkRenderer>	m_labelRen;

	vtkSmartPointer<vtkAppendPolyData>		m_appendedBlobsPD;
	vtkSmartPointer<vtkLookupTable>			m_blobsLT;
	vtkSmartPointer<vtkDepthSortPolyData>	m_blobsDepthSort;
	vtkSmartPointer<vtkPolyDataMapper>		m_blobsMapper;
	vtkSmartPointer<vtkActor>				m_blobsActor;

	vtkSmartPointer<vtkPolyDataSilhouette>	m_silhouette;
	vtkSmartPointer<vtkPolyDataMapper>		m_silhouetteMapper;
	vtkSmartPointer<vtkActor>				m_silhouetteActor;
};
