/*
 * OkcDataModifier.h
 *
 *  Created on: Feb 6, 2009
 *      Author: Zaixian Xie
 */

/*
 * Class OkcDataModifier defines some views for an OkcData.
 * It does not change the OkcData.  We call these views
 * okcdata modifiers.  This class only defines
 * the basic behavior of these modifiers.  Its derived class
 * will define these views.  For example, class OkcDataHighlightModifier
 * defines the highlighted subset in an OkcData.
 * An OkcData can have multiple modifiers.
 * The main function of most of queries is to add a modifier to the OkcData.
 *
 */
#ifndef OKCDATAMODIFIER_H_
#define OKCDATAMODIFIER_H_

#include "main/XmdvTool.h"

class OkcData;

class OkcDataModifier {
public:
	OkcDataModifier();
	OkcDataModifier(OkcData* okcdata);
	virtual ~OkcDataModifier();

private:
	// The OkcData that this modifier applies to
	OkcData* m_okcdata;

protected:
	// The number of dimension and records for the original OkcData
	int m_dims;
	int m_data_size;

public:
	virtual XmdvTool::MODIFIERTYPE getModifierType();

	// set the okcdata
	void setOkcData(OkcData* okcdata);
	OkcData* getOkcData();

	void copyFrom(const OkcDataModifier* copy);
	// overloading the operator "="
	void operator=(const OkcDataModifier &copy);
};

#endif /* OKCDATAMODIFIER_H_ */
