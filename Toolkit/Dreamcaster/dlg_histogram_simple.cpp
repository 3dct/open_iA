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
#include "dlg_histogram_simple.h"

#include <QPainter>
#include <QToolTip>

#include <cmath>

double dlg_histogram_simple::ZOOM_STEP = 1.5;
double dlg_histogram_simple::HIST_ZOOM_STEP = 1.5;

dlg_histogram_simple::dlg_histogram_simple(QWidget *parent) : QWidget (parent)
{
	//set the UI as the working widget
	//setupUi(this);
	translation = 0.0;
	tmpTranslation = 0.0;
	zoom = 1.0;
	tmpZoom = 1.0;
	histZoom = 1.0;
	tmpHistZoom = 1.0;	 
	activeChild = parent;
	accSpacing = 1.0;
}

dlg_histogram_simple::~dlg_histogram_simple()
{

}

void dlg_histogram_simple::initialize(unsigned int* histData, int numberOfBeans, double _dataRange[2] )
{
	//set attribut, so that the objects are deleted while
	//this widget is closed
	this->setAttribute(Qt::WA_DeleteOnClose);
	this->setFocusPolicy(Qt::WheelFocus);

	//initialise variables
	mode = NO_MODE;
	wheelMode = ZOOM_WHEEL_MODE;
	
	draw = false;
	updateAutomatically = true;
	
	width = this->geometry().width();
	height = this->geometry().height();
	numBin = numberOfBeans;
	bottomMargin = 30;
	selectedPoint = -1;

	//get the scalar range, spacing and datapointer of the data object
	oldDataRange[0] = dataRange[0];
	oldDataRange[1] = dataRange[1];
	dataRange[0] = _dataRange[0]; dataRange[1] = _dataRange[1];
	histoPtr.clear();
	for (int i=0; i<numBin; i++)
	{
		histoPtr.push_back(histData[i]);
	}
	// find maximum frequency
	maxFreq = 1;
	for ( int i = 0; i < numBin; i++ ) {
		if (histoPtr[i] > maxFreq)
			maxFreq = histoPtr[i];
	}
}

/**
* \brief  isUpdateAutomatically
*
*  Returns if volume is updated automatically after changing the transfer function
*
* \param 
* \return	
*
*/
bool dlg_histogram_simple::isUpdateAutomatically()
{
	return updateAutomatically;
}


/**
* \brief  resizeEvent
*
*       
*
* \param QResizeEvent *event
* \return	
*
*/
void dlg_histogram_simple::resizeEvent(QResizeEvent *event)
{
	if ( ((this->geometry().width()) != width) || ((this->geometry().height()) != height) )
	{
		width = this->geometry().width();
		height = this->geometry().height();
		//create a QImage newImage with the new window size and load to the QImage image
		image = QImage(width, height, QImage::Format_ARGB32);
		draw = true;
	}
	QWidget::resizeEvent(event);
}

/**
* \brief  paintEvent
*
*       
*
* \param QPaintEvent * 
* \return	
*
*/
void dlg_histogram_simple::paintEvent(QPaintEvent * )
 {
	 if (draw) this->drawHistogram();
	//set the widget as the drawing device for the Qpainter painter
	QPainter painter(this);
	//use the painter to draw from Qimage image to the drawing device widget
	painter.drawImage(QRectF(0, 0, width, height), image);
}

/**
* \brief  drawHistogram
*
*   This function is used to draw the histogram    
*
* \param 
* \return	
*
*/
void dlg_histogram_simple::drawHistogram()
{	
	//initialise a painter
	QPainter painter(&image);
	painter.setRenderHint(QPainter::Antialiasing);
	if(painter.isActive() == false)
			return;
	drawBackground(painter);
	
	//change the origin of the window to left bottom
	painter.translate(tmpTranslation, height - bottomMargin);
	painter.scale(1, -1);
	
	if(histoPtr.size() > 0)
		drawHistogram(painter);	
	painter.scale(1, -1);

	drawAxes(painter);

	//update the painter
	update();
	draw = false;
}

