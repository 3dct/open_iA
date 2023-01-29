// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iASlicerMode.h>

#include <vtkSmartPointer.h>

#include <QScopedPointer>
#include <QString>
#include <QStringList>

class vtkImageData;
class vtkTransform;
class vtkColorTransferFunction;
class vtkMarchingContourFilter;
class vtkDistancePolyDataFilter;
class vtkPolyData;
struct iAChanData;
class iASlicer;

class QWidget;
class QVBoxLayout;

class iASSSlicer
{
public:
	iASSSlicer( const QString slicerName, vtkSmartPointer<vtkTransform> transform);
	~iASSSlicer();
	void enableMasksChannel( bool isEnabled );
	void enableGTChannel( bool isEnabled );
	void enableContours(bool isEnabled);
	void setMasksOpacity( double opacity );
	void setGTOpacity( double opacity );
	void changeMode( iASlicerMode mode );
	void initialize( vtkSmartPointer<vtkImageData> img,
		vtkSmartPointer<vtkColorTransferFunction> tf );
	void initBPDChans( QString const & minFile, QString const & medFile, QString const & maxFile );
	void initializeMasks( QStringList & masks );
	void initializeGT( QString const & fileName );
	vtkPolyData * GetDeviationPolyData( int deviationMode );
	vtkPolyData * GetMedPolyData();
	QString getSlicerName();

protected:
	void computeAggregatedImageData( const QStringList & filesList );
	void initializeChannel( iAChanData * chData );
	void update();

public:
	QWidget * container;
	QVBoxLayout * containerLayout;
	iASlicer * slicer;
	QScopedPointer<iAChanData> masksChan;
	QScopedPointer<iAChanData> gtChan;
	QScopedPointer<iAChanData> minChan;
	QScopedPointer<iAChanData> medChan;
	QScopedPointer<iAChanData> maxChan;
	vtkSmartPointer<vtkMarchingContourFilter> medContour, minContour, maxContour;
	vtkSmartPointer<vtkDistancePolyDataFilter> distFilterMax, distFilterMin;

private:
	const QString m_SlicerName;
};
