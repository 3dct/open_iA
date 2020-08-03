/*
 * OkcDataModifierManager.cpp
 *
 *  Created on: Feb 6, 2009
 *      Author: Zaixian Xie
 */

#include "OkcDataModifierManager.h"
#include "../OkcDataModifier.h"
#include "../OkcDataModifierDimOOO.h"
#include "OkcDataModifierHighlight.h"
#include "OkcDataModifierRowIndex.h"
//#include "data/cluster/OkcDataModifierClusterNodeMax.h"
//#include "data/cluster/OkcDataModifierClusterNodeMin.h"
//#include "data/cluster/OkcDataModifierClusterColor.h"
//#include "data/cluster/OkcDataModifierClusterHighlight.h"
//#include "data/multidim/OkcDataModifierPCADerive.h"
//#include "data/multidim/diagonal/HistogramModifier.h"
//#include "data/multidim/diagonal/RowIndexOnAllDimModifier.h"
//#include "data/cluster/OkcDataModifierClusterEntries.h"
#include "../main/XmdvTool.h"
#include <map>

OkcDataModifierManager::OkcDataModifierManager()
{
	m_okcdata = 0;
}

OkcDataModifierManager::OkcDataModifierManager(OkcData* okcdata)
{
	m_okcdata = okcdata;
}

OkcDataModifierManager::~OkcDataModifierManager()
{
	std::map<XmdvTool::MODIFIERTYPE, OkcDataModifier*>::iterator it;
	// release the memory occupied by all of modifiers
	for ( it=m_modifiers.begin(); it != m_modifiers.end(); it++ )
	{
		// We only release the memory occupied by those non-reference modifier.
		if ( !m_modifierRefFlag[ (*it).first ] ) {
			delete ((*it).second);
		}
	}

}

OkcDataModifier* OkcDataModifierManager::getOkcDataModifier(XmdvTool::MODIFIERTYPE modifierType) {
	if (m_modifiers.find(modifierType)==m_modifiers.end()) {
		// the specified modifier type does not exist
		return 0;
	} else {
		return m_modifiers[modifierType];
	}
}

void OkcDataModifierManager::addOkcDataModifier(XmdvTool::MODIFIERTYPE modifierType,
		OkcDataModifier* modifier) {
	// This action is allowed only if the specified modifier does not exist
	assert(m_modifiers.find(modifierType)==m_modifiers.end());
	m_modifiers[modifierType] = modifier;
	// By default, the new modifier is not a reference
	m_modifierRefFlag[modifierType] = false;
}


void OkcDataModifierManager::setOkcData(OkcData* okcdata){
	m_okcdata = okcdata;
}

OkcData* OkcDataModifierManager::getOkcData() {
	return m_okcdata;
}

int OkcDataModifierManager::getDataSize(int data_size) {
	return data_size;
}

int OkcDataModifierManager::getDimSize(int dims) {
	OkcDataModifier* modifier = getOkcDataModifier(XmdvTool::MODIFIER_DIMOOO);
	if (modifier) {
		// If the modifier exists, call its map function
		OkcDataModifierDimOOO* modifierDimOOO = (OkcDataModifierDimOOO*)modifier;
		return modifierDimOOO->getDimSize();
	} else {
		return dims;
	}
}

int OkcDataModifierManager::getOrigLine(int line) {
	OkcDataModifier* modifier = getOkcDataModifier(XmdvTool::MODIFIER_ROWINDEX);
	if (modifier) {
		// If the modifier exists, call its map function
		OkcDataModifierRowIndex* modifierRowIndex = (OkcDataModifierRowIndex*)modifier;
		return modifierRowIndex->getRowIndex(line);
	} else {
		return line;
	}
}

void OkcDataModifierManager::copyFrom(const OkcDataModifierManager* copy) {
	m_okcdata = copy->m_okcdata;
	m_modifiers.clear();
	std::map<XmdvTool::MODIFIERTYPE, OkcDataModifier*>::iterator it;
	std::map<XmdvTool::MODIFIERTYPE, OkcDataModifier*> copymap;
	copymap = copy->m_modifiers;
	// copy each modifier
	for ( it=copymap.begin(); it!=copymap.end(); it++) {
		m_modifiers[ (*it).first ] = copymap[ (*it).first ];
		m_modifierRefFlag[ (*it).first ] = true;
	}
}

void OkcDataModifierManager::operator=(const OkcDataModifierManager& copy) {
	copyFrom(&copy);
}
