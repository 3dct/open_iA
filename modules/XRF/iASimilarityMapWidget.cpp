/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
#include "iASimilarityMapWidget.h"

#include <io/iAFileUtils.h>

#include <vtkImageData.h>
#include <vtkMetaImageReader.h>

#include <QImage>
#include <QPainter>
#include <QMouseEvent>

#include <algorithm>

//TODO: typedef, boundary checks
iASimilarityMapWidget::ImageScalarType getValue( int x, int y, iASimilarityMapWidget::ImageScalarType * ptr, int * dims )
{
	return ptr[y + x*dims[1]];
}

iASimilarityMapWidget::ImageScalarType getNeighborhoodAvrg( int x, int y, iASimilarityMapWidget::ImageScalarType * ptr, int * dims )
{
	iASimilarityMapWidget::ImageScalarType val = getValue( x, y, ptr, dims );
	for( int i = -1; i <= 1; ++i )
		for( int j = -1; j <= 1; ++j )
			if( i && j )
				val += getValue( x + i, y + j, ptr, dims );
	return val;
}

int steepestGradientPos( int x, int y, int dx, int dy, iASimilarityMapWidget::ImageScalarType * ptr, int * dims )
{
	iASimilarityMapWidget::ImageScalarType gradient = 0, nextGradient = gradient;
	int curPos[2] = { x, y };
	int nextPos[2] = { curPos[0], curPos[1] };
	int iterCount = 0;
	do
	{
		if( iterCount > 100 )
			break;

		gradient = nextGradient;
		curPos[0] = nextPos[0]; curPos[1] = nextPos[1];
		nextPos[0] += dx; nextPos[1] += dy;

//		nextGradient =	getValue( nextPos[0], nextPos[1], ptr, dims )
//						- getValue( curPos[0], curPos[1], ptr, dims );
		nextGradient = getNeighborhoodAvrg( nextPos[0], nextPos[1], ptr, dims )
						- getNeighborhoodAvrg( curPos[0], curPos[1], ptr, dims );
	} while( nextGradient <= 0 );
	if( dx != 0 )
		return  curPos[0];
	return curPos[1];
}

iASimilarityMapWidget::iASimilarityMapWidget( QWidget *parent /*= 0 */ )
: QWidget( parent ), m_numBins( 0 ), m_mapWidth( 0 )
{
	m_peakPos[0] = m_peakPos[1] = 0;
	for( int i = 0; i < 2; ++i )
		for( int j = 0; j < 2; ++j )
			m_peakRange[i][j] = 0.0;

	m_WindowRange[0] = m_WindowRange[1] = 0;
	setMouseTracking( true );
}

void iASimilarityMapWidget::load( QString const & filename )
{
	vtkSmartPointer<vtkMetaImageReader> reader = vtkSmartPointer<vtkMetaImageReader>::New();
	reader->SetFileName( getLocalEncodingFileName(filename).c_str() );
	reader->Update();
	setImageData( reader->GetOutput() );
}

iASimilarityMapWidget::~iASimilarityMapWidget()
{

}

void iASimilarityMapWidget::setImageData( vtkImageData * image )
{
	m_vtkImageData = vtkSmartPointer<vtkImageData>( image );
	updateQtImage();
	updateAverageSimilarity();
}

