/*
 * OkcDataModifierHighlight.h
 *
 *  Created on: Feb 9, 2009
 *      Author: Zaixian Xie
 */

/*
 * class OkcDataModifierHighlight is a modifier for OkcData
 * to highlight a subset of datasets.  The main job of brush operator
 * it to add an instance of OkcDataModifierHighlight to OkcData
 */
#ifndef OKCDATAMODIFIERHIGHLIGHT_H_
#define OKCDATAMODIFIERHIGHLIGHT_H_

#include "../OkcDataModifier.h"
#include <vector>

class OkcDataModifierHighlight: public OkcDataModifier {
public:
	OkcDataModifierHighlight();
	OkcDataModifierHighlight(OkcData* okcdata);
	virtual ~OkcDataModifierHighlight();

private:
	// The brush operator stores all brush flag in the original order of the
	// datapoints.  This means that it will ignore all sorting order.
	// See function BrushOperator::doOperator();
	std::vector<bool> m_highlighted;

public:
	// return the modifier type
	XmdvTool::MODIFIERTYPE getModifierType();

	// Initialize the highlighted array,
	// its length is data_size of the OkcData
	void initHighlightArr();
	void setHighlight(int i, bool isHighlighted);
	bool getHighlight(int i);

	void copyFrom(const OkcDataModifierHighlight* copy);
	// overloading the operator "="
	void operator=(const OkcDataModifierHighlight &copy);


};

#endif /* OKCDATAMODIFIERHIGHLIGHT_H_ */
