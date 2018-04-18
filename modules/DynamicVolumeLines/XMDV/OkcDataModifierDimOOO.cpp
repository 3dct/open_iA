/*
 * OkcDataModifierDimOOO.cpp
 *
 *  Created on: Feb 10, 2009
 *      Author: Zaixian Xie
 */

#include "OkcDataModifierDimOOO.h"
#include "OkcData.h"
#include "OkcDataModifier.h"
#include "DimOOOMap.h"
#include "main/XmdvTool.h"

OkcDataModifierDimOOO::OkcDataModifierDimOOO() {
	m_dimOOOMap = new DimOOOMap();
}

OkcDataModifierDimOOO::OkcDataModifierDimOOO(OkcData* okcdata):
	OkcDataModifier(okcdata) {
	m_dimOOOMap = new DimOOOMap(okcdata->getDimSize());
}

OkcDataModifierDimOOO::~OkcDataModifierDimOOO() {
	SAFE_DELETE(m_dimOOOMap);
}

DimOOOMap* OkcDataModifierDimOOO::getDimOOOMap() {
	return m_dimOOOMap;
}

XmdvTool::MODIFIERTYPE OkcDataModifierDimOOO::getModifierType() {
	return XmdvTool::MODIFIER_DIMOOO;
}

int OkcDataModifierDimOOO::getDimSize() {
	return m_dimOOOMap->getViewDimSize();
}

void OkcDataModifierDimOOO::copyFrom(const OkcDataModifierDimOOO* copy) {
	OkcDataModifier::copyFrom((OkcDataModifier*)copy);
	*(m_dimOOOMap) = *(copy->m_dimOOOMap);
}

void OkcDataModifierDimOOO::operator=(const OkcDataModifierDimOOO &copy) {
	copyFrom(&copy);
}
