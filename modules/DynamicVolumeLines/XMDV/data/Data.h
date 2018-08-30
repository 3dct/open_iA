/*
 * Data.h
 *
 *  Created on: Jan 6, 2009
 *      Author: Zaixian Xie
 */

#ifndef DATA_H_
#define DATA_H_

/*
 * This class is the base of all classes representing data,
 * such as OkcData, ClusterTree, CInterRingDimClusterTree
 */

#include <QString>
#include <QStringList>

class Data {
public:
	Data();
	virtual ~Data();

private:
	// The title of this data, it will be used as the title of the view.
	// It will be set when reading the data file
	QString m_title;

public:
	void setTitle (QString title);
	QString getTitle();

	// Return a string list to describe this data.
	// It should be rewritten in the subclass
	virtual QStringList toStringList();
};

#endif /* DATA_H_ */
