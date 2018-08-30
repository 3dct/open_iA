/*
 * ReadOkcException.cpp
 *
 *  Created on: Jan 8, 2009
 *      Author: Zaixian Xie
 *
 */
#include "ReadOkcException.h"

ReadOkcException::ReadOkcException() throw() {
}

ReadOkcException::ReadOkcException(string strError)  throw() : exception(){
    m_strError = strError;
}

ReadOkcException::~ReadOkcException() throw() {
}

const char* ReadOkcException::what() const throw() {
	return m_strError.c_str();
}