/**
* \brief  redraw
*
*   This function is used to redraw the histogram    
*
* \param 
* \return	
*
*/
void dlg_histogram_simple::redraw()
{
	draw = true;
	update();
}


/**
* \brief  mousePressEvent
*
*   
*
* \param QMouseEvent * event
* \return	
*
*/
void dlg_histogram_simple::mousePressEvent(QMouseEvent *event)  
{
	switch(event->button())
	{
		case Qt::LeftButton:
			if (((event->modifiers() & Qt::ShiftModifier) == Qt::ShiftModifier) &&
				((event->modifiers() & Qt::ControlModifier) == Qt::ControlModifier) &&
				((event->modifiers() & Qt::AltModifier) == Qt::AltModifier))
			{
				zoomY = event->y();
				changeMode(HIST_ZOOM_MODE, event);
			}
			else if (((event->modifiers() & Qt::ShiftModifier) == Qt::ShiftModifier) &&
				((event->modifiers() & Qt::ControlModifier) == Qt::ControlModifier))
			{
				zoomX = event->x();
				zoomY = event->y();
				tmpZoom = zoom;
				changeMode(ZOOM_MODE, event);
			}
		break;
		case Qt::MidButton:
			changeMode(MOVE_VIEW_MODE, event);
		break;
	}
}

/**
* \brief  mouseReleaseEvent
*
*   
*
* \param QMouseEvent * event
* \return	
*
*/
void dlg_histogram_simple::mouseReleaseEvent(QMouseEvent *event)  
{
	switch(event->button())
	{
		case Qt::LeftButton:
			if (mode == HIST_ZOOM_MODE)
				histZoom = tmpHistZoom;
			else if (mode == ZOOM_MODE)
				zoom = tmpZoom;
			else if (mode == MOVE_VIEW_MODE)
				translation = tmpTranslation;
			this->mode = NO_MODE;

			redraw();
		break;
		case Qt::MidButton:
			if (mode == MOVE_VIEW_MODE)
				translation = tmpTranslation;
			this->mode = NO_MODE;
		break;
	}
}

/**
* \brief  mouseDoubleClickEvent
*
*     
*
* \param QMouseEvent * event
* \return	
*
*/
void dlg_histogram_simple::mouseDoubleClickEvent(QMouseEvent *event)  
{
	//void
}

/**
* \brief  mouseMoveEvent
*
*     
*
* \param QMouseEvent * event
* \return	
*
*/
void dlg_histogram_simple::mouseMoveEvent(QMouseEvent *event)
{
	switch(this->mode)
	{
		case MOVE_VIEW_MODE:
			tmpTranslation = translation + event->x() - dragStartPosX;

			if (tmpTranslation > 0)
				tmpTranslation = 0;
			else if (tmpTranslation < -(width * tmpZoom - width))
				tmpTranslation = -(width * tmpZoom - width);

			redraw();
		break;
		case ZOOM_MODE:
			zoomHistogramView(((zoomY-event->y())/2.0)+zoom, zoomX, false);
			redraw();
		break;
		case HIST_ZOOM_MODE:
		{
			int diff = (zoomY-event->y())/2.0;
			if (diff < 0)
				zoomHistogram(-std::pow(HIST_ZOOM_STEP,-diff)+histZoom, false);
			else
				zoomHistogram(std::pow(HIST_ZOOM_STEP,diff)+histZoom, false);
			redraw();
		}
		break;
	}
	
	if (histoPtr.size() > 0)
	{
		xPos = event->x();
		if (xPos < 0.0) xPos = 0.0;
		if (xPos >= width) xPos = width-1;
			
		//calculate the nth bin located at a given pixel, actual formula is (i/100 * width) * (numBin / width)
		int nthBin = (((xPos-tmpTranslation) * numBin) / width) / tmpZoom;
		if (nthBin >= numBin || xPos == width-1) nthBin = numBin-1;

		QString text( tr("Greyvalue: %1 Frequency: %3").arg( (accSpacing * nthBin + dataRange[0]) )
												.arg( histoPtr[nthBin] ));

		QToolTip::showText( event->globalPos(), text, this );
	}
}

