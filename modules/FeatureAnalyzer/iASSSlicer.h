/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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

#include <iASlicerMode.h>

#include <vtkSmartPointer.h>

#include <QScopedPointer>
#include <QString>


class vtkImageData;
class vtkTransform;
class vtkColorTransferFunction;
class vtkMarchingContourFilter;
class vtkDistancePolyDataFilter;
class vtkPolyData;
struct iAChanData;
class iASlicer;

#if QT_VERSION < QT_VERSION_CHECK(5, 99, 0)
class QStringList;
#else
#include <QList>
using QStringList = QList<QString>;
#endif
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
