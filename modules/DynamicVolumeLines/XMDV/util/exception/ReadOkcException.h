/*
 * ReadOkcException.h
 *
 *  Created on: Jan 8, 2009
 *      Author: Zaixian Xie
 */

#ifndef READOKCEXCEPTION_H_
#define READOKCEXCEPTION_H_

#include <exception>
#include <string>

//! This class represents an exception during reading Okc file
class ReadOkcException : public std::exception {
public:
	virtual ~ReadOkcException() throw();

private:
	ReadOkcException() throw(); // not meant to be called
	std::string m_strError;

public:
	ReadOkcException(std::string strError) throw();
	const char* what() const throw();
};

#endif /* READOKCEXCEPTION_H_ */
