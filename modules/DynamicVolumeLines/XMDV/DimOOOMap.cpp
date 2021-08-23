/*
 * DimOOOMap.cpp
 *
 *  Created on: Feb 14, 2009
 *      Author: Zaixian Xie
 */

#include "DimOOOMap.h"
#include <vector>
#include <cassert>

DimOOOMap::DimOOOMap()
{
	map_in_to_out.clear();
	on.clear();
}

DimOOOMap::DimOOOMap(int dimSize)
{
	m_dimSize = dimSize;
	map_in_to_out.clear();
	on.clear();
}


DimOOOMap::~DimOOOMap()
{
}

void DimOOOMap::setDimSize(int dimSize)
{
	m_dimSize = dimSize;
}

int DimOOOMap::getDimSize()
{
	return m_dimSize;
}

void DimOOOMap::initMap()
{
	map_in_to_out.resize(m_dimSize);
	on.resize(m_dimSize);
	for (int i=0; i<m_dimSize; i++)
	{
		map_in_to_out[i] = i;
		on[i] = true;
	}
}

void DimOOOMap::setMapInToOut(std::vector<int>& newMap)
{
	assert( (int)newMap.size() == m_dimSize );
	this->map_in_to_out = newMap;
}

void DimOOOMap::getMapInToOut(std::vector<int>& outMap)
{
	outMap = this->map_in_to_out;
}

void DimOOOMap::setOn(std::vector<bool>& newOn)
{
	assert( (int)newOn.size() == m_dimSize );
	this->on = newOn;
}

void DimOOOMap::getOn(std::vector<bool>& outOn)
{
	outOn = this->on;
}

void DimOOOMap::getMapOutToIn(std::vector<int>& map_out_to_in)
{
	assert( (int)map_in_to_out.size() == m_dimSize );
	map_out_to_in.resize(m_dimSize);
	int i;
	// generate map_out_to_in
	for (i=0; i<m_dimSize; i++)
	{
		map_out_to_in[map_in_to_out[i]] = i;
	}
}

void DimOOOMap::getPosInView(std::vector<int>& posInView)
{
	assert( (int)map_in_to_out.size() == m_dimSize );
	assert( (int)on.size() == m_dimSize );

	std::vector<int> map_out_to_in;
	getMapOutToIn(map_out_to_in);
	//Now map_out_to_in is just posInView if all dimensions are on

	// generate posInView based in map_out_to_in
	// set the initial values to -1
	posInView.assign(m_dimSize, -1);
	int i, pos = -1;
	for (i=0; i<m_dimSize; i++) {
		if (on[map_out_to_in[i]]) {
			pos++;
			posInView[map_out_to_in[i]] = pos;
		}
	}
}

int DimOOOMap::getViewDimSize() {
	int	i, count=0;
	for (i = 0; i < m_dimSize; i++) {
		if (on[i]) {
			count++;
		}
	}
	return count;
}

void DimOOOMap::operator=(const DimOOOMap &copy) {
	this->m_dimSize = copy.m_dimSize;
	this->map_in_to_out = copy.map_in_to_out;
	this->on = copy.on;
}

void DimOOOMap::setDimOnOff(int rowID, bool state) {
	std::vector<int> map_out_to_in;
	getMapOutToIn(map_out_to_in);
	if (rowID >= 0) {
		on[map_out_to_in[rowID]] = state;
	}
}

void DimOOOMap::reorderDimension(int rowID, DimOOOMap::DIRECTION direction)
{
	//neighbor indicates that dimension need to be exchange with "which" dimension.
	int neighbor;

	if (direction == DimOOOMap::BACKWARD) {
		if (rowID == 0) {
			neighbor = m_dimSize - 1;
		} else {
			neighbor = rowID - 1;
		}
	} else if (direction == DimOOOMap::FORWARD) {
		if (rowID == m_dimSize - 1) {
			neighbor = 0;
		} else {
			neighbor = rowID + 1;
		}
	} else {
		return;
	}

	//bool temp_bool;

	std::vector<int> map_out_to_in;
	getMapOutToIn(map_out_to_in);
	// dimNewPos is the new position of dimension dimID
	int dimNewPos = map_out_to_in[neighbor];
	// neighborNewPos is the new position of neighbor
	int neighborNewPos = map_out_to_in[rowID];

	map_in_to_out[dimNewPos] = rowID;
	map_in_to_out[neighborNewPos] = neighbor;

}
