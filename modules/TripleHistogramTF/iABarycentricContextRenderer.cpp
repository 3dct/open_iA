/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iABarycentricContextRenderer.h"

#include "BarycentricTriangle.h"

#include <vtkVersion.h>

#include <QPainter>
#include <QImage>

static const QImage::Format IMAGE_FORMAT = QImage::Format::Format_ARGB32;
static const int ONE_DIV_THREE = 1.0 / 3.0;
static const int GRAY_VALUE_MIN = 48;
static const int GRAY_VALUE_INTERVAL = 255 - GRAY_VALUE_MIN;

iABarycentricContextRenderer::iABarycentricContextRenderer()
{
	
}

iABarycentricContextRenderer::~iABarycentricContextRenderer()
{

}

void iABarycentricContextRenderer::paintContext(QPainter &p)
{
	if (!m_image.isNull()) {
		p.drawImage(m_imagePoint, m_image);
	}
}

void iABarycentricContextRenderer::setModalities(vtkSmartPointer<vtkImageData> d1, vtkSmartPointer<vtkImageData> d2, vtkSmartPointer<vtkImageData> d3, BarycentricTriangle triangle)
{
	calculateCoordinates(d1, d2, d3);
	updateTriangle(triangle);
}

void iABarycentricContextRenderer::setTriangle(BarycentricTriangle triangle)
{
	updateTriangle(triangle);
}

bool iABarycentricContextRenderer::canPaint()
{
	return !m_image.isNull();
}


// PRIVATE -----------------------------------------------------------------------------------------------

// TODO good? we're assuming each vtkImageData has the same dimensions
void iABarycentricContextRenderer::calculateCoordinates(vtkSmartPointer<vtkImageData> d1, vtkSmartPointer<vtkImageData> d2, vtkSmartPointer<vtkImageData> d3)
{
	m_barycentricCoordinates = vtkSmartPointer<vtkImageData>::New();

	int* dims = d1->GetDimensions();
	m_barycentricCoordinates->SetDimensions(dims[0], dims[1], dims[2]);

	// Source: https://www.vtk.org/Wiki/VTK/Examples/Cxx/ImageData/IterateImageData (28.08.2018)
	// 2 components because barycentric coordinates have 2 degrees of freedom (alpha and beta; gamma is dependent on the others)
#if VTK_MAJOR_VERSION <= 5
	m_barycentricCoordinates->SetNumberOfScalarComponents(2);
	m_barycentricCoordinates->SetScalarTypeToDouble();
#else
	m_barycentricCoordinates->AllocateScalars(VTK_DOUBLE, 2);
#endif

	double rangea[2], rangeb[2], rangec[2];
	d1->GetScalarRange(rangea);
	d2->GetScalarRange(rangeb);
	d3->GetScalarRange(rangec);
	rangea[1] -= rangea[0];
	rangeb[1] -= rangeb[0];
	rangec[1] -= rangec[0];

	//int numComponents = d1->GetNumberOfScalarComponents(); // TODO use this in another stage to make sure the modalities being used have 1 scalar component

	double a, b, c, sum, *values;
	for (int z = 0; z < dims[2]; z++) {
		for (int y = 0; y < dims[1]; y++) {
			for (int x = 0; x < dims[0]; x++) {

				//a = static_cast<double*>(d1->GetScalarPointer(x, y, z))[0];
				//b = static_cast<double*>(d2->GetScalarPointer(x, y, z))[0];
				//c = static_cast<double*>(d3->GetScalarPointer(x, y, z))[0];

				a = d1->GetScalarComponentAsDouble(x, y, z, 0);
				b = d2->GetScalarComponentAsDouble(x, y, z, 0);
				c = d3->GetScalarComponentAsDouble(x, y, z, 0);

				if (qIsNaN(a)) a = 0; else a = (a - rangea[0]) / rangea[1];
				if (qIsNaN(b)) b = 0; else b = (b - rangeb[0]) / rangeb[1];
				if (qIsNaN(c)) c = 0; else c = (c - rangec[0]) / rangec[1];

				sum = a + b + c;

				values = static_cast<double*>(m_barycentricCoordinates->GetScalarPointer(x, y, z));
				if (sum == 0) {
					values[0] = ONE_DIV_THREE;
					values[1] = ONE_DIV_THREE;
				} else {
					values[0] = a / sum;
					values[1] = b / sum;
				}

			}
		}
	}
}

