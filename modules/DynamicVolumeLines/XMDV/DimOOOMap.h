/*
 * DimOOOMap.h
 *
 *  Created on: Feb 14, 2009
 *      Author: Zaixian Xie
 */

/*
 * Class DimOOOMap describes how the dimensions
 * in an OkcData are to be turned on/off or reordering
 *
 */

#ifndef DIMOOOMAP_H_
#define DIMOOOMAP_H_

#include <vector>

class DimOOOMap {
public:
	enum DIRECTION {
		// Exchange the dimension with one before it
		BACKWARD = 0,
		// Exchange the dimension with one after it
		FORWARD = 1,
	};

public:
	DimOOOMap();
	DimOOOMap(int dimSize);
	virtual ~DimOOOMap();

private:
	// the number of dimensions to be mapped
	int m_dimSize;
	//The size of the the following two arrays is m_dimSize
	//map_in_to_out[k] records the current order of the kth dimension input data
	std::vector<int>		map_in_to_out;
	//on[k] records the current "on/off" status of the kth dimension
	std::vector<bool>		on;

public:
	void setDimSize(int dimSize);
	// return the dimension size of input
	int getDimSize();

	// initialize the map to turn on all dimensions
	// and keep the original dimension order
	void initMap();

	void setMapInToOut(std::vector<int>& map_in_to_out);
	void getMapInToOut(std::vector<int>& map_in_to_out);

	void setOn(std::vector<bool>& on);
	void getOn(std::vector<bool>& On);

	// map_out_to_in[i] denotes the position in the input data
	// for the dimension i of view ignoring whether dimension is
	// on or off
	void getMapOutToIn(std::vector<int>& map_out_to_in);

	// posInView denote the positions in the view
	// for each dim of input
	void getPosInView(std::vector<int>& posInView);

	// Get the dimension size of output
	int getViewDimSize();

	// overloading the operator "="
	void operator=(const DimOOOMap &copy);

	// Turn on or off a dimension
	void setDimOnOff(int dim, bool state);

	// Reorder one dimension
	// Param:
	//   dimID: the id of the dimension to be reordered
	//   direction: to indicate whether to move forward or backward this dimension
	void reorderDimension(int dimID, DimOOOMap::DIRECTION direction);
};

#endif /* DIMOOOMAP_H_ */
