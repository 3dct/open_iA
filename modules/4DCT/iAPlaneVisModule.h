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
#pragma once

// iA
#include "iAVisModule.h"
#include "iA4DCTDefects.h"
#include "iA4DCTFileData.h"
#include "iA4DCTFileManager.h"
// vtk
#include <vtkImageData.h>
#include <vtkImageMapToColors.h>
#include <vtkImageReslice.h>
#include <vtkLookupTable.h>
#include <vtkMetaImageReader.h>
#include <vtkPlaneSource.h>
#include <vtkSmartPointer.h>
// Qt
#include <QString>
#include <QColor>
#include <QVector>

class vtkActor;
class vtkPolyDataMapper;
class vtkTexture;

struct iAPlaneVisSettings
{
	iAPlaneVisSettings( ) { Slice[0] = 0; Slice[1] = 0; Slice[2] = 0; }

	enum Direction { XY, XZ, YZ };
	Direction Dir;
	int Slice[3];
	double Opacity;
	bool Shading;
};

class iAPlaneVisModule : public iAVisModule
{
public:
				iAPlaneVisModule( );
	void		show( );
	void		hide( );
	void		setSize( double * size );
	void		setImage( iA4DCTFileData fileName );
	void		setSlice( int slice );
	void		setOpacity( double opacity );
	void		enableShading( );
	void		disableShading( );
	void		setDirXY( );
	void		setDirXZ( );
	void		setDirYZ( );
	void		getImageSize( int * imgSize );
	void		enableHighlighting( bool enable );

	template<typename T>
	void		highlightDefects( QVector<QString> defects, QVector<QColor> colors, iA4DCTFileData labeledImgFile );
	template<typename T>
	void		densityMap( QString defect, QColor color, iA4DCTFileData labeledImgFile, int * size );
	template<typename T>
	void		labledImageToMask( vtkImageData* img, iA4DCTDefects::VectorDataType list );

	iAPlaneVisSettings		settings;

private:
	void		setPlanePosition( int slice );

	vtkSmartPointer<vtkPlaneSource>			m_plane;
	vtkSmartPointer<vtkTexture>				m_texture;
	vtkSmartPointer<vtkPolyDataMapper>		m_mapper;
	vtkSmartPointer<vtkActor>				m_actor;
	vtkSmartPointer<vtkImageReslice>		m_reslice;
	vtkSmartPointer<vtkImageData>			m_img;
	vtkSmartPointer<vtkImageData>			m_colorImg;

	double			m_size[3];
	int				m_imgSize[3];
	double			m_imgSpacing[3];
	double			m_axialElements[16];
};

//==============================================
//
//			Template methods
//
//==============================================

template<typename T>
void iAPlaneVisModule::highlightDefects( QVector<QString> defects, QVector<QColor> colors, iA4DCTFileData labeledImgFile )
{
	QVector<iA4DCTDefects::VectorDataType> defectsLists;
	for( int i = 0; i < defects.size( ); i++ )
	{
		iA4DCTDefects::VectorDataType defList = iA4DCTDefects::load( defects[i] );
		defectsLists.push_back( defList );
	}

	// read the labeled image
	/*vtkSmartPointer<vtkMetaImageReader> reader = vtkSmartPointer<vtkMetaImageReader>::New( );
	reader->SetFileName( labeledImgPath.toStdString( ).c_str( ) );
	reader->Update( );
	vtkImageData * labeledImg = reader->GetOutput( );*/
	vtkImageData * labeledImg = iA4DCTFileManager::getInstance( ).getImage( labeledImgFile );

	// hash the defects
	QVector<iA4DCTDefects::HashDataType> hashes;
	for( auto l : defectsLists ) {
		iA4DCTDefects::HashDataType hash = iA4DCTDefects::DefectDataToHash( l );
		hashes.push_back( hash );
	}

	// scalars to colors
	vtkSmartPointer<vtkLookupTable> lookupTable = vtkSmartPointer<vtkLookupTable>::New( );
	lookupTable->SetRange( 0, 255 );
	lookupTable->SetValueRange( 0., 1. );
	lookupTable->SetSaturationRange( 0., 0. );
	lookupTable->SetRampToLinear( );
	lookupTable->Build( );

	vtkSmartPointer<vtkImageMapToColors> scalarValuesToColors = vtkSmartPointer<vtkImageMapToColors>::New( );
	scalarValuesToColors->PassAlphaToOutputOn( );
	scalarValuesToColors->SetLookupTable( lookupTable );
	scalarValuesToColors->SetInputData( m_img );
	scalarValuesToColors->Update( );
	m_colorImg = scalarValuesToColors->GetOutput( );

	int * dims = labeledImg->GetDimensions( );
	for( int x = 0; x < dims[0]; x++ )
	{
		for( int y = 0; y < dims[1]; y++ )
		{
			for( int z = 0; z < dims[2]; z++ )
			{
				T * labeledPixel = static_cast<T *>( labeledImg->GetScalarPointer( x, y, z ) );
				for( int i = 0; i < hashes.size( ); i++ )
				{
					if( hashes[i].contains( labeledPixel[0] ) ) {
						unsigned char * imgPixel = static_cast<unsigned char *>( m_colorImg->GetScalarPointer( x, y, z ) );
						imgPixel[0] = colors[i].red( );
						imgPixel[1] = colors[i].green( );
						imgPixel[2] = colors[i].blue( );
						imgPixel[3] = colors[i].alpha( );
						break;
					}
				}
			}
		}
	}

	m_reslice->SetInputData( m_colorImg );
	m_plane->Modified( );
}

