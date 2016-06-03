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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 
#ifndef IAENDPOINTSEXTRACTOR_H
#define IAENDPOINTSEXTRACTOR_H
// std
#include <vector>

class vtkImageData;

struct Endpoint
{
public:
	void SetPosition(int * pos)
	{
		m_position[0] = pos[0];
		m_position[1] = pos[1];
		m_position[2] = pos[2];
	}
	const int * GetPosition()
	{
		return m_position;
	}
	void GetPosition(int * pos)
	{
		pos[0] = m_position[0];
		pos[1] = m_position[1];
		pos[2] = m_position[2];
	}
	void SetDirection(double * dir)
	{
		m_direction[0] = dir[0]; 
		m_direction[1] = dir[1];
		m_direction[2] = dir[2];
	}
	const double * GetDirection()
	{
		return m_direction;
	}
private:
	int		m_position[3];
	double	m_direction[3];
};

class EndpointsExtractor
{
public:
	void		extract();
public:
	bool		extract(vtkImageData * image, std::vector<Endpoint>& endpoints);
private:
	bool		ranAway(vtkImageData * image, vtkImageData * stepMap, const int pos[3], int pathLength, int outPosition[3]);
	bool		getRemotePoint(vtkImageData * image, const int pos[3], int pathLength, int outPosition[3]);
	bool		isWithinExtent(const int coordinate[3], const int exntent[6]);
	bool		isAnEndpoint(vtkImageData * image, const int pos[3]);
	Endpoint	procceedFiber(vtkImageData * image, vtkImageData * stepsMap, const int pos[3]);
	void		findTheMostFarPoint(vtkImageData * image, vtkImageData * stepsMap, const int pos[3], int * endpoint);
};

#endif // IAENDPOINTSEXTRACTOR_H