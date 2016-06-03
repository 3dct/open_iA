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
 
#ifndef IAGRAPHDRAWER_H
#define IAGRAPHDRAWER_H

#include "open_iA_Core_export.h"

#include <map>
#include <vector>

using namespace std;



typedef long	idType;

struct graphVertex {
	graphVertex() {}

	graphVertex(float posX, float posY, int rank) {
		this->positionX = posX;
		this->positionY = posY;
		this->rank = rank;
	}

	float	positionX;
	float	positionY;
	float	radius;
	int		rank;
};

struct graphEdge {
	// edge has a direction from vertex 1 to vertex 2
	idType vert1, vert2;
};

typedef map<idType, graphVertex*>	verticesMap;
typedef map<idType, graphEdge*>		edgesMap;
typedef vector<vector<idType> >		orderType;
typedef vector<vector<idType> >		edgesTableType;

class iAGraphDrawerImpl;

class open_iA_Core_API iAGraphDrawer {
public:
				iAGraphDrawer();
				~iAGraphDrawer();
	
	// modifiers
	idType		addVertex();
	idType		addVertex(graphVertex vert);
	idType		addEdge(idType v1, idType v2);
	bool		modifyVertex(idType vertId, graphVertex value);

	void			setMaxIteration(int maxIter);

	// operations
	bool			isHeaderVerticle(idType vert);
	vector<idType>	getParentsVertices(idType vert);
	vector<idType>	getChildsVertices(idType vert);
	int				getMaximumRank();
/*	int				getMaxNumberOfVertices();*/
	int				getNumberOfVertices(int rankNum);

// 	void			getAverageParentsPosition(idType vertId, float* averagePos);
// 	void			getAverageChildsPosition(idType vertId, float* averagePos);
// 	void			getAveragePosition(idType vertId, float* averagePos);
// 	void			getAveragePosition(vector<idType> vertices, float* averagePos);
	void			shiftRank(int rankNumber, bool forwardDir);


	const graphVertex*		getVertex(idType vert);
	const graphEdge*		getEdge(idType edge);

	void		ordering();
	void		setPosition();

private:
	idType		getNewUniqueIndex();

	void		initialOrder(orderType* order);
	void		calculateEdgesTable(edgesTableType* edgesTable);
	void		addVerticesToOrder(idType headerVert, orderType* order);

	void		wmedian(orderType* order, bool forwardTraversal);
	float		medianValue(idType vert, orderType* order, bool forwardTraversal);
	vector<int>	getAdjacentPositions(idType vert, orderType* order, bool forwardTraversal);
	int			crossing(orderType* order, edgesTableType* edgesTable);
	int			crossing(orderType* order, edgesTableType* edgesTable, size_t rank);
	int			crossing(orderType* order, edgesTableType* edgesTable, size_t startRank, size_t endRank);
	size_t		vertexPosition(idType vert, orderType* order);
	void		transpose(orderType* order, edgesTableType* edgesTable);

	void		setInitialPositions();

	static idType	m_uniqueIndex;		// do not use this variable directly
	iAGraphDrawerImpl* m_pImpl;
};

#endif