void iASimilarityMapWidget::updateQtImage()
{
	if (!m_vtkImageData)
	{
		return;
	}
	double * scalRange = m_vtkImageData->GetScalarRange();
	int * dims = m_vtkImageData->GetDimensions();
	m_numBins = dims[0];
	ImageScalarType * scalPtr = (ImageScalarType*)m_vtkImageData->GetScalarPointer();
	double windowRange[2] = { scalRange[1] * m_WindowRange[0], scalRange[1] * m_WindowRange[1] };
	m_qtImage = QSharedPointer<QImage>( new QImage( dims[0], dims[1], QImage::Format_ARGB32 ) );

	for( int x = 0; x < dims[0]; ++x )
	{
		for( int y = 0; y < dims[1]; ++y )
		{
			int inv_y = dims[1] - y - 1;
			ImageScalarType val = getValue(x, y, scalPtr, dims);
			applyWindow( val, windowRange );
			if( val <= windowRange[0] )
				val = 0.0;
			else if( val >= windowRange[1] )
				val = 1.0;
			else
				val = (val - windowRange[0]) / (windowRange[1] - windowRange[0]);

			QColor color( 255.0*val, 255.0*val, 255.0*val );
			m_qtImage->setPixel( x, inv_y, color.rgba() );
		}
	}
}

void iASimilarityMapWidget::applyWindow( ImageScalarType &val_out, const double( &windowRange )[2] )
{
	if( val_out <= windowRange[0] )
		val_out = 0.0;
	else if( val_out >= windowRange[1] )
		val_out = 1.0;
	else
		val_out = (val_out - windowRange[0]) / (windowRange[1] - windowRange[0]);
}

void iASimilarityMapWidget::paintEvent( QPaintEvent * )
{
	drawMap();
	drawPeak();
	drawAverageSimilarityPlot();
}

void iASimilarityMapWidget::drawMap()
{
	if (!m_qtImage)
	{
		return;
	}
	QPainter painter( this );
	m_mapWidth = std::min( geometry().width(), geometry().height() );
	painter.save();
	painter.translate( 0, geometry().height() - m_mapWidth );
	painter.drawImage( QRectF( 0, 0, m_mapWidth, m_mapWidth ), *m_qtImage.data() );
	painter.restore();
}

void iASimilarityMapWidget::drawPeak()
{
	if(!m_peakPos[0] || !m_peakPos[1] )
	{
		return;
	}
	QPainter painter( this );
	painter.setRenderHint(QPainter::Antialiasing);
	QPen pen( QColor( 200, 0, 0, 200 ) ); pen.setWidthF( 2 );
	painter.setPen( pen );
	int cpos[2]; posFromBins( m_peakPos, cpos );
	QPoint c( cpos[0], cpos[1] );
	painter.drawEllipse( c, 3, 3 );
}

void iASimilarityMapWidget::drawAverageSimilarityPlot()
{
	if (m_avrgSimilarityVec.size() == 0)
	{
		return;
	}
	QPainter painter( this );
	painter.translate( m_mapWidth - 1, geometry().height() - m_mapWidth );
	for( int i = 0; i < m_avrgSimilarityVec.size(); ++i )
	{
		int pos[2][2];
		int bins[2][2] = {
			{ 0, i - 1 },
			{ static_cast<int>(m_avrgSimilarityVec[i] * 300), i }
		};
		posFromBins( bins[0], pos[0] );
		posFromBins( bins[1], pos[1] );
		QBrush brush( QColor( 80, 80, 84 ) );
		painter.fillRect( pos[0][0], pos[0][1], pos[1][0] - pos[0][0], pos[1][1] - pos[0][1], brush );
	}
}

void iASimilarityMapWidget::setWindowing( double lowerVal, double upperVal )
{
	m_WindowRange[0] = lowerVal;
	m_WindowRange[1] = upperVal;
	updateQtImage();
	repaint();
}

void iASimilarityMapWidget::mouseMoveEvent( QMouseEvent *event )
{
	int selectedBins[2];
	int pos[2] = { event->x(), event->y() };
	binsFromPos( pos, selectedBins );
	emit energyBinsSelectedSignal( selectedBins[0], selectedBins[1] );
}

void iASimilarityMapWidget::mouseReleaseEvent( QMouseEvent * event )
{
	int selectedBins[2];
	int pos[2] = { event->x(), event->y() };
	binsFromPos( pos, selectedBins );
	findPeak( selectedBins[0], selectedBins[1] );
	findPeakRanges();
	repaint();
}

