// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAbase_export.h"

#include <QDomDocument>

class iATransferFunction;

//! Class for loading and storing settings to/from XML documents.
//! transfer functions
class iAbase_API iAXmlSettings
{
private:
	QDomDocument domDocument;
public:
	iAXmlSettings();
	bool read(QString const & filename);
	void loadTransferFunction(iATransferFunction* transferFunction);
	void saveTransferFunction(iATransferFunction* transferFunction);
	QDomNode node(QString const & nodeName);
	bool hasElement(QString const & nodeName) const;
	QDomElement createElement(QString const & elementName);
	QDomElement createElement(QString const & elementName, QDomNode parent);
	QDomElement documentElement();
	QDomDocument & document();
	static void loadTransferFunction(QDomNode const & functionsNode, iATransferFunction* transferFunction);
	void save(QString const & fileName);
private:
	void removeNode(QString const & str);
};