/**
* \brief  enterEvent
*
*   
*
* \param QEvent * event
* \return	
*
*/
void dlg_histogram_simple::enterEvent(QEvent*)
{
}

/**
* \brief  leaveEvent
*
*   
*
* \param QEvent * event
* \return	
*
*/
void dlg_histogram_simple::leaveEvent(QEvent*)
{
	this->clearFocus();
}

/**
* \brief  wheelEvent
*
*   
*
* \param QWheelEvent * event
* \return	
*
*/
void dlg_histogram_simple::wheelEvent(QWheelEvent *event)
{
	if (wheelMode == HIST_ZOOM_WHEEL_MODE)
		zoomHistogram(event->delta(), true);
	else
		zoomHistogramView(event->delta(), event->x(), true);
	
	redraw();
}

/**
* \brief  keyPressEvent
*
*   
*
* \param QKeyEvent * event
* \return	
*
*/
void dlg_histogram_simple::keyPressEvent(QKeyEvent *event)
{
	if (event->key() == Qt::Key_Alt || event->key() == Qt::Key_AltGr)
		changeWheelMode(HIST_ZOOM_WHEEL_MODE);
}

void dlg_histogram_simple::keyReleaseEvent(QKeyEvent *event)
{
	changeWheelMode(ZOOM_WHEEL_MODE);
}

/**
* \brief  view2dataX
*
*   Converts the x value of the histogram view to transfer function data
*
* \param dataX double pointer to save data
* \param viewX X value of the histogram view
*
* \return
*/
void dlg_histogram_simple::view2dataX(double *dataX, int viewX)
{
	*dataX = ((double)(viewX-tmpTranslation) / (double)this->geometry().width() * (dataRange[1] - dataRange[0]) ) /tmpZoom + dataRange[0];
}

/**
* \brief  view2dataY
*
*   Converts the y value of the histogram view to transfer function data
*
* \param dataY double pointer to save data
* \param viewY Y value of the histogram view
*
* \return
*/
void dlg_histogram_simple::view2dataY(double *dataY, int viewY)
{
	*dataY = ((double)this->geometry().height() - bottomMargin - viewY ) / (double)(this->geometry().height() - bottomMargin);
}

/**
* \brief  view2data
*
*   Converts the x and y value of the histogram view to transfer function data
*
* \param dataX double pointer to save data
* \param dataY double pointer to save data
* \param viewX X value of the histogram view
* \param viewY Y value of the histogram view
*
* \return
*/
void dlg_histogram_simple::view2data(double *dataX, double *dataY, int viewX, int viewY)
{
	view2dataX(dataX, viewX);
	view2dataY(dataY, viewY);
}

/**
* \brief  data2viewX
*
*   Converts x value of transfer function data to histogram view
*
* \param viewX int pointer to save view data
* \param dataX X value of the transfer function data
*
* \return
*/
void dlg_histogram_simple::data2viewX(int *viewX, double dataX, double oldDataRange0, double oldDataRange1)
{
	if (oldDataRange0 == -1 && oldDataRange1 == -1)
	{
		*viewX = (int)((dataX -dataRange[0]) * (double)this->geometry().width() / (dataRange[1] - dataRange[0])*tmpZoom +tmpTranslation);
	}
	else
	{
		*viewX = (int)((dataX -oldDataRange0) * (double)this->geometry().width() / (oldDataRange1 - oldDataRange0)*tmpZoom +tmpTranslation);
	}
}

/**
* \brief  data2viewY
*
*   Converts y value of transfer function data to histogram view
*
* \param viewY int pointer to save view data
* \param dataY Y value of the transfer function data
*
* \return
*/
void dlg_histogram_simple::data2viewY(int *viewY, double dataY)
{
	*viewY = (int)((this->geometry().height() - bottomMargin) - dataY *(double)(this->geometry().height() - bottomMargin));
}

