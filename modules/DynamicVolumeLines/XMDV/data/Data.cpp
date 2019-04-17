/*
 * Data.cpp
 *
 *  Created on: Jan 6, 2009
 *      Author: Zaixian Xie
 */

#include "Data.h"

Data::Data() {
	m_title.clear();
}

Data::~Data() {
}

void Data::setTitle (QString title) {
	m_title = title;
}

QString Data::getTitle() {
	return m_title;
}

QStringList Data::toStringList() {
	return QStringList();
}
