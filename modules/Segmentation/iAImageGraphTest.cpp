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
#include "iAImageGraph.h"

#include "iASimpleTester.h"

std::ostream& operator<<(std::ostream& out, iAImageCoordinate const & c)
{
	out << "(x="<<c.x<<", y="<<c.y<<", z="<<c.z<<")";
	return out;
}


BEGIN_TEST

	iAImageGraph test2x2Graph(2, 2);
	TestEqual(static_cast<iAEdgeIndexType>(4), test2x2Graph.edgeCount());
	TestAssert(  test2x2Graph.containsEdge(0, 1) );
	TestAssert( !test2x2Graph.containsEdge(1, 2) );
	TestAssert(  test2x2Graph.containsEdge(2, 3) );
	TestAssert( !test2x2Graph.containsEdge(3, 4) );
	TestAssert(  test2x2Graph.containsEdge(0, 2) );
	TestAssert(  test2x2Graph.containsEdge(1, 3) );
	TestAssert( !test2x2Graph.containsEdge(2, 4) );
	TestAssert( !test2x2Graph.containsEdge(3, 5) );
	TestAssert( !test2x2Graph.containsEdge(0, 3) );
	TestAssert( !test2x2Graph.containsEdge(1, 4) );
	TestAssert( !test2x2Graph.containsEdge(2, 5) );
	TestAssert( !test2x2Graph.containsEdge(3, 6) );

	iAImageGraph test2x2x2Graph(2, 2, 2);
	TestEqual(static_cast<iAEdgeIndexType>(12), test2x2x2Graph.edgeCount());
	TestAssert(  test2x2x2Graph.containsEdge(0, 1) );
	TestAssert( !test2x2x2Graph.containsEdge(1, 2) );
	TestAssert(  test2x2x2Graph.containsEdge(2, 3) );
	TestAssert( !test2x2x2Graph.containsEdge(3, 4) );
	TestAssert(  test2x2x2Graph.containsEdge(4, 5) );
	TestAssert( !test2x2x2Graph.containsEdge(5, 6) );
	TestAssert(  test2x2x2Graph.containsEdge(6, 7) );
	TestAssert( !test2x2x2Graph.containsEdge(7, 8) );
	TestAssert( !test2x2x2Graph.containsEdge(8, 9) );
	TestAssert( !test2x2x2Graph.containsEdge(9, 10) );
	TestAssert( !test2x2x2Graph.containsEdge(10, 11) );
	TestAssert( !test2x2x2Graph.containsEdge(11, 12) );

	TestAssert(  test2x2x2Graph.containsEdge(0, 2) );
	TestAssert(  test2x2x2Graph.containsEdge(1, 3) );
	TestAssert( !test2x2x2Graph.containsEdge(2, 4) );
	TestAssert( !test2x2x2Graph.containsEdge(3, 5) );
	TestAssert(  test2x2x2Graph.containsEdge(4, 6) );
	TestAssert(  test2x2x2Graph.containsEdge(5, 7) );
	TestAssert( !test2x2x2Graph.containsEdge(6, 8) );
	TestAssert( !test2x2x2Graph.containsEdge(7, 9) );
	TestAssert( !test2x2x2Graph.containsEdge(8, 10) );
	TestAssert( !test2x2x2Graph.containsEdge(9, 11) );
	TestAssert( !test2x2x2Graph.containsEdge(10, 12) );
	TestAssert( !test2x2x2Graph.containsEdge(11, 13) );

	TestAssert(  test2x2x2Graph.containsEdge(0, 4) );
	TestAssert(  test2x2x2Graph.containsEdge(1, 5) );
	TestAssert(  test2x2x2Graph.containsEdge(2, 6) );
	TestAssert(  test2x2x2Graph.containsEdge(3, 7) );
	TestAssert( !test2x2x2Graph.containsEdge(4, 8) );
	TestAssert( !test2x2x2Graph.containsEdge(5, 9) );
	TestAssert( !test2x2x2Graph.containsEdge(6, 10) );
	TestAssert( !test2x2x2Graph.containsEdge(7, 11) );
	TestAssert( !test2x2x2Graph.containsEdge(8, 12) );
	TestAssert( !test2x2x2Graph.containsEdge(9, 13) );
	TestAssert( !test2x2x2Graph.containsEdge(10, 14) );
	TestAssert( !test2x2x2Graph.containsEdge(11, 15) );

	TestAssert( !test2x2x2Graph.containsEdge(0, 3) );
	TestAssert( !test2x2x2Graph.containsEdge(1, 4) );
	TestAssert( !test2x2x2Graph.containsEdge(2, 5) );
	TestAssert( !test2x2x2Graph.containsEdge(3, 6) );
	TestAssert( !test2x2x2Graph.containsEdge(4, 7) );
	TestAssert( !test2x2x2Graph.containsEdge(5, 8) );
	TestAssert( !test2x2x2Graph.containsEdge(6, 9) );
	TestAssert( !test2x2x2Graph.containsEdge(7, 10) );
	TestAssert( !test2x2x2Graph.containsEdge(8, 11) );
	TestAssert( !test2x2x2Graph.containsEdge(9, 12) );


	iAImageGraph test2x3x2Graph(2, 3, 2);
	TestEqual(static_cast<iAEdgeIndexType>(20), test2x3x2Graph.edgeCount());
	TestAssert(  test2x3x2Graph.containsEdge(0, 1) );
	TestAssert( !test2x3x2Graph.containsEdge(1, 2) );
	TestAssert(  test2x3x2Graph.containsEdge(2, 3) );
	TestAssert( !test2x3x2Graph.containsEdge(3, 4) );
	TestAssert(  test2x3x2Graph.containsEdge(4, 5) );
	TestAssert( !test2x3x2Graph.containsEdge(5, 6) );
	TestAssert(  test2x3x2Graph.containsEdge(6, 7) );
	TestAssert( !test2x3x2Graph.containsEdge(7, 8) );
	TestAssert(  test2x3x2Graph.containsEdge(8, 9) );
	TestAssert( !test2x3x2Graph.containsEdge(9, 10) );
	TestAssert(  test2x3x2Graph.containsEdge(10, 11) );
	TestAssert( !test2x3x2Graph.containsEdge(11, 12) );

	TestAssert(  test2x3x2Graph.containsEdge(0, 2) );
	TestAssert(  test2x3x2Graph.containsEdge(1, 3) );
	TestAssert(  test2x3x2Graph.containsEdge(2, 4) );
	TestAssert(  test2x3x2Graph.containsEdge(3, 5) );
	TestAssert( !test2x3x2Graph.containsEdge(4, 6) );
	TestAssert( !test2x3x2Graph.containsEdge(5, 7) );
	TestAssert(  test2x3x2Graph.containsEdge(6, 8) );
	TestAssert(  test2x3x2Graph.containsEdge(7, 9) );
	TestAssert(  test2x3x2Graph.containsEdge(8, 10) );
	TestAssert(  test2x3x2Graph.containsEdge(9, 11) );
	TestAssert( !test2x3x2Graph.containsEdge(10, 12) );
	TestAssert( !test2x3x2Graph.containsEdge(11, 13) );

	TestAssert(  test2x3x2Graph.containsEdge(0, 6) );
	TestAssert(  test2x3x2Graph.containsEdge(1, 7) );
	TestAssert(  test2x3x2Graph.containsEdge(2, 8) );
	TestAssert(  test2x3x2Graph.containsEdge(3, 9) );
	TestAssert(  test2x3x2Graph.containsEdge(4, 10) );
	TestAssert(  test2x3x2Graph.containsEdge(5, 11) );
	TestAssert( !test2x3x2Graph.containsEdge(6, 12) );
	TestAssert( !test2x3x2Graph.containsEdge(7, 13) );
	TestAssert( !test2x3x2Graph.containsEdge(8, 14) );
	TestAssert( !test2x3x2Graph.containsEdge(9, 15) );
	TestAssert( !test2x3x2Graph.containsEdge(10, 16) );
	TestAssert( !test2x3x2Graph.containsEdge(11, 17) );

	iAImageGraph test2x2x3Graph(2, 2, 3);
	TestEqual(static_cast<iAEdgeIndexType>(20), test2x2x3Graph.edgeCount());
	iAImageGraph test3x2x2Graph(3, 2, 2);
	TestEqual(static_cast<iAEdgeIndexType>(20), test3x2x2Graph.edgeCount());
	iAImageGraph test2x2x4Graph(2, 2, 4);
	TestEqual(static_cast<iAEdgeIndexType>(28), test2x2x4Graph.edgeCount());
	iAImageGraph test2x2x5Graph(2, 2, 5);
	TestEqual(static_cast<iAEdgeIndexType>(36), test2x2x5Graph.edgeCount());
	iAImageGraph test2x3x4Graph(2, 3, 4);
	TestEqual(static_cast<iAEdgeIndexType>(46), test2x3x4Graph.edgeCount());

	iAVoxelIndexType
		width = 2,
		height = 3,
		depth = 4;
	iAImageCoordConverter test2x3x4Conv(width, height, depth);
	TestEqual(iAImageCoordinate(0,0,0), test2x3x4Conv.coordinatesFromIndex(0));
	TestEqual(iAImageCoordinate(1,0,0), test2x3x4Conv.coordinatesFromIndex(1));
	TestEqual(iAImageCoordinate(width-1,0,0), test2x3x4Conv.coordinatesFromIndex(width-1));
	TestEqual(iAImageCoordinate(width-1,height-1,0), test2x3x4Conv.coordinatesFromIndex(width*height-1));
	TestEqual(iAImageCoordinate(0,0,1), test2x3x4Conv.coordinatesFromIndex(width*height));
	TestEqual(iAImageCoordinate(width-1,height-1,depth-1), test2x3x4Conv.coordinatesFromIndex(width*height*depth-1));

	iAImageCoordConverter test2x2Conv(2, 2);
	TestEqual( static_cast<iAVoxelIndexType>(0), test2x2Conv.indexFromCoordinates(iAImageCoordinate(0, 0, 0)));
	TestEqual( static_cast<iAVoxelIndexType>(1), test2x2Conv.indexFromCoordinates(iAImageCoordinate(1, 0, 0)));
	TestEqual( static_cast<iAVoxelIndexType>(2), test2x2Conv.indexFromCoordinates(iAImageCoordinate(0, 1, 0)));
	TestEqual( static_cast<iAVoxelIndexType>(3), test2x2Conv.indexFromCoordinates(iAImageCoordinate(1, 1, 0)));

	iAImageCoordConverter test2x2x2Conv(2, 2, 2);
	TestEqual( static_cast<iAVoxelIndexType>(0), test2x2x2Conv.indexFromCoordinates(iAImageCoordinate(0, 0, 0)));
	TestEqual( static_cast<iAVoxelIndexType>(1), test2x2x2Conv.indexFromCoordinates(iAImageCoordinate(1, 0, 0)));
	TestEqual( static_cast<iAVoxelIndexType>(2), test2x2x2Conv.indexFromCoordinates(iAImageCoordinate(0, 1, 0)));
	TestEqual( static_cast<iAVoxelIndexType>(3), test2x2x2Conv.indexFromCoordinates(iAImageCoordinate(1, 1, 0)));
	TestEqual( static_cast<iAVoxelIndexType>(4), test2x2x2Conv.indexFromCoordinates(iAImageCoordinate(0, 0, 1)));
	TestEqual( static_cast<iAVoxelIndexType>(5), test2x2x2Conv.indexFromCoordinates(iAImageCoordinate(1, 0, 1)));
	TestEqual( static_cast<iAVoxelIndexType>(6), test2x2x2Conv.indexFromCoordinates(iAImageCoordinate(0, 1, 1)));
	TestEqual( static_cast<iAVoxelIndexType>(7), test2x2x2Conv.indexFromCoordinates(iAImageCoordinate(1, 1, 1)));

	iAImageCoordConverter test2x2x5Conv(2, 2, 5);
	for (iAVoxelIndexType t=0; t<test2x2x5Conv.vertexCount(); ++t)
	{
		TestEqual( t, test2x2x5Conv.indexFromCoordinates(test2x2x5Conv.coordinatesFromIndex(t)) );
	}

	iAImageCoordConverter test3x4x5Conv(3, 4, 5);
	for (iAVoxelIndexType t=0; t<test3x4x5Conv.vertexCount(); ++t)
	{
		TestEqual( t, test3x4x5Conv.indexFromCoordinates(test3x4x5Conv.coordinatesFromIndex(t)) );
	}
	width = 2;
	height = 3;
	depth = 4;
	iAImageCoordConverter test2x3x4ColConv(width, height, depth, iAImageCoordinate::ColRowDepMajor);
	TestEqual(iAImageCoordinate(0,0,0),                    test2x3x4ColConv.coordinatesFromIndex(0));
	TestEqual(iAImageCoordinate(0,1,0),                    test2x3x4ColConv.coordinatesFromIndex(1));
	TestEqual(iAImageCoordinate(0,height-1,0),             test2x3x4ColConv.coordinatesFromIndex(height-1));
	TestEqual(iAImageCoordinate(width-1,height-1,0),       test2x3x4ColConv.coordinatesFromIndex(width*height-1));
	TestEqual(iAImageCoordinate(0,0,1),                    test2x3x4ColConv.coordinatesFromIndex(width*height));
	TestEqual(iAImageCoordinate(width-1,height-1,depth-1), test2x3x4ColConv.coordinatesFromIndex(width*height*depth-1));

	iAImageCoordConverter test2x2ColConv(2, 2, 1, iAImageCoordinate::ColRowDepMajor);
	TestEqual( static_cast<iAVoxelIndexType>(0), test2x2ColConv.indexFromCoordinates(iAImageCoordinate(0, 0, 0)));
	TestEqual( static_cast<iAVoxelIndexType>(1), test2x2ColConv.indexFromCoordinates(iAImageCoordinate(0, 1, 0)));
	TestEqual( static_cast<iAVoxelIndexType>(2), test2x2ColConv.indexFromCoordinates(iAImageCoordinate(1, 0, 0)));
	TestEqual( static_cast<iAVoxelIndexType>(3), test2x2ColConv.indexFromCoordinates(iAImageCoordinate(1, 1, 0)));

	iAImageCoordConverter test2x2x2ColConv(2, 2, 2, iAImageCoordinate::ColRowDepMajor);
	TestEqual( static_cast<iAVoxelIndexType>(0), test2x2x2ColConv.indexFromCoordinates(iAImageCoordinate(0, 0, 0)));
	TestEqual( static_cast<iAVoxelIndexType>(1), test2x2x2ColConv.indexFromCoordinates(iAImageCoordinate(0, 1, 0)));
	TestEqual( static_cast<iAVoxelIndexType>(2), test2x2x2ColConv.indexFromCoordinates(iAImageCoordinate(1, 0, 0)));
	TestEqual( static_cast<iAVoxelIndexType>(3), test2x2x2ColConv.indexFromCoordinates(iAImageCoordinate(1, 1, 0)));
	TestEqual( static_cast<iAVoxelIndexType>(4), test2x2x2ColConv.indexFromCoordinates(iAImageCoordinate(0, 0, 1)));
	TestEqual( static_cast<iAVoxelIndexType>(5), test2x2x2ColConv.indexFromCoordinates(iAImageCoordinate(0, 1, 1)));
	TestEqual( static_cast<iAVoxelIndexType>(6), test2x2x2ColConv.indexFromCoordinates(iAImageCoordinate(1, 0, 1)));
	TestEqual( static_cast<iAVoxelIndexType>(7), test2x2x2ColConv.indexFromCoordinates(iAImageCoordinate(1, 1, 1)));


	// TEST Moore Neighbourhood:
	iAImageGraph test2x2GraphMoore(2, 2, 1, iAImageCoordinate::RowColDepMajor, iAImageGraph::nbhMoore);
	TestEqual(static_cast<iAEdgeIndexType>(6), test2x2GraphMoore.edgeCount());
	TestAssert( !test2x2GraphMoore.containsEdge(0, 0) );
	TestAssert( !test2x2GraphMoore.containsEdge(1, 1) );
	TestAssert( !test2x2GraphMoore.containsEdge(2, 2) );
	TestAssert( !test2x2GraphMoore.containsEdge(3, 3) );
	TestAssert(  test2x2GraphMoore.containsEdge(0, 1) );
	TestAssert(  test2x2GraphMoore.containsEdge(1, 2) );
	TestAssert(  test2x2GraphMoore.containsEdge(2, 3) );
	TestAssert( !test2x2GraphMoore.containsEdge(3, 4) );
	TestAssert(  test2x2GraphMoore.containsEdge(0, 2) );
	TestAssert(  test2x2GraphMoore.containsEdge(1, 3) );
	TestAssert( !test2x2GraphMoore.containsEdge(2, 4) );
	TestAssert( !test2x2GraphMoore.containsEdge(3, 5) );
	TestAssert(  test2x2GraphMoore.containsEdge(0, 3) );
	TestAssert( !test2x2GraphMoore.containsEdge(1, 4) );
	TestAssert( !test2x2GraphMoore.containsEdge(2, 5) );
	TestAssert( !test2x2GraphMoore.containsEdge(3, 6) );

	// test symmetry:
	TestAssert(  test2x2GraphMoore.containsEdge(1, 0) );
	TestAssert(  test2x2GraphMoore.containsEdge(2, 1) );
	TestAssert(  test2x2GraphMoore.containsEdge(3, 2) );
	TestAssert( !test2x2GraphMoore.containsEdge(4, 3) );
	TestAssert(  test2x2GraphMoore.containsEdge(2, 0) );
	TestAssert(  test2x2GraphMoore.containsEdge(3, 1) );
	TestAssert( !test2x2GraphMoore.containsEdge(4, 2) );
	TestAssert( !test2x2GraphMoore.containsEdge(5, 3) );
	TestAssert(  test2x2GraphMoore.containsEdge(3, 0) );
	TestAssert( !test2x2GraphMoore.containsEdge(4, 1) );
	TestAssert( !test2x2GraphMoore.containsEdge(5, 2) );
	TestAssert( !test2x2GraphMoore.containsEdge(6, 3) );


	iAImageGraph test2x2x2GraphMoore(2, 2, 2, iAImageCoordinate::RowColDepMajor, iAImageGraph::nbhMoore);
	TestEqual(static_cast<iAEdgeIndexType>(28), test2x2x2GraphMoore.edgeCount());
	TestAssert(  test2x2x2GraphMoore.containsEdge( 0,  1) );
	TestAssert(  test2x2x2GraphMoore.containsEdge( 1,  2) );
	TestAssert(  test2x2x2GraphMoore.containsEdge( 2,  3) );
	TestAssert(  test2x2x2GraphMoore.containsEdge( 3,  4) );
	TestAssert(  test2x2x2GraphMoore.containsEdge( 4,  5) );
	TestAssert(  test2x2x2GraphMoore.containsEdge( 5,  6) );
	TestAssert(  test2x2x2GraphMoore.containsEdge( 6,  7) );
	TestAssert( !test2x2x2GraphMoore.containsEdge( 7,  8) );
	TestAssert( !test2x2x2GraphMoore.containsEdge( 8,  9) );
	TestAssert( !test2x2x2GraphMoore.containsEdge( 9, 10) );
	TestAssert( !test2x2x2GraphMoore.containsEdge(10, 11) );
	TestAssert( !test2x2x2GraphMoore.containsEdge(11, 12) );

	TestAssert(  test2x2x2GraphMoore.containsEdge( 0,  2) );
	TestAssert(  test2x2x2GraphMoore.containsEdge( 1,  3) );
	TestAssert(  test2x2x2GraphMoore.containsEdge( 2,  4) );
	TestAssert(  test2x2x2GraphMoore.containsEdge( 3,  5) );
	TestAssert(  test2x2x2GraphMoore.containsEdge( 4,  6) );
	TestAssert(  test2x2x2GraphMoore.containsEdge( 5,  7) );
	TestAssert( !test2x2x2GraphMoore.containsEdge( 6,  8) );
	TestAssert( !test2x2x2GraphMoore.containsEdge( 7,  9) );
	TestAssert( !test2x2x2GraphMoore.containsEdge( 8, 10) );
	TestAssert( !test2x2x2GraphMoore.containsEdge( 9, 11) );
	TestAssert( !test2x2x2GraphMoore.containsEdge(10, 12) );
	TestAssert( !test2x2x2GraphMoore.containsEdge(11, 13) );

	TestAssert(  test2x2x2GraphMoore.containsEdge( 0,  3) );
	TestAssert(  test2x2x2GraphMoore.containsEdge( 1,  4) );
	TestAssert(  test2x2x2GraphMoore.containsEdge( 2,  5) );
	TestAssert(  test2x2x2GraphMoore.containsEdge( 3,  6) );
	TestAssert(  test2x2x2GraphMoore.containsEdge( 4,  7) );
	TestAssert( !test2x2x2GraphMoore.containsEdge( 5,  8) );
	TestAssert( !test2x2x2GraphMoore.containsEdge( 6,  9) );
	TestAssert( !test2x2x2GraphMoore.containsEdge( 7, 10) );
	TestAssert( !test2x2x2GraphMoore.containsEdge( 8, 11) );
	TestAssert( !test2x2x2GraphMoore.containsEdge( 9, 12) );

	TestAssert(  test2x2x2GraphMoore.containsEdge( 0,  4) );
	TestAssert(  test2x2x2GraphMoore.containsEdge( 1,  5) );
	TestAssert(  test2x2x2GraphMoore.containsEdge( 2,  6) );
	TestAssert(  test2x2x2GraphMoore.containsEdge( 3,  7) );
	TestAssert( !test2x2x2GraphMoore.containsEdge( 4,  8) );
	TestAssert( !test2x2x2GraphMoore.containsEdge( 5,  9) );
	TestAssert( !test2x2x2GraphMoore.containsEdge( 6, 10) );
	TestAssert( !test2x2x2GraphMoore.containsEdge( 7, 11) );
	TestAssert( !test2x2x2GraphMoore.containsEdge( 8, 12) );
	TestAssert( !test2x2x2GraphMoore.containsEdge( 9, 13) );
	TestAssert( !test2x2x2GraphMoore.containsEdge(10, 14) );
	TestAssert( !test2x2x2GraphMoore.containsEdge(11, 15) );

	TestAssert(  test2x2x2GraphMoore.containsEdge( 0,  5) );
	TestAssert(  test2x2x2GraphMoore.containsEdge( 1,  6) );
	TestAssert(  test2x2x2GraphMoore.containsEdge( 2,  7) );
	TestAssert( !test2x2x2GraphMoore.containsEdge( 3,  8) );
	TestAssert( !test2x2x2GraphMoore.containsEdge( 4,  9) );
	TestAssert( !test2x2x2GraphMoore.containsEdge( 5, 10) );
	TestAssert( !test2x2x2GraphMoore.containsEdge( 6, 11) );
	TestAssert( !test2x2x2GraphMoore.containsEdge( 7, 12) );
	TestAssert( !test2x2x2GraphMoore.containsEdge( 8, 13) );
	TestAssert( !test2x2x2GraphMoore.containsEdge( 9, 14) );

	TestAssert(  test2x2x2GraphMoore.containsEdge( 0,  6) );
	TestAssert(  test2x2x2GraphMoore.containsEdge( 1,  7) );
	TestAssert( !test2x2x2GraphMoore.containsEdge( 2,  8) );
	TestAssert( !test2x2x2GraphMoore.containsEdge( 3,  9) );
	TestAssert( !test2x2x2GraphMoore.containsEdge( 4, 10) );
	TestAssert( !test2x2x2GraphMoore.containsEdge( 5, 11) );
	TestAssert( !test2x2x2GraphMoore.containsEdge( 6, 12) );
	TestAssert( !test2x2x2GraphMoore.containsEdge( 7, 13) );
	TestAssert( !test2x2x2GraphMoore.containsEdge( 8, 14) );
	TestAssert( !test2x2x2GraphMoore.containsEdge( 9, 15) );

	TestAssert(  test2x2x2GraphMoore.containsEdge( 0,  7) );
	TestAssert( !test2x2x2GraphMoore.containsEdge( 1,  8) );
	TestAssert( !test2x2x2GraphMoore.containsEdge( 2,  9) );
	TestAssert( !test2x2x2GraphMoore.containsEdge( 3, 10) );
	TestAssert( !test2x2x2GraphMoore.containsEdge( 4, 11) );
	TestAssert( !test2x2x2GraphMoore.containsEdge( 5, 12) );
	TestAssert( !test2x2x2GraphMoore.containsEdge( 6, 13) );
	TestAssert( !test2x2x2GraphMoore.containsEdge( 7, 14) );
	TestAssert( !test2x2x2GraphMoore.containsEdge( 8, 15) );
	TestAssert( !test2x2x2GraphMoore.containsEdge( 9, 16) );

	iAImageGraph test2x3x2GraphMoore(2, 3, 2, iAImageCoordinate::RowColDepMajor, iAImageGraph::nbhMoore);
	TestEqual(static_cast<iAEdgeIndexType>(50), test2x3x2GraphMoore.edgeCount());
	TestAssert(  test2x3x2GraphMoore.containsEdge( 0,  1) );
	TestAssert(  test2x3x2GraphMoore.containsEdge( 1,  2) );
	TestAssert(  test2x3x2GraphMoore.containsEdge( 2,  3) );
	TestAssert(  test2x3x2GraphMoore.containsEdge( 3,  4) );
	TestAssert(  test2x3x2GraphMoore.containsEdge( 4,  5) );
	TestAssert( !test2x3x2GraphMoore.containsEdge( 5,  6) );
	TestAssert(  test2x3x2GraphMoore.containsEdge( 6,  7) );
	TestAssert(  test2x3x2GraphMoore.containsEdge( 7,  8) );
	TestAssert(  test2x3x2GraphMoore.containsEdge( 8,  9) );
	TestAssert(  test2x3x2GraphMoore.containsEdge( 9, 10) );
	TestAssert(  test2x3x2GraphMoore.containsEdge(10, 11) );
	TestAssert( !test2x3x2GraphMoore.containsEdge(11, 12) );

	TestAssert(  test2x3x2GraphMoore.containsEdge( 0,  2) );
	TestAssert(  test2x3x2GraphMoore.containsEdge( 1,  3) );
	TestAssert(  test2x3x2GraphMoore.containsEdge( 2,  4) );
	TestAssert(  test2x3x2GraphMoore.containsEdge( 3,  5) );
	TestAssert( !test2x3x2GraphMoore.containsEdge( 4,  6) );
	TestAssert( !test2x3x2GraphMoore.containsEdge( 5,  7) );
	TestAssert(  test2x3x2GraphMoore.containsEdge( 6,  8) );
	TestAssert(  test2x3x2GraphMoore.containsEdge( 7,  9) );
	TestAssert(  test2x3x2GraphMoore.containsEdge( 8, 10) );
	TestAssert(  test2x3x2GraphMoore.containsEdge( 9, 11) );
	TestAssert( !test2x3x2GraphMoore.containsEdge(10, 12) );
	TestAssert( !test2x3x2GraphMoore.containsEdge(11, 13) );

	TestAssert(  test2x3x2GraphMoore.containsEdge( 0,  3) );
	TestAssert( !test2x3x2GraphMoore.containsEdge( 1,  4) );
	TestAssert(  test2x3x2GraphMoore.containsEdge( 2,  5) );
	TestAssert(  test2x3x2GraphMoore.containsEdge( 3,  6) );
	TestAssert( !test2x3x2GraphMoore.containsEdge( 4,  7) );
	TestAssert(  test2x3x2GraphMoore.containsEdge( 5,  8) );
	TestAssert(  test2x3x2GraphMoore.containsEdge( 6,  9) );
	TestAssert( !test2x3x2GraphMoore.containsEdge( 7, 10) );
	TestAssert(  test2x3x2GraphMoore.containsEdge( 8, 11) );
	TestAssert( !test2x3x2GraphMoore.containsEdge( 9, 12) );
	TestAssert( !test2x3x2GraphMoore.containsEdge(10, 13) );
	TestAssert( !test2x3x2GraphMoore.containsEdge(11, 14) );

	TestAssert( !test2x3x2GraphMoore.containsEdge( 0,  4) );
	TestAssert( !test2x3x2GraphMoore.containsEdge( 1,  5) );
	TestAssert(  test2x3x2GraphMoore.containsEdge( 2,  6) );
	TestAssert(  test2x3x2GraphMoore.containsEdge( 3,  7) );
	TestAssert(  test2x3x2GraphMoore.containsEdge( 4,  8) );
	TestAssert(  test2x3x2GraphMoore.containsEdge( 5,  9) );
	TestAssert( !test2x3x2GraphMoore.containsEdge( 6, 10) );
	TestAssert( !test2x3x2GraphMoore.containsEdge( 7, 11) );
	TestAssert( !test2x3x2GraphMoore.containsEdge( 8, 12) );
	TestAssert( !test2x3x2GraphMoore.containsEdge( 9, 13) );
	TestAssert( !test2x3x2GraphMoore.containsEdge(10, 14) );
	TestAssert( !test2x3x2GraphMoore.containsEdge(11, 15) );

	TestAssert( !test2x3x2GraphMoore.containsEdge( 0,  5) );
	TestAssert(  test2x3x2GraphMoore.containsEdge( 1,  6) );
	TestAssert(  test2x3x2GraphMoore.containsEdge( 2,  7) );
	TestAssert(  test2x3x2GraphMoore.containsEdge( 3,  8) );
	TestAssert(  test2x3x2GraphMoore.containsEdge( 4,  9) );
	TestAssert(  test2x3x2GraphMoore.containsEdge( 5, 10) );
	TestAssert( !test2x3x2GraphMoore.containsEdge( 6, 11) );
	TestAssert( !test2x3x2GraphMoore.containsEdge( 7, 12) );
	TestAssert( !test2x3x2GraphMoore.containsEdge( 8, 13) );
	TestAssert( !test2x3x2GraphMoore.containsEdge( 9, 14) );
	TestAssert( !test2x3x2GraphMoore.containsEdge(10, 15) );
	TestAssert( !test2x3x2GraphMoore.containsEdge(11, 16) );

	TestAssert(  test2x3x2GraphMoore.containsEdge( 0,  6) );
	TestAssert(  test2x3x2GraphMoore.containsEdge( 1,  7) );
	TestAssert(  test2x3x2GraphMoore.containsEdge( 2,  8) );
	TestAssert(  test2x3x2GraphMoore.containsEdge( 3,  9) );
	TestAssert(  test2x3x2GraphMoore.containsEdge( 4, 10) );
	TestAssert(  test2x3x2GraphMoore.containsEdge( 5, 11) );
	TestAssert( !test2x3x2GraphMoore.containsEdge( 6, 12) );
	TestAssert( !test2x3x2GraphMoore.containsEdge( 7, 13) );
	TestAssert( !test2x3x2GraphMoore.containsEdge( 8, 14) );
	TestAssert( !test2x3x2GraphMoore.containsEdge( 9, 15) );
	TestAssert( !test2x3x2GraphMoore.containsEdge(10, 16) );
	TestAssert( !test2x3x2GraphMoore.containsEdge(11, 17) );

	TestAssert(  test2x3x2GraphMoore.containsEdge( 0,  7) );
	TestAssert(  test2x3x2GraphMoore.containsEdge( 1,  8) );
	TestAssert(  test2x3x2GraphMoore.containsEdge( 2,  9) );
	TestAssert(  test2x3x2GraphMoore.containsEdge( 3, 10) );
	TestAssert(  test2x3x2GraphMoore.containsEdge( 4, 11) );
	TestAssert( !test2x3x2GraphMoore.containsEdge( 5, 12) );
	TestAssert( !test2x3x2GraphMoore.containsEdge( 6, 13) );
	TestAssert( !test2x3x2GraphMoore.containsEdge( 7, 14) );
	TestAssert( !test2x3x2GraphMoore.containsEdge( 8, 15) );
	TestAssert( !test2x3x2GraphMoore.containsEdge( 9, 16) );
	TestAssert( !test2x3x2GraphMoore.containsEdge(10, 17) );
	TestAssert( !test2x3x2GraphMoore.containsEdge(11, 18) );

	TestAssert(  test2x3x2GraphMoore.containsEdge( 0,  8) );
	TestAssert(  test2x3x2GraphMoore.containsEdge( 1,  9) );
	TestAssert(  test2x3x2GraphMoore.containsEdge( 2, 10) );
	TestAssert(  test2x3x2GraphMoore.containsEdge( 3, 11) );
	TestAssert( !test2x3x2GraphMoore.containsEdge( 4, 12) );
	TestAssert( !test2x3x2GraphMoore.containsEdge( 5, 13) );
	TestAssert( !test2x3x2GraphMoore.containsEdge( 6, 14) );
	TestAssert( !test2x3x2GraphMoore.containsEdge( 7, 15) );
	TestAssert( !test2x3x2GraphMoore.containsEdge( 8, 16) );
	TestAssert( !test2x3x2GraphMoore.containsEdge( 9, 17) );
	TestAssert( !test2x3x2GraphMoore.containsEdge(10, 18) );
	TestAssert( !test2x3x2GraphMoore.containsEdge(11, 19) );

	TestAssert(  test2x3x2GraphMoore.containsEdge( 0,  9) );
	TestAssert( !test2x3x2GraphMoore.containsEdge( 1, 10) );
	TestAssert(  test2x3x2GraphMoore.containsEdge( 2, 11) );
	TestAssert( !test2x3x2GraphMoore.containsEdge( 3, 12) );
	TestAssert( !test2x3x2GraphMoore.containsEdge( 4, 13) );
	TestAssert( !test2x3x2GraphMoore.containsEdge( 5, 14) );
	TestAssert( !test2x3x2GraphMoore.containsEdge( 6, 15) );
	TestAssert( !test2x3x2GraphMoore.containsEdge( 7, 16) );
	TestAssert( !test2x3x2GraphMoore.containsEdge( 8, 17) );
	TestAssert( !test2x3x2GraphMoore.containsEdge( 9, 18) );
	TestAssert( !test2x3x2GraphMoore.containsEdge(10, 19) );
	TestAssert( !test2x3x2GraphMoore.containsEdge(11, 20) );

	TestAssert( !test2x3x2GraphMoore.containsEdge( 0, 10) );
	TestAssert( !test2x3x2GraphMoore.containsEdge( 1, 11) );
	TestAssert( !test2x3x2GraphMoore.containsEdge( 2, 12) );
	TestAssert( !test2x3x2GraphMoore.containsEdge( 3, 13) );
	TestAssert( !test2x3x2GraphMoore.containsEdge( 4, 14) );
	TestAssert( !test2x3x2GraphMoore.containsEdge( 5, 15) );
	TestAssert( !test2x3x2GraphMoore.containsEdge( 6, 16) );
	TestAssert( !test2x3x2GraphMoore.containsEdge( 7, 17) );
	TestAssert( !test2x3x2GraphMoore.containsEdge( 8, 18) );
	TestAssert( !test2x3x2GraphMoore.containsEdge( 9, 19) );
	TestAssert( !test2x3x2GraphMoore.containsEdge(10, 20) );
	TestAssert( !test2x3x2GraphMoore.containsEdge(11, 21) );

	TestAssert( !test2x3x2GraphMoore.containsEdge( 0, 11) );
	TestAssert( !test2x3x2GraphMoore.containsEdge( 1, 12) );
	TestAssert( !test2x3x2GraphMoore.containsEdge( 2, 13) );
	TestAssert( !test2x3x2GraphMoore.containsEdge( 3, 14) );
	TestAssert( !test2x3x2GraphMoore.containsEdge( 4, 15) );
	TestAssert( !test2x3x2GraphMoore.containsEdge( 5, 16) );
	TestAssert( !test2x3x2GraphMoore.containsEdge( 6, 17) );
	TestAssert( !test2x3x2GraphMoore.containsEdge( 7, 18) );
	TestAssert( !test2x3x2GraphMoore.containsEdge( 8, 19) );
	TestAssert( !test2x3x2GraphMoore.containsEdge( 9, 20) );
	TestAssert( !test2x3x2GraphMoore.containsEdge(10, 21) );
	TestAssert( !test2x3x2GraphMoore.containsEdge(11, 22) );


	// Test index orderings:
	for(int i=0; i<2; ++i)
	{
		iAImageCoordinate::iAIndexOrdering indexOrdering = static_cast<iAImageCoordinate::iAIndexOrdering>(i);
		iAImageGraph test2x3x2GraphMooreColRow(2, 3, 2, indexOrdering, iAImageGraph::nbhMoore);

		TestEqual(static_cast<iAEdgeIndexType>(50), test2x3x2GraphMooreColRow.edgeCount());
		// left/lower/behind:
		TestAssert(test2x3x2GraphMooreColRow.containsEdge(iAImageCoordinate(0, 0, 0), iAImageCoordinate(0, 0, 1)));
		TestAssert(test2x3x2GraphMooreColRow.containsEdge(iAImageCoordinate(0, 0, 0), iAImageCoordinate(0, 1, 0)));
		TestAssert(test2x3x2GraphMooreColRow.containsEdge(iAImageCoordinate(0, 0, 0), iAImageCoordinate(1, 0, 0)));

		TestAssert(test2x3x2GraphMooreColRow.containsEdge(iAImageCoordinate(1, 0, 0), iAImageCoordinate(1, 1, 0)));
		TestAssert(test2x3x2GraphMooreColRow.containsEdge(iAImageCoordinate(1, 0, 0), iAImageCoordinate(1, 0, 1)));

		TestAssert(test2x3x2GraphMooreColRow.containsEdge(iAImageCoordinate(0, 1, 0), iAImageCoordinate(0, 1, 1)));
		TestAssert(test2x3x2GraphMooreColRow.containsEdge(iAImageCoordinate(0, 1, 0), iAImageCoordinate(0, 2, 0)));
		TestAssert(test2x3x2GraphMooreColRow.containsEdge(iAImageCoordinate(0, 1, 0), iAImageCoordinate(1, 1, 0)));

		TestAssert(test2x3x2GraphMooreColRow.containsEdge(iAImageCoordinate(1, 1, 0), iAImageCoordinate(1, 2, 0)));
		TestAssert(test2x3x2GraphMooreColRow.containsEdge(iAImageCoordinate(1, 1, 0), iAImageCoordinate(1, 1, 1)));

		TestAssert(test2x3x2GraphMooreColRow.containsEdge(iAImageCoordinate(0, 2, 0), iAImageCoordinate(0, 2, 1)));
		TestAssert(test2x3x2GraphMooreColRow.containsEdge(iAImageCoordinate(0, 2, 0), iAImageCoordinate(1, 2, 0)));

		TestAssert(test2x3x2GraphMooreColRow.containsEdge(iAImageCoordinate(1, 2, 0), iAImageCoordinate(1, 2, 1)));

		TestAssert(test2x3x2GraphMooreColRow.containsEdge(iAImageCoordinate(0, 0, 1), iAImageCoordinate(0, 1, 1)));
		TestAssert(test2x3x2GraphMooreColRow.containsEdge(iAImageCoordinate(0, 0, 1), iAImageCoordinate(1, 0, 1)));

		TestAssert(test2x3x2GraphMooreColRow.containsEdge(iAImageCoordinate(1, 0, 1), iAImageCoordinate(1, 1, 1)));

		TestAssert(test2x3x2GraphMooreColRow.containsEdge(iAImageCoordinate(0, 1, 1), iAImageCoordinate(0, 2, 1)));
		TestAssert(test2x3x2GraphMooreColRow.containsEdge(iAImageCoordinate(0, 1, 1), iAImageCoordinate(1, 1, 1)));

		TestAssert(test2x3x2GraphMooreColRow.containsEdge(iAImageCoordinate(1, 1, 1), iAImageCoordinate(1, 2, 1)));

		TestAssert(test2x3x2GraphMooreColRow.containsEdge(iAImageCoordinate(0, 2, 1), iAImageCoordinate(1, 2, 1)));

		// "plane" diagonals:
		TestAssert(test2x3x2GraphMooreColRow.containsEdge(iAImageCoordinate(0, 0, 0), iAImageCoordinate(0, 1, 1)));
		TestAssert(test2x3x2GraphMooreColRow.containsEdge(iAImageCoordinate(0, 1, 0), iAImageCoordinate(0, 0, 1)));

		TestAssert(test2x3x2GraphMooreColRow.containsEdge(iAImageCoordinate(0, 0, 0), iAImageCoordinate(1, 0, 1)));
		TestAssert(test2x3x2GraphMooreColRow.containsEdge(iAImageCoordinate(1, 0, 0), iAImageCoordinate(0, 0, 1)));

		TestAssert(test2x3x2GraphMooreColRow.containsEdge(iAImageCoordinate(0, 0, 0), iAImageCoordinate(1, 1, 0)));
		TestAssert(test2x3x2GraphMooreColRow.containsEdge(iAImageCoordinate(1, 0, 0), iAImageCoordinate(0, 1, 0)));

		TestAssert(test2x3x2GraphMooreColRow.containsEdge(iAImageCoordinate(1, 0, 0), iAImageCoordinate(1, 1, 1)));
		TestAssert(test2x3x2GraphMooreColRow.containsEdge(iAImageCoordinate(1, 1, 0), iAImageCoordinate(1, 0, 1)));

		TestAssert(test2x3x2GraphMooreColRow.containsEdge(iAImageCoordinate(0, 1, 0), iAImageCoordinate(0, 2, 1)));
		TestAssert(test2x3x2GraphMooreColRow.containsEdge(iAImageCoordinate(0, 2, 0), iAImageCoordinate(0, 1, 1)));

		TestAssert(test2x3x2GraphMooreColRow.containsEdge(iAImageCoordinate(0, 1, 0), iAImageCoordinate(1, 1, 1)));
		TestAssert(test2x3x2GraphMooreColRow.containsEdge(iAImageCoordinate(1, 1, 0), iAImageCoordinate(0, 1, 1)));

		TestAssert(test2x3x2GraphMooreColRow.containsEdge(iAImageCoordinate(0, 1, 0), iAImageCoordinate(1, 2, 0)));
		TestAssert(test2x3x2GraphMooreColRow.containsEdge(iAImageCoordinate(0, 2, 0), iAImageCoordinate(1, 1, 0)));

		TestAssert(test2x3x2GraphMooreColRow.containsEdge(iAImageCoordinate(1, 1, 0), iAImageCoordinate(1, 2, 1)));
		TestAssert(test2x3x2GraphMooreColRow.containsEdge(iAImageCoordinate(1, 2, 0), iAImageCoordinate(1, 1, 1)));

		TestAssert(test2x3x2GraphMooreColRow.containsEdge(iAImageCoordinate(0, 2, 0), iAImageCoordinate(1, 2, 1)));
		TestAssert(test2x3x2GraphMooreColRow.containsEdge(iAImageCoordinate(1, 2, 0), iAImageCoordinate(0, 2, 1)));

		TestAssert(test2x3x2GraphMooreColRow.containsEdge(iAImageCoordinate(0, 0, 1), iAImageCoordinate(1, 1, 1)));
		TestAssert(test2x3x2GraphMooreColRow.containsEdge(iAImageCoordinate(1, 0, 1), iAImageCoordinate(0, 1, 1)));

		TestAssert(test2x3x2GraphMooreColRow.containsEdge(iAImageCoordinate(0, 1, 1), iAImageCoordinate(1, 2, 1)));
		TestAssert(test2x3x2GraphMooreColRow.containsEdge(iAImageCoordinate(1, 1, 1), iAImageCoordinate(0, 2, 1)));

		// "cross" diagonals:
		TestAssert(test2x3x2GraphMooreColRow.containsEdge(iAImageCoordinate(0, 0, 0), iAImageCoordinate(1, 1, 1)));
		TestAssert(test2x3x2GraphMooreColRow.containsEdge(iAImageCoordinate(0, 0, 1), iAImageCoordinate(1, 1, 0)));
		TestAssert(test2x3x2GraphMooreColRow.containsEdge(iAImageCoordinate(0, 1, 0), iAImageCoordinate(1, 0, 1)));
		TestAssert(test2x3x2GraphMooreColRow.containsEdge(iAImageCoordinate(1, 0, 0), iAImageCoordinate(0, 1, 1)));

		TestAssert(test2x3x2GraphMooreColRow.containsEdge(iAImageCoordinate(0, 1, 0), iAImageCoordinate(1, 2, 1)));
		TestAssert(test2x3x2GraphMooreColRow.containsEdge(iAImageCoordinate(0, 1, 1), iAImageCoordinate(1, 2, 0)));
		TestAssert(test2x3x2GraphMooreColRow.containsEdge(iAImageCoordinate(0, 2, 0), iAImageCoordinate(1, 1, 1)));
		TestAssert(test2x3x2GraphMooreColRow.containsEdge(iAImageCoordinate(1, 1, 0), iAImageCoordinate(0, 2, 1)));
	}



END_TEST