/**
* \brief  data2view
*
*   Converts x and y value of transfer function data to histogram view
*
* \param viewX int pointer to save view data
* \param viewY int pointer to save view data
* \param dataX X value of the transfer function data
* \param dataY Y value of the transfer function data
*
* \return
*/
void dlg_histogram_simple::data2view(int *viewX, int *viewY, double dataX, double dataY)
{
	data2viewX(viewX, dataX);
	data2viewY(viewY, dataY);
}


/**
* \brief  getSelectedPoint
*
*   Draws the background of the histogram view.
*
* \return
*/
void dlg_histogram_simple::drawBackground(QPainter &painter)
{
	//create linear gradient
	QLinearGradient bg(0, 0, 0, this->geometry().height());
	bg.setSpread(QGradient::PadSpread);
	bg.setColorAt( 0.0, QColor(128,170,255) );
	bg.setColorAt( 1.0, QColor(255,255,255) );
	
	//apply the linear gradient to the brush
	painter.fillRect( image.rect(), bg );
}

/**
* \brief  getSelectedPoint
*
*   Draws the histogram of the histogram view.
*
* \return
*/
void dlg_histogram_simple::drawHistogram(QPainter &painter)
{
	double binWidth = (double)width / numBin *tmpZoom;
	
	int intBinWidth = (int)binWidth;

	if (intBinWidth < binWidth)
		intBinWidth++;

	//change the pen color to blue
	painter.setPen(Qt::blue);
	
	if (histoPtr.size() > 0)
	{
		int x, h;
		
		//draw the histogram using the painter on the image
		for ( int j = 0; j < numBin; j++ ) 
		{		 
			x = (int)(j * binWidth);
			h = (double)histoPtr[j] / maxFreq * (height - bottomMargin - OFFSET_FROM_TOP) * tmpHistZoom;
			//if (h < histoPtr[j] * (double)(height) / maxFreq-1.0) //overflow
			//	h = height;
			painter.fillRect(QRect(x, 0, intBinWidth, h), Qt::blue); // draw line 
		}
	}
}

/**
* \brief  getSelectedPoint
*
*   Draws the axes of the histogram view.
*
* \return
*/
void dlg_histogram_simple::drawAxes(QPainter &painter)
{
	//change pen color to black
	painter.setPen(Qt::black);

	//markings in x axis
	int stepNumber = (int)(10.0*tmpZoom);
	double step = 100.0 / stepNumber;
	double pos = 0.0;
	for (int i = 0; i<= stepNumber; i++)
	{
		pos = step *i;
		//calculate the nth bin located at a given pixel, actual formula is (i/100 * width) * (numBin / width)
		int nthBin = (int)((pos * dataRange[1]) / 100.0);
		//get the intensity value into a string
		double value = accSpacing * nthBin + dataRange[0];

		QString text;
		if (value < 1.0)
			text = QString::number(value, 'g', 3);
		else
			text = QString::number((int)value, 10);

		//calculate the x coordinate
		int x = (int)(pos * ((int)(width*tmpZoom) / 100.0));
		//draw a small indicator line
		painter.drawLine(x, bottomMargin*0.1, x, 0);
		int coef = 3;
		if(i == stepNumber)
			coef = 6;
		if (i <= 0)
			painter.drawText(x, 15, text ); //write the text right aligned to indicator line
		else
			painter.drawText(x - text.length()*coef, 15, text); //write the text centered to the indicator line
	}
	//set pen color to black
	painter.setPen(Qt::black);
	//draw the x axis
	painter.drawLine(0, 0, width*tmpZoom, 0);
	

	//write the x axis label
	painter.drawText( QPointF((int)(width * 0.45 ), bottomMargin-2), "Penetration length");
}

