// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <vtkSmartPointer.h>

#include <QVector>
#include <QWidget>

#include <memory>

class vtkImageData;
class QImage;

class iASimilarityMapWidget : public QWidget
{
	Q_OBJECT
public:
	iASimilarityMapWidget( QWidget * parent = 0 );
	void setImageData( vtkImageData * image );
	void setWindowing( double lowerVal, double upperVal );
	void load( QString const & filename );
	typedef double ImageScalarType;
protected:
	void paintEvent(QPaintEvent * ) override;
	void mouseMoveEvent( QMouseEvent * event ) override;
	void mouseReleaseEvent( QMouseEvent * event ) override;
	void drawMap();
	void drawPeak();
	void drawAverageSimilarityPlot();
	void updateQtImage();
	void updateAverageSimilarity();
	void applyWindow( ImageScalarType &val_out, const double( &windowRange )[2] );
	void findPeak( int x, int y );
	void findPeakRanges();
	void binsFromPos(double const pos[2], int (&bins_out)[2]);
	void posFromBins( const int( &bins )[2], int( &pos_out )[2] );
signals:
	void energyBinsSelectedSignal( int binX, int binY );

protected:
	vtkSmartPointer<vtkImageData> m_vtkImageData;
	QVector<ImageScalarType> m_avrgSimilarityVec;
	int m_numBins;
	std::shared_ptr<QImage>	m_qtImage;
	double m_WindowRange[2];
	int m_mapWidth;
	int m_peakPos[2]; //TODO: refactor?, struct/class iAPeak?
	int m_peakRange[2][2];
};
