/*
 * OkcDataModifier.cpp
 *
 *  Created on: Feb 6, 2009
 *      Author: Zaixian Xie
 */

#include "OkcDataModifier.h"
#include "main/XmdvTool.h"
#include "OkcData.h"

OkcDataModifier::OkcDataModifier() {

}

OkcDataModifier::OkcDataModifier(OkcData* okcdata) {
	setOkcData(okcdata);
}

OkcDataModifier::~OkcDataModifier() {
}

XmdvTool::MODIFIERTYPE OkcDataModifier::getModifierType() {
	return XmdvTool::MODIFIER_NULL;
}

void OkcDataModifier::setOkcData(OkcData* okcdata) {
	m_okcdata = okcdata;
	m_dims = okcdata->getOrigDimSize();
	m_data_size = okcdata->getOrigDataSize();
}

OkcData* OkcDataModifier::getOkcData() {
	return m_okcdata;
}

void OkcDataModifier::copyFrom(const OkcDataModifier* copy) {
	m_okcdata = copy->m_okcdata;
	m_dims = copy->m_dims;
	m_data_size = copy->m_data_size;
}

void OkcDataModifier::operator=(const OkcDataModifier &copy) {
	copyFrom(&copy);
}
