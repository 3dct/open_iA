/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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
 
#include "pch.h"
#include "iAColorTheme.h"

#include <cassert>


iAColorTheme const * iAColorTheme::NullTheme()
{
	static iAColorTheme singletonNullTheme;
	return &singletonNullTheme;
}

QColor iAColorTheme::ErrorColor(255, 0, 0);

size_t iAColorTheme::size() const
{
	return m_colors.size();
}

void iAColorTheme::AddColor(QColor const & color)
{
	m_colors.push_back(color);
}

QColor const & iAColorTheme::GetColor(int idx) const
{
	if (idx >= m_colors.size())
	{
		return ErrorColor;
	}
	return m_colors[idx];
}

iAColorThemeManager const & iAColorThemeManager::GetInstance()
{
	static iAColorThemeManager manager;
	return manager;
}

iAColorThemeManager::iAColorThemeManager()
{
	// source: http://mkweb.bcgsc.ca/brewer/swatches/brewer.txt

	// TODO: read from file?

	iAColorTheme* accent = new iAColorTheme();
	accent->AddColor(QColor(127, 201, 127));
	accent->AddColor(QColor(190, 174, 212));
	accent->AddColor(QColor(253, 192, 134));
	accent->AddColor(QColor(255, 255, 153));
	accent->AddColor(QColor(56,  108, 176));
	accent->AddColor(QColor(240,   2, 127));
	accent->AddColor(QColor(191,  91,  23));
	accent->AddColor(QColor(102, 102, 102));
	m_themes.insert("Brewer Accent (max. 8)", accent);

	iAColorTheme* dark2 = new iAColorTheme();
	dark2->AddColor(QColor( 27,158,119));
	dark2->AddColor(QColor(217, 95,  2));
	dark2->AddColor(QColor(117,112,179));
	dark2->AddColor(QColor(231, 41,138));
	dark2->AddColor(QColor(102,166, 30));
	dark2->AddColor(QColor(230,171,  2));
	dark2->AddColor(QColor(166,118, 29));
	dark2->AddColor(QColor(102,102,102));
	m_themes.insert("Brewer Dark2 (max. 8)", dark2);
	
	iAColorTheme* paired = new iAColorTheme();
	paired->AddColor(QColor(166, 206, 227));
	paired->AddColor(QColor( 31, 120, 180));
	paired->AddColor(QColor(178, 223, 138));
	paired->AddColor(QColor( 51, 160,  44));
	paired->AddColor(QColor(251, 154, 153));
	paired->AddColor(QColor(227,  26,  28));
	paired->AddColor(QColor(253, 191, 111));
	paired->AddColor(QColor(255, 127,   0));
	paired->AddColor(QColor(202, 178, 214));
	paired->AddColor(QColor(106,  61, 154));
	paired->AddColor(QColor(255, 255, 153));
	paired->AddColor(QColor(177,  89,  40));
	m_themes.insert("Brewer Paired (max. 12)", paired);

	iAColorTheme* set1 = new iAColorTheme();
	set1->AddColor(QColor(228, 26, 28));
	set1->AddColor(QColor( 55,126,184));
	set1->AddColor(QColor( 77,175, 74));
	set1->AddColor(QColor(152, 78,163));
	set1->AddColor(QColor(255,127,  0));
	set1->AddColor(QColor(255,255, 51));
	set1->AddColor(QColor(166, 86, 40));
	set1->AddColor(QColor(247,129,191));
	set1->AddColor(QColor(153,153,153));
	m_themes.insert("Brewer Set1 (max. 9)", set1);

	iAColorTheme* set2 = new iAColorTheme();
	set2->AddColor(QColor(102,194,165));
	set2->AddColor(QColor(252,141, 98));
	set2->AddColor(QColor(141,160,203));
	set2->AddColor(QColor(231,138,195));
	set2->AddColor(QColor(166,216, 84));
	set2->AddColor(QColor(255,217, 47));
	set2->AddColor(QColor(229,196,148));
	set2->AddColor(QColor(179,179,179));
	m_themes.insert("Brewer Set2 (max. 8)", set2);

	iAColorTheme* set3 = new iAColorTheme();
	set3->AddColor(QColor(141, 211, 199));
	set3->AddColor(QColor(255, 255, 179));
	set3->AddColor(QColor(190, 186, 218));
	set3->AddColor(QColor(251, 128, 114));
	set3->AddColor(QColor(128, 177, 211));
	set3->AddColor(QColor(253, 180,  98));
	set3->AddColor(QColor(179, 222, 105));
	set3->AddColor(QColor(252, 205, 229));
	set3->AddColor(QColor(217, 217, 217));
	set3->AddColor(QColor(188, 128, 189));
	set3->AddColor(QColor(204, 235, 197));
	set3->AddColor(QColor(255, 237, 111));
	m_themes.insert("Brewer Set3 (max. 12)", set3);
	
	// source: http://www.mulinblog.com/a-color-palette-optimized-for-data-visualization/
	iAColorTheme* few = new iAColorTheme();
	few->AddColor(QColor( 77, 77, 77));
	few->AddColor(QColor( 93,165,218));
	few->AddColor(QColor(250,164, 58));
	few->AddColor(QColor( 96,189,104));
	few->AddColor(QColor(241,124,176));
	few->AddColor(QColor(178,145, 47));
	few->AddColor(QColor(178,118,178));
	few->AddColor(QColor(222,207, 63));
	few->AddColor(QColor(241, 88, 84));
	m_themes.insert("Few (max. 9)", few);
}

iAColorThemeManager::~iAColorThemeManager()
{
	for (QString key: m_themes.keys())
	{
		delete m_themes[key];
	}
}

iAColorTheme const * iAColorThemeManager::GetTheme(QString const & name) const
{
	QMap<QString, iAColorTheme*>::const_iterator it = m_themes.find(name);
	assert (it != m_themes.end());
	return *it;
}

QList<QString> iAColorThemeManager::GetAvailableThemes() const
{
	return m_themes.keys();
}