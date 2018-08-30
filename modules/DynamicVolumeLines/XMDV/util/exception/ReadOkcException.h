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
using namespace std;

/*
 * This class represents the exception
 * during reading Okc file
 */

class ReadOkcException : public exception {
public:
	virtual ~ReadOkcException() throw();

private:
	ReadOkcException() throw(); // not meant to be called
	string m_strError;

public:
	ReadOkcException(string strError) throw();
	const char* what() const throw();
};

#endif /* READOKCEXCEPTION_H_ */
