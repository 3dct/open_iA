// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iabase_export.h"

#include <QDomDocument>

class iATransferFunction;

//! Class for loading and storing settings (transfer functions) to/from XML documents.
class iAbase_API iAXmlSettings
{
private:
	QDomDocument domDocument;
public:
	iAXmlSettings();
	bool read(QString const & filename);
	bool loadTransferFunction(iATransferFunction* transferFunction);
	void saveTransferFunction(iATransferFunction* transferFunction);
	QDomNode node(QString const & nodeName);
	bool hasElement(QString const & nodeName) const;
	QDomElement createElement(QString const & elementName);
	QDomElement createElement(QString const & elementName, QDomNode parent);
	QDomElement documentElement();
	QDomDocument & document();
	static bool loadTransferFunction(QDomNode const & functionsNode, iATransferFunction* transferFunction);
	void save(QString const & fileName);
	bool fromString(QString const& xmlStr);
	QString toString() const;
private:
	void removeNode(QString const & str);
};
