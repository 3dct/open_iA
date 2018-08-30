/*
 * OkcDataModifierHighlight.cpp
 *
 *  Created on: Feb 9, 2009
 *      Author: Zaixian Xie
 */

#include "OkcDataModifierHighlight.h"
#include "../OkcDataModifier.h"
#include "../main/XmdvTool.h"
#include "../OkcData.h"


OkcDataModifierHighlight::OkcDataModifierHighlight() {

}

OkcDataModifierHighlight::OkcDataModifierHighlight(OkcData* okcdata) :
	OkcDataModifier(okcdata) {

}

OkcDataModifierHighlight::~OkcDataModifierHighlight() {

}

XmdvTool::MODIFIERTYPE OkcDataModifierHighlight::getModifierType() {
	return XmdvTool::MODIFIER_HIGHLIGHT;
}


void OkcDataModifierHighlight::initHighlightArr() {
	OkcData* okcdata = getOkcData();
	int dataSize = okcdata->getDataSize();
	m_highlighted.resize(dataSize);
	int i;
	for (i=0; i<dataSize; i++) {
		m_highlighted[i] = false;
	}
}

void OkcDataModifierHighlight::setHighlight(int i, bool isHighlighted=true) {
	m_highlighted[i] = isHighlighted;
}

bool OkcDataModifierHighlight::getHighlight(int i) {
	return m_highlighted[i];
}

void OkcDataModifierHighlight::copyFrom(const OkcDataModifierHighlight* copy) {
	OkcDataModifier::copyFrom((OkcDataModifier*)copy);
	m_highlighted = copy->m_highlighted;
}

void OkcDataModifierHighlight::operator=(const OkcDataModifierHighlight &copy) {
	copyFrom(&copy);
}
