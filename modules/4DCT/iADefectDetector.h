/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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
 
#ifndef IADEFECTDETECTOR_H
#define IADEFECTDETECTOR_H

class vtkImageData;
struct Endpoint;

class iADefectDetector
{
public:
	void findFiberPullOuts(vtkImageData * image, Endpoint endpoint, vtkImageData * poresSeg, vtkImageData * fibersSeg);
	void fillMovingImage(vtkImageData * refImg, vtkImageData * output, int point[3], double radius);

private:
	struct Plain
	{
	public:
		Plain(double norm[3], double point[3])
		{
			m_a = norm[0];
			m_b = norm[1];
			m_c = norm[2];
			m_d = norm[0] * point[0] + norm[1] * point[1] + norm[2] * point[2];
		}
		bool isPointInOuterSide(double point[3])
		{
			double val = point[0] * m_a + point[1] * m_b + point[2] * m_c - m_d;
			return (val > 0);
		}
	private:
		double m_a, m_b, m_c, m_d;
	};
};

#endif // IADEFECTDETECTOR_H