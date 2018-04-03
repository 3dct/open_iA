/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
#include "ComparisonAndWeighting.h"
#include "raycast/include/common.h"
#include "PaintWidget.h"
#include <QWidget>
#include <QPixmap>

//ParamWidget impl//////////////////////////////////////////////////////////////////////////
ParamWidget::ParamWidget()
{
	pxmp = 0;
	paintWidget = 0;
	buffer = 0;
	initialized = false;
}

void ParamWidget::Init(int pxmpWidth, int pxmpHeight, QWidget *widget)
{
	if(initialized)
		return;
	pxmp = new QPixmap(pxmpWidth, pxmpHeight);

	paintWidget = new PaintWidget(pxmp, widget);
	paintWidget->setGeometry(0, 0, widget->geometry().width(), widget->geometry().height());
	paintWidget->setCursor(widget->cursor());
	QColor tempColor(250,250,0);
	paintWidget->SetHighlightStyle(tempColor, 2.0);
	buffer = 0;
	initialized = true;
}

int ParamWidget::AllocateBuffer(int width, int height)
{
	if(pxmp)
		delete pxmp;
	pxmp = new QPixmap(width, height);
	paintWidget->SetPixmap(pxmp);
	if(buffer)
	{
		delete [] buffer;
		buffer = 0;
	}
	int s = width*height;
	buffer = new unsigned int[s];
	for (int i=0; i<s; i++)
		buffer[i] = 0;
	memset(buffer, 0, s*sizeof(buffer[0]));
	bufferWidth = width;
	bufferHeight = height;
	return 1;
}

ParamWidget::~ParamWidget()
{
	if(buffer)
		delete [] buffer;
	if(pxmp)
		delete pxmp;
	if(paintWidget)
		delete paintWidget;
}

//ParametersView impl //////////////////////////////////////////////////////////////////////////
ParametersView::ParametersView(int width, int height, QWidget *w1, QWidget *w2, QWidget *w3)
{
	paramWidgets[0].Init(width, height, w1);
	paramWidgets[1].Init(width, height, w2);
	paramWidgets[2].Init(width, height, w3);
}

void ParametersView::Update()
{
	QPainter painter;
	for (unsigned int i=0; i<3; i++)
	{
		QImage img = QImage((uchar*)paramWidgets[i].buffer, paramWidgets[i].bufferWidth, paramWidgets[i].bufferHeight, QImage::Format_RGB32); 
		painter.begin(paramWidgets[i].pxmp);
		painter.drawImage(QRect(0, 0, paramWidgets[i].pxmp->width(), paramWidgets[i].pxmp->height()), img, QRect(0, 0, paramWidgets[i].bufferWidth, paramWidgets[i].bufferHeight));
		painter.end();
		paramWidgets[i].paintWidget->update();
	}
}

//WeightingView impl //////////////////////////////////////////////////////////////////////////
CombinedParametersView::CombinedParametersView(QWidget *resultsWidget, int width, int height) 
{
	results.Init(width, height, resultsWidget);
}

void CombinedParametersView::Update()
{
	QPainter painter;
	QImage img = QImage((uchar*)results.buffer, results.bufferWidth, results.bufferHeight, QImage::Format_RGB32); 
	painter.begin(results.pxmp);
	painter.drawImage(QRect(0, 0, results.pxmp->width(), results.pxmp->height()), img, QRect(0, 0, results.bufferWidth, results.bufferHeight));
	painter.end();
	results.paintWidget->update();
}
