// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

class iAPaintWidget;

class QWidget;
class QPixmap;

//! Class representing widget that shows height map of one parameter.
//! Contains: PaintWidget, pixmap that it uses, memory buffer used to fill pixmap and dimensions of that buffer.
struct iAParamWidget
{
public:
	// methods
	iAParamWidget();
	~iAParamWidget();
	void Init(int pxmpWidth, int pxmpHeight, QWidget *widget);
	int AllocateBuffer(int width, int height);
	// properties
	QPixmap *pxmp;
	iAPaintWidget *paintWidget;
	unsigned int * buffer;
	int bufferWidth, bufferHeight;
private:
	bool initialized;
};

//! Struct that uses all parameters.
//! Contains: ParamWidget for every parameter.
struct iAParametersView final
{
	// methods
	iAParametersView(int width, int height, QWidget *w1, QWidget *w2, QWidget *w3);
	void Update();
	// properties
	iAParamWidget paramWidgets[3];
};

//! Struct that uses all parameters and combination of all parameters.
//! Contains: ParamWidget for every parameter and one for combination results
struct iACombinedParametersView final
{
	// methods
	iACombinedParametersView(QWidget *resultsWidget, int width, int height);
	void Update();
	// properties
	iAParamWidget results;
};
