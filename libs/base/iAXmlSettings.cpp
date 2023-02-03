// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAXmlSettings.h"

#include "iALog.h"
#include "iATransferFunction.h"

#include <vtkPiecewiseFunction.h>
#include <vtkColorTransferFunction.h>

#include <QFile>
#include <QTextStream>

iAXmlSettings::iAXmlSettings()
{
	QDomElement root = domDocument.createElement("settings");
	domDocument.appendChild(root);
}

bool iAXmlSettings::read(QString const & filename)
{
	QFile file(filename);
	// TODO: better error handling!
	if (!file.open(QIODevice::ReadOnly))
	{
		return false;
	}
	if (!domDocument.setContent(&file))
	{
		file.close();
		return false;
	}
	file.close();
	if (!domDocument.hasChildNodes() ||
		domDocument.documentElement().tagName() != "settings")
	{
		//QDomElement root = domDocument.createElement("settings");
		//domDocument.appendChild(root);
		return false;
	}
	return true;
}

bool iAXmlSettings::loadTransferFunction(iATransferFunction* transferFunction)
{
	QDomElement root = domDocument.documentElement();
	QDomNode functionsNode = root.namedItem("functions");
	if (!functionsNode.isElement())
	{
		return false;
	}
	return iAXmlSettings::loadTransferFunction(functionsNode, transferFunction);
}

QDomNode iAXmlSettings::node(QString const & nodeName)
{
	QDomElement root = domDocument.documentElement();
	return root.namedItem(nodeName);
}

bool iAXmlSettings::hasElement(QString const & nodeName) const
{
	QDomNode node = domDocument.documentElement().namedItem(nodeName);
	return node.isElement();
}

QDomElement iAXmlSettings::createElement(QString const & elementName)
{
	return createElement(elementName, domDocument.documentElement());
}

QDomElement iAXmlSettings::createElement(QString const & elementName, QDomNode parent)
{
	auto element = domDocument.createElement(elementName);
	parent.appendChild(element);
	return element;
}

QDomElement iAXmlSettings::documentElement()
{
	return domDocument.documentElement();
}

QDomDocument & iAXmlSettings::document()
{
	return domDocument;
}

bool iAXmlSettings::loadTransferFunction(QDomNode const & functionsNode, iATransferFunction* transferFunction)
{
	QDomNode transferNode = functionsNode.namedItem("transfer");
	if (!transferNode.isElement())
	{
		LOG(lvlError, "'transfer' node not found in given XML file, aborting load of transfer function!");
		return false;
	}
	transferFunction->opacityTF()->RemoveAllPoints();
	transferFunction->colorTF()->RemoveAllPoints();
	QDomNodeList list = transferNode.childNodes();
	for (int n = 0; n < int(list.length()); n++)
	{
		QDomNode node = list.item(n);
		QDomNamedNodeMap attributes = node.attributes();
		double value = attributes.namedItem("value").nodeValue().toDouble();
		double opacity = attributes.namedItem("opacity").nodeValue().toDouble();
		double red = attributes.namedItem("red").nodeValue().toDouble();
		double green = attributes.namedItem("green").nodeValue().toDouble();
		double blue = attributes.namedItem("blue").nodeValue().toDouble();
		//value = clamp(range[0], range[1], value);
		transferFunction->opacityTF()->AddPoint(value, opacity);
		transferFunction->colorTF()->AddRGBPoint(value, red, green, blue);
	}
	transferFunction->colorTF()->Build();
	return true;
}

void iAXmlSettings::saveTransferFunction(iATransferFunction* transferFunction)
{
	QDomNode functionsNode = domDocument.documentElement().namedItem("functions");
	if (!functionsNode.isElement())
	{
		functionsNode = createElement("functions");
	}
	QDomElement transferElement = domDocument.createElement("transfer");

	for (int i = 0; i < transferFunction->opacityTF()->GetSize(); i++)
	{
		double opacityTFValue[4];
		double colorTFValue[6];
		transferFunction->opacityTF()->GetNodeValue(i, opacityTFValue);
		transferFunction->colorTF()->GetNodeValue(i, colorTFValue);

		QDomElement nodeElement = domDocument.createElement("node");
		nodeElement.setAttribute("value", QObject::tr("%1").arg(opacityTFValue[0]));
		nodeElement.setAttribute("opacity", QObject::tr("%1").arg(opacityTFValue[1]));
		nodeElement.setAttribute("red", QObject::tr("%1").arg(colorTFValue[1]));
		nodeElement.setAttribute("green", QObject::tr("%1").arg(colorTFValue[2]));
		nodeElement.setAttribute("blue", QObject::tr("%1").arg(colorTFValue[3]));
		transferElement.appendChild(nodeElement);
	}

	functionsNode.appendChild(transferElement);
}

void iAXmlSettings::save(QString const & filename)
{
	QFile file(filename);
	file.open(QIODevice::WriteOnly);
	QTextStream ts(&file);
	ts << domDocument.toString();
	file.close();
}

bool iAXmlSettings::fromString(QString const& xmlStr)
{
	return domDocument.setContent(xmlStr);
}

QString iAXmlSettings::toString() const
{
	return domDocument.toString();
}

void iAXmlSettings::removeNode(QString const & str)
{
	QDomNode rootNode = domDocument.documentElement();
	QDomNodeList list = rootNode.childNodes();
	for (int n = 0; n < int(list.length()); n++)
	{
		QDomNode node = list.item(n);
		if (node.nodeName() == str)
		{
			rootNode.removeChild(node);
			break;
		}
	}
}