/**
* \brief  changeMode
*
*   Changes the mode of interaction. Possible modes are NO_MODE, MOVE_VIEW_MODE
*
* \return
*/
void dlg_histogram_simple::changeMode(Mode mode, QMouseEvent *event)
{
	if(mode == MOVE_VIEW_MODE)
			dragStartPosX = event->x();
	this->mode = mode;
}

/**
* \brief  changeWheelMode
*
*   Changes the mode of interaction with the mouse wheel. Possible modes are ZOOM_WHEEL_MODE, HIST_ZOOM_WHEEL_MODE
*
* \return
*/
void dlg_histogram_simple::changeWheelMode(WheelMode mode)
{
	this->wheelMode = mode;
}

/**
* \brief  zoomHistogramView
*
*   Zooms the histogram view.
*
* \param value Indicates how big the zoom steps are
* \param x position where to zoom in and out
* \param deltaMode Indicates if the value is used as delta or absolute value
* \return
*/
void dlg_histogram_simple::zoomHistogramView(double value, int x, bool deltaMode)
{
	int absXAfterZoom;

	double delta;

	if (deltaMode)
	{
		delta = value;
	
		if (delta > 0)
		{
			int absoluteX = x-tmpTranslation;
			double absoluteXRatio = (double)absoluteX/(width*zoom);
			zoom *= ZOOM_STEP;
			if (zoom > MAX_ZOOM)
				zoom = MAX_ZOOM;

			absXAfterZoom = (int)(width*zoom*absoluteXRatio);
		}
		else
		{
			int absoluteX = x-tmpTranslation;
			double absoluteXRatio = (double)absoluteX/(width*zoom);
			zoom /= ZOOM_STEP;
			if (zoom < MIN_ZOOM)
				zoom = MIN_ZOOM;

			absXAfterZoom = (int)(width*zoom*absoluteXRatio);
		}

		tmpZoom = zoom;
	}
	else
	{
		int absoluteX = x-tmpTranslation;
		double absoluteXRatio = (double)absoluteX/(width*tmpZoom);
		
		tmpZoom = value;
		if (tmpZoom < MIN_ZOOM)
			tmpZoom = MIN_ZOOM;
		if (tmpZoom > MAX_ZOOM)
			tmpZoom = MAX_ZOOM;

		absXAfterZoom = (int)(width*tmpZoom*absoluteXRatio);
	}

	tmpTranslation = -absXAfterZoom +x;

	if (tmpTranslation > 0)
		tmpTranslation = 0;
	else if (tmpTranslation < -(width * tmpZoom - width))
		tmpTranslation = -(width * tmpZoom - width);

	translation = tmpTranslation;
}

/**
* \brief  zoomHistogram
*
*   Zooms the histogram.
*
* \param value Indicates how big the zoom steps are
* \param deltaMode Indicates if the value is used as delta or absolute value
* \return
*/
void dlg_histogram_simple::zoomHistogram(double value, bool deltaMode)
{
	if (deltaMode)
	{
		if (value > 0)
			histZoom *= HIST_ZOOM_STEP;
		else
		{
			histZoom /= HIST_ZOOM_STEP;
			if (histZoom < MIN_ZOOM)
				histZoom = MIN_ZOOM;
		}

		tmpHistZoom = histZoom;
	}
	else
	{
		tmpHistZoom = value;
		if (tmpHistZoom < MIN_ZOOM)
			tmpHistZoom = MIN_ZOOM;
	}
}

/**
* \brief  autoUpdate
*
*   If set to true rendering view updates are done automatically after changes in the transfer function.
*
* \return
*/

void dlg_histogram_simple::autoUpdate(bool toggled)
{
	updateAutomatically = toggled;
}

/**
* \brief  resetView
*
*   Reset the histogram view and redraw it.
*
* \return
*/

void dlg_histogram_simple::resetView()
{
	translation = 0.0;
	tmpTranslation = 0.0;
	zoom = 1.0;
	tmpZoom = 1.0;

	redraw();
}
