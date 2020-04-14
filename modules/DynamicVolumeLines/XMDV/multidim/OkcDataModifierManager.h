/*
 * OkcDataModifierManager.h
 *
 *  Created on: Feb 6, 2009
 *      Author: Zaixian xie
 */

/*
 * class OkcDataModifierManager manages all of modifiers of
 * an okcdata.  It stores all of modifiers in a map.  Each okcdata
 * has a pointer to an OkcDataModifierManager.
 */
#ifndef OKCDATAMODIFIERMANAGER_H_
#define OKCDATAMODIFIERMANAGER_H_

#include "../main/XmdvTool.h"
#include <vector>
#include "../OkcData.h"
#include "../OkcDataModifier.h"
#include "../OkcDataModifierDimOOO.h"
#include <map>
#include <cassert>
using namespace std;

class OkcDataModifier;

class OkcDataModifierManager {

	// Make the class OkcData as the friend class of this class
	// so that OkcData can access m_modifiers
	friend class OkcData;
public:
	OkcDataModifierManager();
	OkcDataModifierManager(OkcData* okcdata);
	virtual ~OkcDataModifierManager();

private:
	// the dataset that this modifier applies to
	OkcData* m_okcdata;
	// All pointers to modifiers are stored in this map
	map<XmdvTool::MODIFIERTYPE, OkcDataModifier*> m_modifiers;

	// The flag to indicate whether the specified modifier is a reference
	// to a modifier in some other okcdata.  When we copy the data from an OkcData
	// to another, we also copy all modifiers.  In order to save time cost, we only
	// copy the reference (pointer).  Thus, we use this flag to represent whether this
	// modifier is a reference.  We only release the memory occupied by those modifiers
	// are not references in the destructor of OkcDataModifierManager.
	// m_modifierRefFlag[] = false:  The specified modifier in this OkcData is not reference.
	//    We need to release the memory occupied by modifier in the destructor of OkcDataModifierManager.
	// m_modifierRefFlag[] = true:  The specified modifier in this OkcData is a reference.
	//    We cannot release the memory occupied by modifier in the destructor of OkcDataModifierManager.
	map<XmdvTool::MODIFIERTYPE, bool> m_modifierRefFlag;
public:
	void setOkcData(OkcData* okcdata);
	OkcData* getOkcData();

	/*
	 * The functions to get okcdata attributes or data
	 * based on current modifiers.  For example,
	 * the original okcdata has a dims to denote the number
	 * of dimensions, which is changed due to the
	 * DimOOOModifier (dimension on/off/ordering)
	 */
public:
	// return the modifier specified by the type.
	// if the particular modifier does not exist, return 0
	OkcDataModifier* getOkcDataModifier(XmdvTool::MODIFIERTYPE modifierType);
	// add a new OkcData modifier into the map
	void addOkcDataModifier(XmdvTool::MODIFIERTYPE modifierType,
			OkcDataModifier* modifier);
	int getDataSize(int data_size);
	int getDimSize(int dims);

	// if the order for rows has been changed,
	// or sampling has been applied to the dataset,
	// this function can find the actual line no in the original data
	int getOrigLine(int line);

	// overloading the operator "="
	void copyFrom(const OkcDataModifierManager* copy);
	void operator=(const OkcDataModifierManager& copy);

	template<class T>
	void mapData(std::vector<T> const &all_data, std::vector<T> &data)
	{
		static std::vector<T>	temp;

		OkcDataModifier* modifier = getOkcDataModifier(XmdvTool::MODIFIER_DIMOOO);
		if (modifier)
		{
			// If the modifier exists, call its map function
			OkcDataModifierDimOOO* modifierDimOOO = (OkcDataModifierDimOOO*)modifier;
			modifierDimOOO->mapData(all_data, data);
		}
		else
		{
			// Otherwise, make the output equal to the input
			size_t n=all_data.size();
			data.resize(n);
			for (size_t i=0; i<n; i++)
			{
				data[i] = all_data[i];
			}
		}
	}

	template<class T>
	void invMapData(std::vector<T> &data, std::vector<T> &origData)
	{
		OkcDataModifier* modifier = getOkcDataModifier(XmdvTool::MODIFIER_DIMOOO);
		if (modifier)
		{
			// If the modifier exists, call its invMap function
			OkcDataModifierDimOOO* modifierDimOOO = (OkcDataModifierDimOOO*)modifier;
			modifierDimOOO->invMapData(data, origData);
		}
		else
		{
			// Otherwise, make the output equal to the input
			assert(data.size()==origData.size());
			size_t n = data.size();
			for (size_t i=0; i<n; i++)
			{
				origData[i] = data[i];
			}
		}
	}

};

#endif /* OKCDATAMODIFIERMANAGER_H_ */
