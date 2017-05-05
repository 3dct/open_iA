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
#ifndef COMPARISONANDWEIGHTING_H
#define COMPARISONANDWEIGHTING_H

class QWidget;
class QPixmap;
class PaintWidget;
/**	\class ParamWidget.
\brief Class representing widget that shows height map of one parameter.

Contains: PaintWidget, pixmap that it uses, memory buffer used to fill pixmap and dimensions of that buffer.	
*/
struct ParamWidget
{
public:
	//methods
	ParamWidget();
	void Init(int pxmpWidth, int pxmpHeight, QWidget *widget);
	int AllocateBuffer(int width, int height);
	~ParamWidget();
	//props
	QPixmap *pxmp;
	PaintWidget *paintWidget;
	unsigned int * buffer;
	int bufferWidth, bufferHeight;
protected:
	bool initialized;
};
/**	\struct ParametersView.
\brief struct that uses all parameters.

Contains: ParamWidget for every parameter.
*/
struct ParametersView
{
	//meth
	ParametersView(int width, int height, QWidget *w1, QWidget *w2, QWidget *w3);
	virtual void Update();
	//props
	ParamWidget paramWidgets[3];
};
/**	\struct WeightingView.
\brief struct that uses all parameters and combination of all parameters.

Contains: ParamWidget for every parameter and one for combination results
*/
struct CombinedParametersView
{
	//meth
	CombinedParametersView(QWidget *resultsWidget, int width, int height);
	virtual void Update();
	//props
	ParamWidget results;
};
#endif//COMPARISONANDWEIGHTING_H
