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
#ifndef PLOT_H
#define PLOT_H


#include <math.h>
#include <qapplication.h>
#include <qwt3d_surfaceplot.h>
#include <qwt3d_function.h>
#include "raycast/include/common.h"
using namespace Raytracer;
/**	\class Plot.
	\brief Representing 3d plot.

	Qwt3D widget representing 3D plot. Default parameters are set in constructor.	
*/
class Plot : public Qwt3D::SurfacePlot
{
public:
	Plot(QWidget *   	 parent = 0,
		const QGLWidget *  	shareWidget = 0) :SurfacePlot(parent, shareWidget)
	{
		setTitleFont("", 8);
		setTitle("Parameter 3D plot");
		setRotation(30,0,15);
		setScale(1,1,1);
		setShift(0.15,0,0);
		setZoom(10);

		for (unsigned i=0; i!=coordinates()->axes.size(); ++i)
		{
			coordinates()->axes[i].setLabelFont("",8);
			coordinates()->axes[i].setNumberFont("",8);
			//coordinates()->axes[i].setMajors(7);
			coordinates()->axes[i].setMinors(5);
		}


		coordinates()->axes[Qwt3D::X1].setLabelString("x");
		coordinates()->axes[Qwt3D::Y1].setLabelString("y");
		coordinates()->axes[Qwt3D::Z1].setLabelString("F(x,y)"); // Omega - see http://www.unicode.org/charts/


		setCoordinateStyle(Qwt3D::FRAME);
		QGLColormap cmap = QGLColormap();
		cmap.setEntry(0, QColor(COL_RANGE_MIN_R,COL_RANGE_MIN_G,COL_RANGE_MIN_B));
		cmap.setEntry(1, QColor(COL_RANGE_MIN_R,COL_RANGE_MIN_G,COL_RANGE_MIN_B));
		this->setColormap(cmap);
		updateData();
		updateGL();
	}
};

#endif