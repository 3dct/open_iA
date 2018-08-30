/*
 * OkcDataModifierRowIndex.h
 *
 *  Created on: May 12, 2009
 *      Author: Zaixian Xie
 */
/*
 * class OkcDataModifierRowIndex is a modifier for OkcData
 * to change the order of rows and/or do sampling.
 * The data member m_rowIndex whose size is the data size
 * of the input OkcData. m_rowIndex[i] is the original order in the
 * input OkcData for the datapoint that is in the ith position
 * after sorting or sampling.  For example, if we sort the following three datapoints
 * data[0] = 3 ...
 * data[1] = 6 ...
 * data[2] = 4 ...
 * based on the first dimension in increasing order. So:
 * m_rowIndex[0] = 0
 * m_rowIndex[1] = 2
 * m_rowIndex[2] = 1
 */

#ifndef OKCDATAMODIFIERROWINDEX_H_
#define OKCDATAMODIFIERROWINDEX_H_

#include "../OkcDataModifier.h"
#include "../main/XmdvTool.h"
#include <vector>

class OkcDataModifierRowIndex : public OkcDataModifier {
	friend class SortRowsOperator;
	friend class OkcDataModifierManager;
public:
	OkcDataModifierRowIndex();
	OkcDataModifierRowIndex(OkcData* okcdata);
	virtual ~OkcDataModifierRowIndex();

private:
	std::vector<int> m_rowIndex;

public:
	// return the modifier type
	XmdvTool::MODIFIERTYPE getModifierType();

	// Initialize the highlighted array,
	// its length is data_size of the OkcData
	void initRowIndexArr();
	void setRowIndex(int i, int index);
	int getRowIndex(int i);

	void copyFrom(const OkcDataModifierRowIndex* copy);
	// overloading the operator "="
	void operator=(const OkcDataModifierRowIndex &copy);


};

#endif /* OKCDATAMODIFIERROWINDEX_H_ */
