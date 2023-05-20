// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iabase_export.h"

#include <QColor>
#include <QMap>
#include <QStringList>

#include <vector>

class QString;

//! Interface for (qualitative) color themes. Holds a number of colors (which can be distinguished easily)
class iAbase_API iAColorTheme
{
public:
	iAColorTheme(QString const & name);
	virtual ~iAColorTheme();
	//! returns the number of colors in this theme
	virtual size_t size() const =0;
	//! returns the color with the given index in this theme
	virtual QColor const & color(size_t idx) const =0;
	//! get the name of the color theme
	QString const & name() const;
private:
	QString m_name;
};

//! Simple implementation of a color theme, storing the colors in a vector.
class iAbase_API iAVectorColorTheme: public iAColorTheme
{
public:
	iAVectorColorTheme(QString const &  name);
	size_t size() const override;
	QColor const & color(size_t idx) const override;
	//! add a color to the theme (typically only necessary for theme creators)
	void addColor(QColor const &);
private:
	std::vector<QColor> m_colors;
	static QColor ErrorColor;
};

//! Simplest possible color scheme: only one single color for all items.
class iAbase_API iASingleColorTheme : public iAColorTheme
{
public:
	iASingleColorTheme(QString const & name, QColor const & color);
	size_t size() const override;
	QColor const & color(size_t idx) const override;
private:
	QColor m_color;
};

//! Manager for color themes (iAColorTheme).
//! Internally creates the qualitative color themes from Color Brewer
//! (http://mkweb.bcgsc.ca/brewer/swatches/brewer.txt) as well as from a few other
//! sources and provides access to their names as well as the single themes.
class iAbase_API iAColorThemeManager
{
public:
	//! only ever need one of those (Singleton)
	static iAColorThemeManager const & instance();
	//! Get the list of all available themes
	QStringList availableThemes() const;
	//! Get a theme by name
	iAColorTheme const * theme(QString const & name) const;
private:
	iAColorThemeManager();
	~iAColorThemeManager();

	QMap<QString, iAColorTheme*> m_themes;
};