void iABarycentricContextRenderer::updateTriangle(BarycentricTriangle triangle)
{
	if (!m_barycentricCoordinates) {
		return;
	}

	int width, height;
	{
		int xMin = qMin(triangle.getXa(), qMin(triangle.getXb(), triangle.getXc()));
		int yMin = qMin(triangle.getYa(), qMin(triangle.getYb(), triangle.getYc()));

		int xMax = qMax(triangle.getXa(), qMax(triangle.getXb(), triangle.getXc()));
		int yMax = qMax(triangle.getYa(), qMax(triangle.getYb(), triangle.getYc()));

		width = xMax - xMin + 1;
		height = yMax - yMin + 1;

		m_imagePoint = QPoint(xMin, yMin);

		if (!m_image.isNull() && m_image.width() == width && m_image.height() == height) {
			return;
		}

		m_image = QImage(width, height, IMAGE_FORMAT);

		triangle = triangle - m_imagePoint;
	}

	drawImage(triangle, width, height);
}

void iABarycentricContextRenderer::drawImage(BarycentricTriangle triangle, int width, int height)
{
	int widthMinusOne = width - 1;
	int heightMinusOne = height - 1;

	// TODO: a vector of vectors or another vtkImageData?
	QVector<QVector<int>> counts = QVector<QVector<int>>(height, QVector<int>(width, 0));
	int max = 0;
	int min = std::numeric_limits<int>::infinity();
	int maxx, maxy;

	// Go though the volume (m_barycentricCoordinates) and write to the counts 2D-vector
	int *dims = m_barycentricCoordinates->GetDimensions();
	double *values;
	for (int z = 0; z < dims[2]; z++) {
		for (int y = 0; y < dims[1]; y++) {
			for (int x = 0; x < dims[0]; x++) {
				
				values = static_cast<double*>(m_barycentricCoordinates->GetScalarPointer(x, y, z));
				QPoint cartesian = triangle.getCartesianCoordinates(values[0], values[1]);

				// TODO do this in a better way? why is it happenning? numeric imprecision?
				int cx = cartesian.x();
				cx = cx < 0 ? 0 : (cx >= width ? widthMinusOne : cx);

				int cy = cartesian.y();
				cy = cy < 0 ? 0 : (cy >= height ? heightMinusOne : cy);

				int count = counts[cy][cx] + 1;
				if (count > max) {
					max = count;
					maxx = cx;
					maxy = cy;
				}
				if (count < min) {
					min = count;
				}
				counts[cy][cx] = count;

			}
		}
	}

	// Go through every pixel and set the pixel color based on the counts 2D-vector
	//double c = (double) GRAY_VALUE_INTERVAL / (double) max;
	double k = (double)GRAY_VALUE_INTERVAL / (double)log(max);
	int grayValue, count;
	QPoint p;
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			p = QPoint(x, y);
			if (triangle.contains(x, y)) {
				count = counts[y][x];
				if (count > 0) {

					//grayValue = 255 - ((count * c) + GRAY_VALUE_MIN);
					grayValue = 255 - (k * log(count) + GRAY_VALUE_MIN);
					m_image.setPixelColor(p, QColor(grayValue, grayValue, grayValue));

				}
				else {
					m_image.setPixelColor(p, Qt::white);
				}
			} else {
				m_image.setPixelColor(p, Qt::transparent);
			}
		}
	}
}