void iASimilarityMapWidget::findPeak( int x, int y )
{
	if (!m_vtkImageData)
	{
		return;
	}
	int * dims = m_vtkImageData->GetDimensions();
	ImageScalarType * scalPtr = (ImageScalarType*)m_vtkImageData->GetScalarPointer();
	int pos[2] = { x, y };
	int nextPos[2] = { pos[0], pos[1] };
	ImageScalarType curVal = getNeighborhoodAvrg( pos[0], pos[1], scalPtr, dims ), betterVal = curVal;
	int iterCount = 0;
	do
	{
		if( iterCount >= 100 )
			return;
		curVal = betterVal;
		pos[0] = nextPos[0]; pos[1] = nextPos[1];
		for( int i = -1; i <= 1; ++i )
			for( int j = -1; j <= 1; ++j )
			{
				if( !i && !j )
					continue;
				ImageScalarType val = getNeighborhoodAvrg( pos[0] + i, pos[1] + j, scalPtr, dims );
				//ImageScalarType val = getValue( pos[0] + i, pos[1] + j, scalPtr, dims );
				if( val > betterVal )
				{
					betterVal = val;
					nextPos[0] = pos[0] + i;
					nextPos[1] = pos[1] + j;
				}
			}
	} while( curVal < betterVal );
	m_peakPos[0] = pos[0]; m_peakPos[1] = pos[1];
}

void iASimilarityMapWidget::findPeakRanges()
{
	if (!m_vtkImageData)
	{
		return;
	}
	int * dims = m_vtkImageData->GetDimensions();
	ImageScalarType * scalPtr = (ImageScalarType*)m_vtkImageData->GetScalarPointer();
	int x = m_peakPos[0]; int y = m_peakPos[1];
	m_peakRange[0][0] = steepestGradientPos( x, y, -1,  0, scalPtr, dims );
	m_peakRange[0][1] = steepestGradientPos( x, y, 1, 0, scalPtr, dims );
	m_peakRange[1][0] = steepestGradientPos( x, y, 0, -1, scalPtr, dims );
	m_peakRange[1][1] = steepestGradientPos( x, y, 0, 1, scalPtr, dims );
}

void iASimilarityMapWidget::binsFromPos( const int( &pos )[2], int( &bins_out )[2] )
{
	double pos_inv_y[2] = {
			static_cast<double>(pos[0]),
			static_cast<double>(geometry().height() - pos[1])
	};
	if( pos_inv_y[0] > m_mapWidth || pos_inv_y[1] > m_mapWidth )
		return;
	bins_out[0] = m_numBins == 0 ? 0 : pos_inv_y[0] / m_mapWidth * m_numBins - 1;
	bins_out[1] = m_numBins == 0 ? 0 : pos_inv_y[1] / m_mapWidth * m_numBins - 1;
}

void iASimilarityMapWidget::posFromBins( const int( &bins )[2], int( &pos_out )[2] )
{
	pos_out[0] = m_numBins == 0 ? 0 : ((double)bins[0] + 1) / m_numBins * m_mapWidth;
	pos_out[1] = m_numBins == 0 ? 0 : geometry().height() - ((double)bins[1] + 1) / m_numBins * m_mapWidth;
}

void iASimilarityMapWidget::updateAverageSimilarity()
{
	if (!m_vtkImageData)
	{
		return;
	}
	m_avrgSimilarityVec.clear();
	int * dims = m_vtkImageData->GetDimensions();
	ImageScalarType * scalPtr = (ImageScalarType*)m_vtkImageData->GetScalarPointer();
	for( int y = 0; y < dims[1]; ++y )
	{
		ImageScalarType val = 0;
		for( int x = 0; x < dims[0]; ++x )
			val += getValue( x, y, scalPtr, dims );
		val /= dims[0];
		m_avrgSimilarityVec.push_back( val );
	}
}

