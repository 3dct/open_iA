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

#include "iAImageGraph.h"

#include "iASimpleTester.h"

std::ostream& operator<<(std::ostream& out, iAImageCoordinate const & c)
{
	out << "(x="<<c.x<<", y="<<c.y<<", z="<<c.z<<")";
	return out;
}


BEGIN_TEST

	iAImageGraph test2x2Graph(2, 2);
	TestEqual(static_cast<iAEdgeIndexType>(4), test2x2Graph.GetEdgeCount());
	TestAssert(  test2x2Graph.ContainsEdge(0, 1) );
	TestAssert( !test2x2Graph.ContainsEdge(1, 2) );
	TestAssert(  test2x2Graph.ContainsEdge(2, 3) );
	TestAssert( !test2x2Graph.ContainsEdge(3, 4) );
	TestAssert(  test2x2Graph.ContainsEdge(0, 2) );
	TestAssert(  test2x2Graph.ContainsEdge(1, 3) );
	TestAssert( !test2x2Graph.ContainsEdge(2, 4) );
	TestAssert( !test2x2Graph.ContainsEdge(3, 5) );
	TestAssert( !test2x2Graph.ContainsEdge(0, 3) );
	TestAssert( !test2x2Graph.ContainsEdge(1, 4) );
	TestAssert( !test2x2Graph.ContainsEdge(2, 5) );
	TestAssert( !test2x2Graph.ContainsEdge(3, 6) );

	iAImageGraph test2x2x2Graph(2, 2, 2);
	TestEqual(static_cast<iAEdgeIndexType>(12), test2x2x2Graph.GetEdgeCount());
	TestAssert(  test2x2x2Graph.ContainsEdge(0, 1) );
	TestAssert( !test2x2x2Graph.ContainsEdge(1, 2) );
	TestAssert(  test2x2x2Graph.ContainsEdge(2, 3) );
	TestAssert( !test2x2x2Graph.ContainsEdge(3, 4) );
	TestAssert(  test2x2x2Graph.ContainsEdge(4, 5) );
	TestAssert( !test2x2x2Graph.ContainsEdge(5, 6) );
	TestAssert(  test2x2x2Graph.ContainsEdge(6, 7) );
	TestAssert( !test2x2x2Graph.ContainsEdge(7, 8) );
	TestAssert( !test2x2x2Graph.ContainsEdge(8, 9) );
	TestAssert( !test2x2x2Graph.ContainsEdge(9, 10) );
	TestAssert( !test2x2x2Graph.ContainsEdge(10, 11) );
	TestAssert( !test2x2x2Graph.ContainsEdge(11, 12) );

	TestAssert(  test2x2x2Graph.ContainsEdge(0, 2) );
	TestAssert(  test2x2x2Graph.ContainsEdge(1, 3) );
	TestAssert( !test2x2x2Graph.ContainsEdge(2, 4) );
	TestAssert( !test2x2x2Graph.ContainsEdge(3, 5) );
	TestAssert(  test2x2x2Graph.ContainsEdge(4, 6) );
	TestAssert(  test2x2x2Graph.ContainsEdge(5, 7) );
	TestAssert( !test2x2x2Graph.ContainsEdge(6, 8) );
	TestAssert( !test2x2x2Graph.ContainsEdge(7, 9) );
	TestAssert( !test2x2x2Graph.ContainsEdge(8, 10) );
	TestAssert( !test2x2x2Graph.ContainsEdge(9, 11) );
	TestAssert( !test2x2x2Graph.ContainsEdge(10, 12) );
	TestAssert( !test2x2x2Graph.ContainsEdge(11, 13) );

	TestAssert(  test2x2x2Graph.ContainsEdge(0, 4) );
	TestAssert(  test2x2x2Graph.ContainsEdge(1, 5) );
	TestAssert(  test2x2x2Graph.ContainsEdge(2, 6) );
	TestAssert(  test2x2x2Graph.ContainsEdge(3, 7) );
	TestAssert( !test2x2x2Graph.ContainsEdge(4, 8) );
	TestAssert( !test2x2x2Graph.ContainsEdge(5, 9) );
	TestAssert( !test2x2x2Graph.ContainsEdge(6, 10) );
	TestAssert( !test2x2x2Graph.ContainsEdge(7, 11) );
	TestAssert( !test2x2x2Graph.ContainsEdge(8, 12) );
	TestAssert( !test2x2x2Graph.ContainsEdge(9, 13) );
	TestAssert( !test2x2x2Graph.ContainsEdge(10, 14) );
	TestAssert( !test2x2x2Graph.ContainsEdge(11, 15) );

	TestAssert( !test2x2x2Graph.ContainsEdge(0, 3) );
	TestAssert( !test2x2x2Graph.ContainsEdge(1, 4) );
	TestAssert( !test2x2x2Graph.ContainsEdge(2, 5) );
	TestAssert( !test2x2x2Graph.ContainsEdge(3, 6) );
	TestAssert( !test2x2x2Graph.ContainsEdge(4, 7) );
	TestAssert( !test2x2x2Graph.ContainsEdge(5, 8) );
	TestAssert( !test2x2x2Graph.ContainsEdge(6, 9) );
	TestAssert( !test2x2x2Graph.ContainsEdge(7, 10) );
	TestAssert( !test2x2x2Graph.ContainsEdge(8, 11) );
	TestAssert( !test2x2x2Graph.ContainsEdge(9, 12) );


	iAImageGraph test2x3x2Graph(2, 3, 2);
	TestEqual(static_cast<iAEdgeIndexType>(20), test2x3x2Graph.GetEdgeCount());
	TestAssert(  test2x3x2Graph.ContainsEdge(0, 1) );
	TestAssert( !test2x3x2Graph.ContainsEdge(1, 2) );
	TestAssert(  test2x3x2Graph.ContainsEdge(2, 3) );
	TestAssert( !test2x3x2Graph.ContainsEdge(3, 4) );
	TestAssert(  test2x3x2Graph.ContainsEdge(4, 5) );
	TestAssert( !test2x3x2Graph.ContainsEdge(5, 6) );
	TestAssert(  test2x3x2Graph.ContainsEdge(6, 7) );
	TestAssert( !test2x3x2Graph.ContainsEdge(7, 8) );
	TestAssert(  test2x3x2Graph.ContainsEdge(8, 9) );
	TestAssert( !test2x3x2Graph.ContainsEdge(9, 10) );
	TestAssert(  test2x3x2Graph.ContainsEdge(10, 11) );
	TestAssert( !test2x3x2Graph.ContainsEdge(11, 12) );

	TestAssert(  test2x3x2Graph.ContainsEdge(0, 2) );
	TestAssert(  test2x3x2Graph.ContainsEdge(1, 3) );
	TestAssert(  test2x3x2Graph.ContainsEdge(2, 4) );
	TestAssert(  test2x3x2Graph.ContainsEdge(3, 5) );
	TestAssert( !test2x3x2Graph.ContainsEdge(4, 6) );
	TestAssert( !test2x3x2Graph.ContainsEdge(5, 7) );
	TestAssert(  test2x3x2Graph.ContainsEdge(6, 8) );
	TestAssert(  test2x3x2Graph.ContainsEdge(7, 9) );
	TestAssert(  test2x3x2Graph.ContainsEdge(8, 10) );
	TestAssert(  test2x3x2Graph.ContainsEdge(9, 11) );
	TestAssert( !test2x3x2Graph.ContainsEdge(10, 12) );
	TestAssert( !test2x3x2Graph.ContainsEdge(11, 13) );

	TestAssert(  test2x3x2Graph.ContainsEdge(0, 6) );
	TestAssert(  test2x3x2Graph.ContainsEdge(1, 7) );
	TestAssert(  test2x3x2Graph.ContainsEdge(2, 8) );
	TestAssert(  test2x3x2Graph.ContainsEdge(3, 9) );
	TestAssert(  test2x3x2Graph.ContainsEdge(4, 10) );
	TestAssert(  test2x3x2Graph.ContainsEdge(5, 11) );
	TestAssert( !test2x3x2Graph.ContainsEdge(6, 12) );
	TestAssert( !test2x3x2Graph.ContainsEdge(7, 13) );
	TestAssert( !test2x3x2Graph.ContainsEdge(8, 14) );
	TestAssert( !test2x3x2Graph.ContainsEdge(9, 15) );
	TestAssert( !test2x3x2Graph.ContainsEdge(10, 16) );
	TestAssert( !test2x3x2Graph.ContainsEdge(11, 17) );



	iAImageGraph test2x2x3Graph(2, 2, 3);
	TestEqual(static_cast<iAEdgeIndexType>(20), test2x2x3Graph.GetEdgeCount());
	iAImageGraph test3x2x2Graph(3, 2, 2);
	TestEqual(static_cast<iAEdgeIndexType>(20), test3x2x2Graph.GetEdgeCount());
	iAImageGraph test2x2x4Graph(2, 2, 4);
	TestEqual(static_cast<iAEdgeIndexType>(28), test2x2x4Graph.GetEdgeCount());
	iAImageGraph test2x2x5Graph(2, 2, 5);
	TestEqual(static_cast<iAEdgeIndexType>(36), test2x2x5Graph.GetEdgeCount());
	iAImageGraph test2x3x4Graph(2, 3, 4);
	TestEqual(static_cast<iAEdgeIndexType>(46), test2x3x4Graph.GetEdgeCount());

	iAVoxelIndexType
		width = 2,
		height = 3,
		depth = 4;
	iAImageCoordConverter test2x3x4Conv(width, height, depth);
	TestEqual(iAImageCoordinate(0,0,0), test2x3x4Conv.GetCoordinatesFromIndex(0));
	TestEqual(iAImageCoordinate(1,0,0), test2x3x4Conv.GetCoordinatesFromIndex(1));
	TestEqual(iAImageCoordinate(width-1,0,0), test2x3x4Conv.GetCoordinatesFromIndex(width-1));
	TestEqual(iAImageCoordinate(width-1,height-1,0), test2x3x4Conv.GetCoordinatesFromIndex(width*height-1));
	TestEqual(iAImageCoordinate(0,0,1), test2x3x4Conv.GetCoordinatesFromIndex(width*height));
	TestEqual(iAImageCoordinate(width-1,height-1,depth-1), test2x3x4Conv.GetCoordinatesFromIndex(width*height*depth-1));

	iAImageCoordConverter test2x2Conv(2, 2);
	TestEqual( static_cast<iAVoxelIndexType>(0), test2x2Conv.GetIndexFromCoordinates(iAImageCoordinate(0, 0, 0)));
	TestEqual( static_cast<iAVoxelIndexType>(1), test2x2Conv.GetIndexFromCoordinates(iAImageCoordinate(1, 0, 0)));
	TestEqual( static_cast<iAVoxelIndexType>(2), test2x2Conv.GetIndexFromCoordinates(iAImageCoordinate(0, 1, 0)));
	TestEqual( static_cast<iAVoxelIndexType>(3), test2x2Conv.GetIndexFromCoordinates(iAImageCoordinate(1, 1, 0)));

	iAImageCoordConverter test2x2x2Conv(2, 2, 2);
	TestEqual( static_cast<iAVoxelIndexType>(0), test2x2x2Conv.GetIndexFromCoordinates(iAImageCoordinate(0, 0, 0)));
	TestEqual( static_cast<iAVoxelIndexType>(1), test2x2x2Conv.GetIndexFromCoordinates(iAImageCoordinate(1, 0, 0)));
	TestEqual( static_cast<iAVoxelIndexType>(2), test2x2x2Conv.GetIndexFromCoordinates(iAImageCoordinate(0, 1, 0)));
	TestEqual( static_cast<iAVoxelIndexType>(3), test2x2x2Conv.GetIndexFromCoordinates(iAImageCoordinate(1, 1, 0)));
	TestEqual( static_cast<iAVoxelIndexType>(4), test2x2x2Conv.GetIndexFromCoordinates(iAImageCoordinate(0, 0, 1)));
	TestEqual( static_cast<iAVoxelIndexType>(5), test2x2x2Conv.GetIndexFromCoordinates(iAImageCoordinate(1, 0, 1)));
	TestEqual( static_cast<iAVoxelIndexType>(6), test2x2x2Conv.GetIndexFromCoordinates(iAImageCoordinate(0, 1, 1)));
	TestEqual( static_cast<iAVoxelIndexType>(7), test2x2x2Conv.GetIndexFromCoordinates(iAImageCoordinate(1, 1, 1)));

	iAImageCoordConverter test2x2x5Conv(2, 2, 5);
	for (iAVoxelIndexType t=0; t<test2x2x5Conv.GetVertexCount(); ++t)
	{
		TestEqual( t, test2x2x5Conv.GetIndexFromCoordinates(test2x2x5Conv.GetCoordinatesFromIndex(t)) );
	}

	iAImageCoordConverter test3x4x5Conv(3, 4, 5);
	for (iAVoxelIndexType t=0; t<test3x4x5Conv.GetVertexCount(); ++t)
	{
		TestEqual( t, test3x4x5Conv.GetIndexFromCoordinates(test3x4x5Conv.GetCoordinatesFromIndex(t)) );
	}
	width = 2;
	height = 3;
	depth = 4;
	iAImageCoordConverter test2x3x4ColConv(width, height, depth, iAImageCoordinate::ColRowDepMajor);
	TestEqual(iAImageCoordinate(0,0,0),                    test2x3x4ColConv.GetCoordinatesFromIndex(0));
	TestEqual(iAImageCoordinate(0,1,0),                    test2x3x4ColConv.GetCoordinatesFromIndex(1));
	TestEqual(iAImageCoordinate(0,height-1,0),                    test2x3x4ColConv.GetCoordinatesFromIndex(height-1));
	TestEqual(iAImageCoordinate(width-1,height-1,0),       test2x3x4ColConv.GetCoordinatesFromIndex(width*height-1));
	TestEqual(iAImageCoordinate(0,0,1),                    test2x3x4ColConv.GetCoordinatesFromIndex(width*height));
	TestEqual(iAImageCoordinate(width-1,height-1,depth-1), test2x3x4ColConv.GetCoordinatesFromIndex(width*height*depth-1));

	iAImageCoordConverter test2x2ColConv(2, 2, 1, iAImageCoordinate::ColRowDepMajor);
	TestEqual( static_cast<iAVoxelIndexType>(0), test2x2ColConv.GetIndexFromCoordinates(iAImageCoordinate(0, 0, 0)));
	TestEqual( static_cast<iAVoxelIndexType>(1), test2x2ColConv.GetIndexFromCoordinates(iAImageCoordinate(0, 1, 0)));
	TestEqual( static_cast<iAVoxelIndexType>(2), test2x2ColConv.GetIndexFromCoordinates(iAImageCoordinate(1, 0, 0)));
	TestEqual( static_cast<iAVoxelIndexType>(3), test2x2ColConv.GetIndexFromCoordinates(iAImageCoordinate(1, 1, 0)));

	iAImageCoordConverter test2x2x2ColConv(2, 2, 2, iAImageCoordinate::ColRowDepMajor);
	TestEqual( static_cast<iAVoxelIndexType>(0), test2x2x2ColConv.GetIndexFromCoordinates(iAImageCoordinate(0, 0, 0)));
	TestEqual( static_cast<iAVoxelIndexType>(1), test2x2x2ColConv.GetIndexFromCoordinates(iAImageCoordinate(0, 1, 0)));
	TestEqual( static_cast<iAVoxelIndexType>(2), test2x2x2ColConv.GetIndexFromCoordinates(iAImageCoordinate(1, 0, 0)));
	TestEqual( static_cast<iAVoxelIndexType>(3), test2x2x2ColConv.GetIndexFromCoordinates(iAImageCoordinate(1, 1, 0)));
	TestEqual( static_cast<iAVoxelIndexType>(4), test2x2x2ColConv.GetIndexFromCoordinates(iAImageCoordinate(0, 0, 1)));
	TestEqual( static_cast<iAVoxelIndexType>(5), test2x2x2ColConv.GetIndexFromCoordinates(iAImageCoordinate(0, 1, 1)));
	TestEqual( static_cast<iAVoxelIndexType>(6), test2x2x2ColConv.GetIndexFromCoordinates(iAImageCoordinate(1, 0, 1)));
	TestEqual( static_cast<iAVoxelIndexType>(7), test2x2x2ColConv.GetIndexFromCoordinates(iAImageCoordinate(1, 1, 1)));


	// TEST Moore Neighbourhood:
	iAImageGraph test2x2GraphMoore(2, 2, 1, iAImageCoordinate::RowColDepMajor, iAImageGraph::nbhMoore);
	TestEqual(static_cast<iAEdgeIndexType>(6), test2x2GraphMoore.GetEdgeCount());
	TestAssert( !test2x2GraphMoore.ContainsEdge(0, 0) );
	TestAssert( !test2x2GraphMoore.ContainsEdge(1, 1) );
	TestAssert( !test2x2GraphMoore.ContainsEdge(2, 2) );
	TestAssert( !test2x2GraphMoore.ContainsEdge(3, 3) );
	TestAssert(  test2x2GraphMoore.ContainsEdge(0, 1) );
	TestAssert(  test2x2GraphMoore.ContainsEdge(1, 2) );
	TestAssert(  test2x2GraphMoore.ContainsEdge(2, 3) );
	TestAssert( !test2x2GraphMoore.ContainsEdge(3, 4) );
	TestAssert(  test2x2GraphMoore.ContainsEdge(0, 2) );
	TestAssert(  test2x2GraphMoore.ContainsEdge(1, 3) );
	TestAssert( !test2x2GraphMoore.ContainsEdge(2, 4) );
	TestAssert( !test2x2GraphMoore.ContainsEdge(3, 5) );
	TestAssert(  test2x2GraphMoore.ContainsEdge(0, 3) );
	TestAssert( !test2x2GraphMoore.ContainsEdge(1, 4) );
	TestAssert( !test2x2GraphMoore.ContainsEdge(2, 5) );
	TestAssert( !test2x2GraphMoore.ContainsEdge(3, 6) );

	// test symmetry:
	TestAssert(  test2x2GraphMoore.ContainsEdge(1, 0) );
	TestAssert(  test2x2GraphMoore.ContainsEdge(2, 1) );
	TestAssert(  test2x2GraphMoore.ContainsEdge(3, 2) );
	TestAssert( !test2x2GraphMoore.ContainsEdge(4, 3) );
	TestAssert(  test2x2GraphMoore.ContainsEdge(2, 0) );
	TestAssert(  test2x2GraphMoore.ContainsEdge(3, 1) );
	TestAssert( !test2x2GraphMoore.ContainsEdge(4, 2) );
	TestAssert( !test2x2GraphMoore.ContainsEdge(5, 3) );
	TestAssert(  test2x2GraphMoore.ContainsEdge(3, 0) );
	TestAssert( !test2x2GraphMoore.ContainsEdge(4, 1) );
	TestAssert( !test2x2GraphMoore.ContainsEdge(5, 2) );
	TestAssert( !test2x2GraphMoore.ContainsEdge(6, 3) );


	iAImageGraph test2x2x2GraphMoore(2, 2, 2, iAImageCoordinate::RowColDepMajor, iAImageGraph::nbhMoore);
	TestEqual(static_cast<iAEdgeIndexType>(28), test2x2x2GraphMoore.GetEdgeCount());
	TestAssert(  test2x2x2GraphMoore.ContainsEdge( 0,  1) );
	TestAssert(  test2x2x2GraphMoore.ContainsEdge( 1,  2) );
	TestAssert(  test2x2x2GraphMoore.ContainsEdge( 2,  3) );
	TestAssert(  test2x2x2GraphMoore.ContainsEdge( 3,  4) );
	TestAssert(  test2x2x2GraphMoore.ContainsEdge( 4,  5) );
	TestAssert(  test2x2x2GraphMoore.ContainsEdge( 5,  6) );
	TestAssert(  test2x2x2GraphMoore.ContainsEdge( 6,  7) );
	TestAssert( !test2x2x2GraphMoore.ContainsEdge( 7,  8) );
	TestAssert( !test2x2x2GraphMoore.ContainsEdge( 8,  9) );
	TestAssert( !test2x2x2GraphMoore.ContainsEdge( 9, 10) );
	TestAssert( !test2x2x2GraphMoore.ContainsEdge(10, 11) );
	TestAssert( !test2x2x2GraphMoore.ContainsEdge(11, 12) );

	TestAssert(  test2x2x2GraphMoore.ContainsEdge( 0,  2) );
	TestAssert(  test2x2x2GraphMoore.ContainsEdge( 1,  3) );
	TestAssert(  test2x2x2GraphMoore.ContainsEdge( 2,  4) );
	TestAssert(  test2x2x2GraphMoore.ContainsEdge( 3,  5) );
	TestAssert(  test2x2x2GraphMoore.ContainsEdge( 4,  6) );
	TestAssert(  test2x2x2GraphMoore.ContainsEdge( 5,  7) );
	TestAssert( !test2x2x2GraphMoore.ContainsEdge( 6,  8) );
	TestAssert( !test2x2x2GraphMoore.ContainsEdge( 7,  9) );
	TestAssert( !test2x2x2GraphMoore.ContainsEdge( 8, 10) );
	TestAssert( !test2x2x2GraphMoore.ContainsEdge( 9, 11) );
	TestAssert( !test2x2x2GraphMoore.ContainsEdge(10, 12) );
	TestAssert( !test2x2x2GraphMoore.ContainsEdge(11, 13) );

	TestAssert(  test2x2x2GraphMoore.ContainsEdge( 0,  3) );
	TestAssert(  test2x2x2GraphMoore.ContainsEdge( 1,  4) );
	TestAssert(  test2x2x2GraphMoore.ContainsEdge( 2,  5) );
	TestAssert(  test2x2x2GraphMoore.ContainsEdge( 3,  6) );
	TestAssert(  test2x2x2GraphMoore.ContainsEdge( 4,  7) );
	TestAssert( !test2x2x2GraphMoore.ContainsEdge( 5,  8) );
	TestAssert( !test2x2x2GraphMoore.ContainsEdge( 6,  9) );
	TestAssert( !test2x2x2GraphMoore.ContainsEdge( 7, 10) );
	TestAssert( !test2x2x2GraphMoore.ContainsEdge( 8, 11) );
	TestAssert( !test2x2x2GraphMoore.ContainsEdge( 9, 12) );

	TestAssert(  test2x2x2GraphMoore.ContainsEdge( 0,  4) );
	TestAssert(  test2x2x2GraphMoore.ContainsEdge( 1,  5) );
	TestAssert(  test2x2x2GraphMoore.ContainsEdge( 2,  6) );
	TestAssert(  test2x2x2GraphMoore.ContainsEdge( 3,  7) );
	TestAssert( !test2x2x2GraphMoore.ContainsEdge( 4,  8) );
	TestAssert( !test2x2x2GraphMoore.ContainsEdge( 5,  9) );
	TestAssert( !test2x2x2GraphMoore.ContainsEdge( 6, 10) );
	TestAssert( !test2x2x2GraphMoore.ContainsEdge( 7, 11) );
	TestAssert( !test2x2x2GraphMoore.ContainsEdge( 8, 12) );
	TestAssert( !test2x2x2GraphMoore.ContainsEdge( 9, 13) );
	TestAssert( !test2x2x2GraphMoore.ContainsEdge(10, 14) );
	TestAssert( !test2x2x2GraphMoore.ContainsEdge(11, 15) );

	TestAssert(  test2x2x2GraphMoore.ContainsEdge( 0,  5) );
	TestAssert(  test2x2x2GraphMoore.ContainsEdge( 1,  6) );
	TestAssert(  test2x2x2GraphMoore.ContainsEdge( 2,  7) );
	TestAssert( !test2x2x2GraphMoore.ContainsEdge( 3,  8) );
	TestAssert( !test2x2x2GraphMoore.ContainsEdge( 4,  9) );
	TestAssert( !test2x2x2GraphMoore.ContainsEdge( 5, 10) );
	TestAssert( !test2x2x2GraphMoore.ContainsEdge( 6, 11) );
	TestAssert( !test2x2x2GraphMoore.ContainsEdge( 7, 12) );
	TestAssert( !test2x2x2GraphMoore.ContainsEdge( 8, 13) );
	TestAssert( !test2x2x2GraphMoore.ContainsEdge( 9, 14) );

	TestAssert(  test2x2x2GraphMoore.ContainsEdge( 0,  6) );
	TestAssert(  test2x2x2GraphMoore.ContainsEdge( 1,  7) );
	TestAssert( !test2x2x2GraphMoore.ContainsEdge( 2,  8) );
	TestAssert( !test2x2x2GraphMoore.ContainsEdge( 3,  9) );
	TestAssert( !test2x2x2GraphMoore.ContainsEdge( 4, 10) );
	TestAssert( !test2x2x2GraphMoore.ContainsEdge( 5, 11) );
	TestAssert( !test2x2x2GraphMoore.ContainsEdge( 6, 12) );
	TestAssert( !test2x2x2GraphMoore.ContainsEdge( 7, 13) );
	TestAssert( !test2x2x2GraphMoore.ContainsEdge( 8, 14) );
	TestAssert( !test2x2x2GraphMoore.ContainsEdge( 9, 15) );

	TestAssert(  test2x2x2GraphMoore.ContainsEdge( 0,  7) );
	TestAssert( !test2x2x2GraphMoore.ContainsEdge( 1,  8) );
	TestAssert( !test2x2x2GraphMoore.ContainsEdge( 2,  9) );
	TestAssert( !test2x2x2GraphMoore.ContainsEdge( 3, 10) );
	TestAssert( !test2x2x2GraphMoore.ContainsEdge( 4, 11) );
	TestAssert( !test2x2x2GraphMoore.ContainsEdge( 5, 12) );
	TestAssert( !test2x2x2GraphMoore.ContainsEdge( 6, 13) );
	TestAssert( !test2x2x2GraphMoore.ContainsEdge( 7, 14) );
	TestAssert( !test2x2x2GraphMoore.ContainsEdge( 8, 15) );
	TestAssert( !test2x2x2GraphMoore.ContainsEdge( 9, 16) );



	iAImageGraph test2x3x2GraphMoore(2, 3, 2, iAImageCoordinate::RowColDepMajor, iAImageGraph::nbhMoore);
	TestEqual(static_cast<iAEdgeIndexType>(50), test2x3x2GraphMoore.GetEdgeCount());
	TestAssert(  test2x3x2GraphMoore.ContainsEdge( 0,  1) );
	TestAssert(  test2x3x2GraphMoore.ContainsEdge( 1,  2) );
	TestAssert(  test2x3x2GraphMoore.ContainsEdge( 2,  3) );
	TestAssert(  test2x3x2GraphMoore.ContainsEdge( 3,  4) );
	TestAssert(  test2x3x2GraphMoore.ContainsEdge( 4,  5) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 5,  6) );
	TestAssert(  test2x3x2GraphMoore.ContainsEdge( 6,  7) );
	TestAssert(  test2x3x2GraphMoore.ContainsEdge( 7,  8) );
	TestAssert(  test2x3x2GraphMoore.ContainsEdge( 8,  9) );
	TestAssert(  test2x3x2GraphMoore.ContainsEdge( 9, 10) );
	TestAssert(  test2x3x2GraphMoore.ContainsEdge(10, 11) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge(11, 12) );

	TestAssert(  test2x3x2GraphMoore.ContainsEdge( 0,  2) );
	TestAssert(  test2x3x2GraphMoore.ContainsEdge( 1,  3) );
	TestAssert(  test2x3x2GraphMoore.ContainsEdge( 2,  4) );
	TestAssert(  test2x3x2GraphMoore.ContainsEdge( 3,  5) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 4,  6) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 5,  7) );
	TestAssert(  test2x3x2GraphMoore.ContainsEdge( 6,  8) );
	TestAssert(  test2x3x2GraphMoore.ContainsEdge( 7,  9) );
	TestAssert(  test2x3x2GraphMoore.ContainsEdge( 8, 10) );
	TestAssert(  test2x3x2GraphMoore.ContainsEdge( 9, 11) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge(10, 12) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge(11, 13) );

	TestAssert(  test2x3x2GraphMoore.ContainsEdge( 0,  3) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 1,  4) );
	TestAssert(  test2x3x2GraphMoore.ContainsEdge( 2,  5) );
	TestAssert(  test2x3x2GraphMoore.ContainsEdge( 3,  6) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 4,  7) );
	TestAssert(  test2x3x2GraphMoore.ContainsEdge( 5,  8) );
	TestAssert(  test2x3x2GraphMoore.ContainsEdge( 6,  9) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 7, 10) );
	TestAssert(  test2x3x2GraphMoore.ContainsEdge( 8, 11) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 9, 12) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge(10, 13) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge(11, 14) );

	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 0,  4) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 1,  5) );
	TestAssert(  test2x3x2GraphMoore.ContainsEdge( 2,  6) );
	TestAssert(  test2x3x2GraphMoore.ContainsEdge( 3,  7) );
	TestAssert(  test2x3x2GraphMoore.ContainsEdge( 4,  8) );
	TestAssert(  test2x3x2GraphMoore.ContainsEdge( 5,  9) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 6, 10) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 7, 11) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 8, 12) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 9, 13) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge(10, 14) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge(11, 15) );

	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 0,  5) );
	TestAssert(  test2x3x2GraphMoore.ContainsEdge( 1,  6) );
	TestAssert(  test2x3x2GraphMoore.ContainsEdge( 2,  7) );
	TestAssert(  test2x3x2GraphMoore.ContainsEdge( 3,  8) );
	TestAssert(  test2x3x2GraphMoore.ContainsEdge( 4,  9) );
	TestAssert(  test2x3x2GraphMoore.ContainsEdge( 5, 10) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 6, 11) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 7, 12) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 8, 13) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 9, 14) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge(10, 15) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge(11, 16) );

	TestAssert(  test2x3x2GraphMoore.ContainsEdge( 0,  6) );
	TestAssert(  test2x3x2GraphMoore.ContainsEdge( 1,  7) );
	TestAssert(  test2x3x2GraphMoore.ContainsEdge( 2,  8) );
	TestAssert(  test2x3x2GraphMoore.ContainsEdge( 3,  9) );
	TestAssert(  test2x3x2GraphMoore.ContainsEdge( 4, 10) );
	TestAssert(  test2x3x2GraphMoore.ContainsEdge( 5, 11) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 6, 12) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 7, 13) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 8, 14) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 9, 15) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge(10, 16) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge(11, 17) );

	TestAssert(  test2x3x2GraphMoore.ContainsEdge( 0,  7) );
	TestAssert(  test2x3x2GraphMoore.ContainsEdge( 1,  8) );
	TestAssert(  test2x3x2GraphMoore.ContainsEdge( 2,  9) );
	TestAssert(  test2x3x2GraphMoore.ContainsEdge( 3, 10) );
	TestAssert(  test2x3x2GraphMoore.ContainsEdge( 4, 11) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 5, 12) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 6, 13) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 7, 14) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 8, 15) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 9, 16) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge(10, 17) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge(11, 18) );

	TestAssert(  test2x3x2GraphMoore.ContainsEdge( 0,  8) );
	TestAssert(  test2x3x2GraphMoore.ContainsEdge( 1,  9) );
	TestAssert(  test2x3x2GraphMoore.ContainsEdge( 2, 10) );
	TestAssert(  test2x3x2GraphMoore.ContainsEdge( 3, 11) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 4, 12) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 5, 13) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 6, 14) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 7, 15) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 8, 16) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 9, 17) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge(10, 18) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge(11, 19) );

	TestAssert(  test2x3x2GraphMoore.ContainsEdge( 0,  9) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 1, 10) );
	TestAssert(  test2x3x2GraphMoore.ContainsEdge( 2, 11) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 3, 12) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 4, 13) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 5, 14) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 6, 15) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 7, 16) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 8, 17) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 9, 18) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge(10, 19) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge(11, 20) );

	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 0, 10) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 1, 11) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 2, 12) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 3, 13) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 4, 14) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 5, 15) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 6, 16) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 7, 17) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 8, 18) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 9, 19) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge(10, 20) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge(11, 21) );

	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 0, 11) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 1, 12) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 2, 13) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 3, 14) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 4, 15) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 5, 16) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 6, 17) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 7, 18) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 8, 19) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge( 9, 20) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge(10, 21) );
	TestAssert( !test2x3x2GraphMoore.ContainsEdge(11, 22) );


	// Test index orderings:
	for(int i=0; i<2; ++i)
	{
		iAImageCoordinate::IndexOrdering indexOrdering = static_cast<iAImageCoordinate::IndexOrdering>(i);
		iAImageGraph test2x3x2GraphMooreColRow(2, 3, 2, indexOrdering, iAImageGraph::nbhMoore);

		TestEqual(static_cast<iAEdgeIndexType>(50), test2x3x2GraphMooreColRow.GetEdgeCount());
		// left/lower/behind:
		TestAssert(test2x3x2GraphMooreColRow.ContainsEdge(iAImageCoordinate(0, 0, 0), iAImageCoordinate(0, 0, 1)));
		TestAssert(test2x3x2GraphMooreColRow.ContainsEdge(iAImageCoordinate(0, 0, 0), iAImageCoordinate(0, 1, 0)));
		TestAssert(test2x3x2GraphMooreColRow.ContainsEdge(iAImageCoordinate(0, 0, 0), iAImageCoordinate(1, 0, 0)));

		TestAssert(test2x3x2GraphMooreColRow.ContainsEdge(iAImageCoordinate(1, 0, 0), iAImageCoordinate(1, 1, 0)));
		TestAssert(test2x3x2GraphMooreColRow.ContainsEdge(iAImageCoordinate(1, 0, 0), iAImageCoordinate(1, 0, 1)));

		TestAssert(test2x3x2GraphMooreColRow.ContainsEdge(iAImageCoordinate(0, 1, 0), iAImageCoordinate(0, 1, 1)));
		TestAssert(test2x3x2GraphMooreColRow.ContainsEdge(iAImageCoordinate(0, 1, 0), iAImageCoordinate(0, 2, 0)));
		TestAssert(test2x3x2GraphMooreColRow.ContainsEdge(iAImageCoordinate(0, 1, 0), iAImageCoordinate(1, 1, 0)));

		TestAssert(test2x3x2GraphMooreColRow.ContainsEdge(iAImageCoordinate(1, 1, 0), iAImageCoordinate(1, 2, 0)));
		TestAssert(test2x3x2GraphMooreColRow.ContainsEdge(iAImageCoordinate(1, 1, 0), iAImageCoordinate(1, 1, 1)));

		TestAssert(test2x3x2GraphMooreColRow.ContainsEdge(iAImageCoordinate(0, 2, 0), iAImageCoordinate(0, 2, 1)));
		TestAssert(test2x3x2GraphMooreColRow.ContainsEdge(iAImageCoordinate(0, 2, 0), iAImageCoordinate(1, 2, 0)));

		TestAssert(test2x3x2GraphMooreColRow.ContainsEdge(iAImageCoordinate(1, 2, 0), iAImageCoordinate(1, 2, 1)));

		TestAssert(test2x3x2GraphMooreColRow.ContainsEdge(iAImageCoordinate(0, 0, 1), iAImageCoordinate(0, 1, 1)));
		TestAssert(test2x3x2GraphMooreColRow.ContainsEdge(iAImageCoordinate(0, 0, 1), iAImageCoordinate(1, 0, 1)));

		TestAssert(test2x3x2GraphMooreColRow.ContainsEdge(iAImageCoordinate(1, 0, 1), iAImageCoordinate(1, 1, 1)));

		TestAssert(test2x3x2GraphMooreColRow.ContainsEdge(iAImageCoordinate(0, 1, 1), iAImageCoordinate(0, 2, 1)));
		TestAssert(test2x3x2GraphMooreColRow.ContainsEdge(iAImageCoordinate(0, 1, 1), iAImageCoordinate(1, 1, 1)));

		TestAssert(test2x3x2GraphMooreColRow.ContainsEdge(iAImageCoordinate(1, 1, 1), iAImageCoordinate(1, 2, 1)));

		TestAssert(test2x3x2GraphMooreColRow.ContainsEdge(iAImageCoordinate(0, 2, 1), iAImageCoordinate(1, 2, 1)));

		// "plane" diagonals:
		TestAssert(test2x3x2GraphMooreColRow.ContainsEdge(iAImageCoordinate(0, 0, 0), iAImageCoordinate(0, 1, 1)));
		TestAssert(test2x3x2GraphMooreColRow.ContainsEdge(iAImageCoordinate(0, 1, 0), iAImageCoordinate(0, 0, 1)));

		TestAssert(test2x3x2GraphMooreColRow.ContainsEdge(iAImageCoordinate(0, 0, 0), iAImageCoordinate(1, 0, 1)));
		TestAssert(test2x3x2GraphMooreColRow.ContainsEdge(iAImageCoordinate(1, 0, 0), iAImageCoordinate(0, 0, 1)));

		TestAssert(test2x3x2GraphMooreColRow.ContainsEdge(iAImageCoordinate(0, 0, 0), iAImageCoordinate(1, 1, 0)));
		TestAssert(test2x3x2GraphMooreColRow.ContainsEdge(iAImageCoordinate(1, 0, 0), iAImageCoordinate(0, 1, 0)));


		TestAssert(test2x3x2GraphMooreColRow.ContainsEdge(iAImageCoordinate(1, 0, 0), iAImageCoordinate(1, 1, 1)));
		TestAssert(test2x3x2GraphMooreColRow.ContainsEdge(iAImageCoordinate(1, 1, 0), iAImageCoordinate(1, 0, 1)));


		TestAssert(test2x3x2GraphMooreColRow.ContainsEdge(iAImageCoordinate(0, 1, 0), iAImageCoordinate(0, 2, 1)));
		TestAssert(test2x3x2GraphMooreColRow.ContainsEdge(iAImageCoordinate(0, 2, 0), iAImageCoordinate(0, 1, 1)));

		TestAssert(test2x3x2GraphMooreColRow.ContainsEdge(iAImageCoordinate(0, 1, 0), iAImageCoordinate(1, 1, 1)));
		TestAssert(test2x3x2GraphMooreColRow.ContainsEdge(iAImageCoordinate(1, 1, 0), iAImageCoordinate(0, 1, 1)));

		TestAssert(test2x3x2GraphMooreColRow.ContainsEdge(iAImageCoordinate(0, 1, 0), iAImageCoordinate(1, 2, 0)));
		TestAssert(test2x3x2GraphMooreColRow.ContainsEdge(iAImageCoordinate(0, 2, 0), iAImageCoordinate(1, 1, 0)));


		TestAssert(test2x3x2GraphMooreColRow.ContainsEdge(iAImageCoordinate(1, 1, 0), iAImageCoordinate(1, 2, 1)));
		TestAssert(test2x3x2GraphMooreColRow.ContainsEdge(iAImageCoordinate(1, 2, 0), iAImageCoordinate(1, 1, 1)));


		TestAssert(test2x3x2GraphMooreColRow.ContainsEdge(iAImageCoordinate(0, 2, 0), iAImageCoordinate(1, 2, 1)));
		TestAssert(test2x3x2GraphMooreColRow.ContainsEdge(iAImageCoordinate(1, 2, 0), iAImageCoordinate(0, 2, 1)));


		TestAssert(test2x3x2GraphMooreColRow.ContainsEdge(iAImageCoordinate(0, 0, 1), iAImageCoordinate(1, 1, 1)));
		TestAssert(test2x3x2GraphMooreColRow.ContainsEdge(iAImageCoordinate(1, 0, 1), iAImageCoordinate(0, 1, 1)));


		TestAssert(test2x3x2GraphMooreColRow.ContainsEdge(iAImageCoordinate(0, 1, 1), iAImageCoordinate(1, 2, 1)));
		TestAssert(test2x3x2GraphMooreColRow.ContainsEdge(iAImageCoordinate(1, 1, 1), iAImageCoordinate(0, 2, 1)));

		// "cross" diagonals:
		TestAssert(test2x3x2GraphMooreColRow.ContainsEdge(iAImageCoordinate(0, 0, 0), iAImageCoordinate(1, 1, 1)));
		TestAssert(test2x3x2GraphMooreColRow.ContainsEdge(iAImageCoordinate(0, 0, 1), iAImageCoordinate(1, 1, 0)));
		TestAssert(test2x3x2GraphMooreColRow.ContainsEdge(iAImageCoordinate(0, 1, 0), iAImageCoordinate(1, 0, 1)));
		TestAssert(test2x3x2GraphMooreColRow.ContainsEdge(iAImageCoordinate(1, 0, 0), iAImageCoordinate(0, 1, 1)));

		TestAssert(test2x3x2GraphMooreColRow.ContainsEdge(iAImageCoordinate(0, 1, 0), iAImageCoordinate(1, 2, 1)));
		TestAssert(test2x3x2GraphMooreColRow.ContainsEdge(iAImageCoordinate(0, 1, 1), iAImageCoordinate(1, 2, 0)));
		TestAssert(test2x3x2GraphMooreColRow.ContainsEdge(iAImageCoordinate(0, 2, 0), iAImageCoordinate(1, 1, 1)));
		TestAssert(test2x3x2GraphMooreColRow.ContainsEdge(iAImageCoordinate(1, 1, 0), iAImageCoordinate(0, 2, 1)));
	}



END_TEST