template<typename T>
void iAPlaneVisModule::densityMap( QString defect, QColor color, iA4DCTFileData labeledImgFile, int * size )
{
	// read the labeled image
	/*vtkSmartPointer<vtkMetaImageReader> reader = vtkSmartPointer<vtkMetaImageReader>::New( );
	reader->SetFileName( labeledImgPath.toStdString( ).c_str( ) );
	reader->Update( );
	vtkImageData * labeledImg = reader->GetOutput( );*/
	vtkImageData * labeledImg = iA4DCTFileManager::getInstance( ).getImage( labeledImgFile );

	// hash the defect
	iA4DCTDefects::VectorDataType list = iA4DCTDefects::load( defect );

	//int size[0] = 5; int size[1] = 5; int size[2] = 5;
	int*** densityMap;
	densityMap = new int**[size[0]];
	for( int x = 0; x < size[0]; x++ )
	{
		densityMap[x] = new int*[size[1]];
		for( int y = 0; y < size[1]; y++ )
		{
			densityMap[x][y] = new int[size[2]];
			for( int z = 0; z < size[2]; z++ )
			{
				densityMap[x][y][z] = 0.;
			}
		}
	}

	labledImageToMask<T>( labeledImg, list );

	int * dims = labeledImg->GetDimensions( );
	int maxDensity = 0;
	for( int x = 0; x < dims[0]; x++ )
	{
		for( int y = 0; y < dims[1]; y++ )
		{
			for( int z = 0; z < dims[2]; z++ )
			{
				T * labeledPixel = static_cast<T *>( labeledImg->GetScalarPointer( x, y, z ) );
				if( labeledPixel[0] > 0 )
				{
					int newX = x * size[0] / dims[0];
					int newY = y * size[1] / dims[1];
					int newZ = z * size[2] / dims[2];
					int density = densityMap[newX][newY][newZ]++;
					if( density > maxDensity ) maxDensity = density;
				}
			}
		}
	}

	// scalars to colors
	vtkSmartPointer<vtkLookupTable> lookupTable = vtkSmartPointer<vtkLookupTable>::New( );
	lookupTable->SetRange( 0, 255 );
	lookupTable->SetValueRange( 0., 1. );
	lookupTable->SetSaturationRange( 0., 0. );
	lookupTable->SetRampToLinear( );
	lookupTable->Build( );

	vtkSmartPointer<vtkImageMapToColors> scalarValuesToColors = vtkSmartPointer<vtkImageMapToColors>::New( );
	scalarValuesToColors->PassAlphaToOutputOn( );
	scalarValuesToColors->SetLookupTable( lookupTable );
	scalarValuesToColors->SetInputData( m_img );
	scalarValuesToColors->Update( );
	m_img = scalarValuesToColors->GetOutput( );

	for( int x = 0; x < dims[0]; x++ )
	{
		for( int y = 0; y < dims[1]; y++ )
		{
			for( int z = 0; z < dims[2]; z++ )
			{
				unsigned char * imgPixel = static_cast<unsigned char *>( m_img->GetScalarPointer( x, y, z ) );
				int newX = x * size[0] / dims[0];
				int newY = y * size[1] / dims[1];
				int newZ = z * size[2] / dims[2];
				double coef = (double)densityMap[newX][newY][newZ] / maxDensity;
				imgPixel[0] = imgPixel[0] + ( color.red( ) - imgPixel[0] ) * coef;
				imgPixel[1] = imgPixel[1] + ( color.green( ) - imgPixel[1] ) * coef;
				imgPixel[2] = imgPixel[2] + ( color.blue( ) - imgPixel[2] ) * coef;
			}
		}
	}

	m_reslice->SetInputData( m_img );

	for( int x = 0; x < size[0]; x++ )
	{
		for( int y = 0; y < size[1]; y++ )
		{
			delete[ ] densityMap[x][y];
		}
		delete[ ] densityMap[x];
	}
	delete[ ] densityMap;
}

template<typename T>
void iAPlaneVisModule::labledImageToMask( vtkImageData* img, iA4DCTDefects::VectorDataType list )
{
	iA4DCTDefects::HashDataType hash = iA4DCTDefects::DefectDataToHash( list );

	int* dims = img->GetDimensions( );
	for( int x = 0; x < dims[0]; x++ ) {
		for( int y = 0; y < dims[1]; y++ ) {
			for( int z = 0; z < dims[2]; z++ ) {
				T* pixel = static_cast<T*>( img->GetScalarPointer( x, y, z ) );
				if( hash.contains( pixel[0] ) ) {
					pixel[0] = 1;
				}
				else {
					pixel[0] = 0;
				}
			}
		}
	}
}
