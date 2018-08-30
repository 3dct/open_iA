/*
 * OkcDataModifierDimOOO.h
 *
 *  Created on: Feb 10, 2009
 *      Author: Zaixian Xie
 */

/*
 * class OkcDataModifierDimOOO is a modifier for OkcData
 * to do dimension On/Off/reOrdering  The main job of DimOOO operator
 * it to add an instance of OkcDataModifierDimOOO to OkcData
 */

#ifndef OKCDATAMODIFIERDIMOOO_H_
#define OKCDATAMODIFIERDIMOOO_H_

#include <vector>
#include "OkcData.h"
#include "OkcDataModifier.h"
#include "DimOOOMap.h"

class OkcDataModifierDimOOO: public OkcDataModifier {
	// make the class DimOOOOperator as the friend class of this class.
	// So the operator can access two private members .
	friend class DimOOOOperator;
public:
	OkcDataModifierDimOOO();
	OkcDataModifierDimOOO(OkcData* okcdata);
	virtual ~OkcDataModifierDimOOO();

private:
	DimOOOMap* m_dimOOOMap;

public:
	// return the modifier type
	XmdvTool::MODIFIERTYPE getModifierType();

	// return the pointer to the instance of class DimOOOMap
	DimOOOMap* getDimOOOMap();

	// return the dimension size after dimension on/off/reordering
	int getDimSize();

	void copyFrom(const OkcDataModifierDimOOO* copy);
	//overloading the operator "="
	void operator=(const OkcDataModifierDimOOO &copy);

	template<class T>
	void mapData(std::vector<T> const &all_data, std::vector<T> &data) {
		static std::vector<T>	temp;
		int all_dims = getOkcData()->getOrigDimSize();
		temp.resize(all_dims);

		std::vector<int> posInView;
		int viewDimSize = m_dimOOOMap->getViewDimSize();
		data.resize(viewDimSize);
		m_dimOOOMap->getPosInView(posInView);

		int	i;
		for (i = 0; i < all_dims; i++) {

			int pos = posInView[i];
			if (pos>=0){
				data[pos] = all_data[i];
			}
		}

	}

	template<class T>
	void invMapData(std::vector<T> &data, std::vector<T> &origData) {
		std::vector<int> posInView;
		int all_dims = getOkcData()->getOrigDimSize();
		origData.resize(all_dims);
		m_dimOOOMap->getPosInView(posInView);
		int i;
		for (i=0; i<(int)origData.size(); i++) {
			if (posInView[i]>=0) {
				// replace the dimension with the value in the view
				origData[i] = data[posInView[i]];
			}
		}
	}
};

#endif /* OKCDATAMODIFIERDIMOOO_H_ */
