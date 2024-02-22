// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAComparisonAndWeighting.h"

#include "raycast/include/iADreamCasterCommon.h"
#include "iAPaintWidget.h"

#include <QWidget>
#include <QPixmap>

#include <algorithm>    // for std::fill

//ParamWidget impl//////////////////////////////////////////////////////////////////////////
iAParamWidget::iAParamWidget():
	pxmp(nullptr),
	paintWidget(nullptr),
	buffer(nullptr),
	initialized(false)
{
}

void iAParamWidget::Init(int pxmpWidth, int pxmpHeight, QWidget *widget)
{
	if (initialized)
	{
		return;
	}
	pxmp = new QPixmap(pxmpWidth, pxmpHeight);

	paintWidget = new iAPaintWidget(pxmp, widget);
	paintWidget->setGeometry(0, 0, widget->geometry().width(), widget->geometry().height());
	paintWidget->setCursor(widget->cursor());
	QColor tempColor(250,250,0);
	paintWidget->SetHighlightStyle(tempColor, 2.0);
	buffer = nullptr;
	initialized = true;
}

int iAParamWidget::AllocateBuffer(int width, int height)
{
	if (pxmp)
	{
		delete pxmp;
	}
	pxmp = new QPixmap(width, height);
	paintWidget->SetPixmap(pxmp);
	if(buffer)
	{
		delete [] buffer;
		buffer = 0;
	}
	int s = width*height;
	buffer = new unsigned int[s];
	std::fill(buffer, buffer+s, 0);
	bufferWidth = width;
	bufferHeight = height;
	return 1;
}

iAParamWidget::~iAParamWidget()
{
	delete [] buffer;
	delete pxmp;
	delete paintWidget;
}

//ParametersView impl //////////////////////////////////////////////////////////////////////////
iAParametersView::iAParametersView(int width, int height, QWidget *w1, QWidget *w2, QWidget *w3)
{
	paramWidgets[0].Init(width, height, w1);
	paramWidgets[1].Init(width, height, w2);
	paramWidgets[2].Init(width, height, w3);
}

void iAParametersView::Update()
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
iACombinedParametersView::iACombinedParametersView(QWidget *resultsWidget, int width, int height)
{
	results.Init(width, height, resultsWidget);
}

void iACombinedParametersView::Update()
{
	QPainter painter;
	QImage img = QImage((uchar*)results.buffer, results.bufferWidth, results.bufferHeight, QImage::Format_RGB32);
	painter.begin(results.pxmp);
	painter.drawImage(QRect(0, 0, results.pxmp->width(), results.pxmp->height()), img, QRect(0, 0, results.bufferWidth, results.bufferHeight));
	painter.end();
	results.paintWidget->update();
}
