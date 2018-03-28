/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#pragma once

#include "open_iA_Core_export.h"

#include <QColor>
#include <QList>
#include <QMap>

#include <vector>

class QString;
class QStringList;

//! A simple (qualitative) color theme. Holds a number of colors (which can be distinguished easily)
class open_iA_Core_API iAColorTheme
{
public:
	iAColorTheme(QString const & name);
	//! returns the number of colors in this theme
	virtual size_t size() const =0;
	//! returns the color with the given index in this theme
	virtual QColor const & GetColor(int idx) const =0;
	//! get the name of the color theme
	QString const & GetName() const;
private:
	QString m_name;
};

class open_iA_Core_API iAVectorColorTheme: public iAColorTheme
{
public:
	iAVectorColorTheme(QString const &  name);
	size_t size() const override;
	QColor const & GetColor(int idx) const override;
	//! add a color to the theme (typically only necessary for theme creators)
	void AddColor(QColor const &);
private:
	std::vector<QColor> m_colors;
	static QColor ErrorColor;
};

class open_iA_Core_API iASingleColorTheme : public iAColorTheme
{
public:
	iASingleColorTheme(QString const & name, QColor const & color);
	size_t size() const override;
	QColor const & GetColor(int idx) const override;
private:
	QColor m_color;
};

//! Manager for color themes. Internally creates the qualitative color themes from
//! Color Brewer (http://mkweb.bcgsc.ca/brewer/swatches/brewer.txt) and provides
//! access to their names as well as the single themes.
class open_iA_Core_API iAColorThemeManager
{
public:
	//! only every need one of those
	static iAColorThemeManager const & GetInstance();
	//! Get the list of all available themes
	QList<QString> GetAvailableThemes() const;
	//! Get a theme by name
	iAColorTheme const * GetTheme(QString const & name) const;
private:
	iAColorThemeManager();
	~iAColorThemeManager();

	QMap<QString, iAColorTheme*> m_themes;
};
