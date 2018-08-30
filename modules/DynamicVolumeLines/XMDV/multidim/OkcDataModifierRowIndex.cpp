/*
 * OkcDataModifierRowIndex.cpp
 *
 *  Created on: May 12, 2009
 *      Author: Zaixian Xie
 */

#include "OkcDataModifierRowIndex.h"
#include "../OkcData.h"

OkcDataModifierRowIndex::OkcDataModifierRowIndex() {
}

OkcDataModifierRowIndex::OkcDataModifierRowIndex(OkcData* okcdata) :
	OkcDataModifier(okcdata) {

}

OkcDataModifierRowIndex::~OkcDataModifierRowIndex() {
}

XmdvTool::MODIFIERTYPE OkcDataModifierRowIndex::getModifierType() {
	return XmdvTool::MODIFIER_ROWINDEX;
}

void OkcDataModifierRowIndex::initRowIndexArr() {
	OkcData* okcdata = getOkcData();
	int dataSize = okcdata->getDataSize();
	m_rowIndex.resize(dataSize);
	int i;
	for (i=0; i<dataSize; i++) {
		m_rowIndex[i] = i;
	}

}

void OkcDataModifierRowIndex::setRowIndex(int i, int index) {
	m_rowIndex[i] = index;
}

int OkcDataModifierRowIndex::getRowIndex(int i) {
	return m_rowIndex[i];
}


void OkcDataModifierRowIndex::copyFrom(const OkcDataModifierRowIndex* copy) {
	OkcDataModifier::copyFrom((OkcDataModifier*)copy);
	m_rowIndex = copy->m_rowIndex;

}

void OkcDataModifierRowIndex::operator=(const OkcDataModifierRowIndex &copy) {
	copyFrom(&copy);
}
