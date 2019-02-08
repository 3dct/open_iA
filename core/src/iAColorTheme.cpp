/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
#include "iAColorTheme.h"

#include <cassert>

iAColorTheme::iAColorTheme(QString const & name):
	m_name(name)
{}

iAColorTheme::~iAColorTheme()
{}

QString const & iAColorTheme::GetName() const
{
	return m_name;
}


// iAVectorColorTheme

iAVectorColorTheme::iAVectorColorTheme(QString const & name): iAColorTheme(name)
{}

QColor iAVectorColorTheme::ErrorColor(255, 0, 0);

size_t iAVectorColorTheme::size() const
{
	return m_colors.size();
}

void iAVectorColorTheme::AddColor(QColor const & color)
{
	m_colors.push_back(color);
}

QColor const & iAVectorColorTheme::GetColor(int idx) const
{
	if (idx >= m_colors.size())
	{
		return ErrorColor;
	}
	return m_colors[idx];
}


// iASingleColorTheme

iASingleColorTheme::iASingleColorTheme(QString const & name, QColor  const & color):
	iAColorTheme(name), m_color(color)
{}

QColor const & iASingleColorTheme::GetColor(int idx) const
{
	return m_color;
}

size_t iASingleColorTheme::size() const
{
	return std::numeric_limits<size_t>::max();
}


// iAColorThemeManager

iAColorThemeManager const & iAColorThemeManager::GetInstance()
{
	static iAColorThemeManager manager;
	return manager;
}

iAColorThemeManager::iAColorThemeManager()
{
	// TODO: read from file?

	iASingleColorTheme* white = new iASingleColorTheme("White", QColor(255, 255, 255));
	m_themes.insert(white->GetName(), white);
	iASingleColorTheme* gray = new iASingleColorTheme("Gray", QColor(127, 127, 127));
	m_themes.insert(gray->GetName(), gray);
	iASingleColorTheme* black = new iASingleColorTheme("Black", QColor(0, 0, 0));
	m_themes.insert(black->GetName(), black);

	// source: http://mkweb.bcgsc.ca/brewer/swatches/brewer.txt

	iAVectorColorTheme* accent = new iAVectorColorTheme("Brewer Accent (max. 8)");
	accent->AddColor(QColor(127, 201, 127));
	accent->AddColor(QColor(190, 174, 212));
	accent->AddColor(QColor(253, 192, 134));
	accent->AddColor(QColor(255, 255, 153));
	accent->AddColor(QColor(56,  108, 176));
	accent->AddColor(QColor(240,   2, 127));
	accent->AddColor(QColor(191,  91,  23));
	accent->AddColor(QColor(102, 102, 102));
	m_themes.insert(accent->GetName(), accent);

	iAVectorColorTheme* dark2 = new iAVectorColorTheme("Brewer Dark2 (max. 8)");
	dark2->AddColor(QColor( 27,158,119));
	dark2->AddColor(QColor(217, 95,  2));
	dark2->AddColor(QColor(117,112,179));
	dark2->AddColor(QColor(231, 41,138));
	dark2->AddColor(QColor(102,166, 30));
	dark2->AddColor(QColor(230,171,  2));
	dark2->AddColor(QColor(166,118, 29));
	dark2->AddColor(QColor(102,102,102));
	m_themes.insert(dark2->GetName(), dark2);

	iAVectorColorTheme* paired = new iAVectorColorTheme("Brewer Paired (max. 12)");
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
	m_themes.insert(paired->GetName(), paired);

	iAVectorColorTheme* set1 = new iAVectorColorTheme("Brewer Set1 (max. 9)");
	set1->AddColor(QColor(228, 26, 28));
	set1->AddColor(QColor( 55,126,184));
	set1->AddColor(QColor( 77,175, 74));
	set1->AddColor(QColor(152, 78,163));
	set1->AddColor(QColor(255,127,  0));
	set1->AddColor(QColor(255,255, 51));
	set1->AddColor(QColor(166, 86, 40));
	set1->AddColor(QColor(247,129,191));
	set1->AddColor(QColor(153,153,153));
	m_themes.insert(set1->GetName(), set1);

	iAVectorColorTheme* set2 = new iAVectorColorTheme("Brewer Set2 (max. 8)");
	set2->AddColor(QColor(102,194,165));
	set2->AddColor(QColor(252,141, 98));
	set2->AddColor(QColor(141,160,203));
	set2->AddColor(QColor(231,138,195));
	set2->AddColor(QColor(166,216, 84));
	set2->AddColor(QColor(255,217, 47));
	set2->AddColor(QColor(229,196,148));
	set2->AddColor(QColor(179,179,179));
	m_themes.insert(set2->GetName(), set2);

	iAVectorColorTheme* set3 = new iAVectorColorTheme("Brewer Set3 (max. 12)");
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
	m_themes.insert(set3->GetName(), set3);

	// source: http://www.mulinblog.com/a-color-palette-optimized-for-data-visualization/
	iAVectorColorTheme* few = new iAVectorColorTheme("Few (max. 9)");
	few->AddColor(QColor( 77, 77, 77));
	few->AddColor(QColor( 93,165,218));
	few->AddColor(QColor(250,164, 58));
	few->AddColor(QColor( 96,189,104));
	few->AddColor(QColor(241,124,176));
	few->AddColor(QColor(178,145, 47));
	few->AddColor(QColor(178,118,178));
	few->AddColor(QColor(222,207, 63));
	few->AddColor(QColor(241, 88, 84));
	m_themes.insert(few->GetName(), few);


	iAVectorColorTheme* grayScale17 = new iAVectorColorTheme("Grayscale (best for 1,2,3,5,9,17 labels; max. 17)");
	grayScale17->AddColor(QColor(  0,  0,  0));
	grayScale17->AddColor(QColor(255,255,255));
	grayScale17->AddColor(QColor(128,128,128));
	grayScale17->AddColor(QColor( 64, 64, 64));
	grayScale17->AddColor(QColor(192,192,192));
	grayScale17->AddColor(QColor( 32, 32, 32));
	grayScale17->AddColor(QColor( 96, 96, 96));
	grayScale17->AddColor(QColor(160,160,160));
	grayScale17->AddColor(QColor(224,224,224));
	grayScale17->AddColor(QColor( 16, 16, 16));
	grayScale17->AddColor(QColor( 48, 48, 48));
	grayScale17->AddColor(QColor( 80, 80, 80));
	grayScale17->AddColor(QColor(112,112,112));
	grayScale17->AddColor(QColor(144,144,144));
	grayScale17->AddColor(QColor(176,176,176));
	grayScale17->AddColor(QColor(208,208,208));
	grayScale17->AddColor(QColor(240,240,240));
	m_themes.insert(grayScale17->GetName(), grayScale17);

	iAVectorColorTheme* grayScale4 = new iAVectorColorTheme("Grayscale (ideal&max. 4)");
	grayScale4->AddColor(QColor(  0,  0,  0));
	grayScale4->AddColor(QColor( 85, 85, 85));
	grayScale4->AddColor(QColor(170,170,170));
	grayScale4->AddColor(QColor(255,255,255));
	m_themes.insert(grayScale4->GetName(), grayScale4);

	iAVectorColorTheme* grayScale6 = new iAVectorColorTheme("Grayscale (ideal&max. 6)");
	grayScale6->AddColor(QColor(  0,  0,  0));
	grayScale6->AddColor(QColor( 51, 51, 51));
	grayScale6->AddColor(QColor(102,102,102));
	grayScale6->AddColor(QColor(153,153,153));
	grayScale6->AddColor(QColor(204,204,204));
	grayScale6->AddColor(QColor(255,255,255));
	m_themes.insert(grayScale6->GetName(), grayScale6);

	// themes from https://www.materialui.co/colors
	iAVectorColorTheme* materialRed = new iAVectorColorTheme("Material red (max. 10)");
	materialRed->AddColor(QColor(255, 235, 238));
	materialRed->AddColor(QColor(255, 205, 210));
	materialRed->AddColor(QColor(239,154,154));
	materialRed->AddColor(QColor(229, 115, 115));
	materialRed->AddColor(QColor(239, 83, 80));
	materialRed->AddColor(QColor(244, 67, 54));
	materialRed->AddColor(QColor(229, 57, 53));
	materialRed->AddColor(QColor(211, 47, 47));
	materialRed->AddColor(QColor(198, 40, 40));
	materialRed->AddColor(QColor(183, 28, 28));
	m_themes.insert(materialRed->GetName(), materialRed);

	iAVectorColorTheme* materialBlue = new iAVectorColorTheme("Material blue (max. 10)");
	materialBlue->AddColor(QColor(227, 242, 253));
	materialBlue->AddColor(QColor(187, 222, 251));
	materialBlue->AddColor(QColor(144, 202, 249));
	materialBlue->AddColor(QColor(100, 181, 246));
	materialBlue->AddColor(QColor(66, 165, 245));
	materialBlue->AddColor(QColor(33, 150, 243));
	materialBlue->AddColor(QColor(30, 136, 229));
	materialBlue->AddColor(QColor(25, 118, 210));
	materialBlue->AddColor(QColor(21, 101, 192));
	materialBlue->AddColor(QColor(13, 71, 161));
	m_themes.insert(materialBlue->GetName(), materialBlue);

	iAVectorColorTheme* flatUI = new iAVectorColorTheme("Flat UI (max. 20)");
	flatUI->AddColor(QColor(26, 188, 156));
	flatUI->AddColor(QColor(46, 204, 113));
	flatUI->AddColor(QColor(52, 152, 219));
	flatUI->AddColor(QColor(155, 89, 182));
	flatUI->AddColor(QColor(52, 73, 94));
	flatUI->AddColor(QColor(22, 160, 133));
	flatUI->AddColor(QColor(39, 174, 96));
	flatUI->AddColor(QColor(41, 128, 185));
	flatUI->AddColor(QColor(142, 68, 173));
	flatUI->AddColor(QColor(44, 62, 80));
	flatUI->AddColor(QColor(241, 196, 15));
	flatUI->AddColor(QColor(230, 126, 34));
	flatUI->AddColor(QColor(231, 76, 60));
	flatUI->AddColor(QColor(236, 240, 241));
	flatUI->AddColor(QColor(149, 165, 166));
	flatUI->AddColor(QColor(243, 156, 18));
	flatUI->AddColor(QColor(211, 84, 0));
	flatUI->AddColor(QColor(192, 57, 43));
	flatUI->AddColor(QColor(189, 195, 199));
	flatUI->AddColor(QColor(127, 140, 141));
	m_themes.insert(flatUI->GetName(), flatUI);

	iAVectorColorTheme* metroColors = new iAVectorColorTheme("Metro Colors (max. 20)");
	metroColors->AddColor(QColor(164, 196, 0));
	metroColors->AddColor(QColor(96, 169, 23));
	metroColors->AddColor(QColor(0, 138, 0));
	metroColors->AddColor(QColor(0, 171, 169));
	metroColors->AddColor(QColor(27, 161, 226));
	metroColors->AddColor(QColor(0, 80, 239));
	metroColors->AddColor(QColor(106, 0, 255));
	metroColors->AddColor(QColor(170, 0, 255));
	metroColors->AddColor(QColor(244, 114, 208));
	metroColors->AddColor(QColor(216, 0, 115));
	metroColors->AddColor(QColor(162, 0, 37));
	metroColors->AddColor(QColor(229, 20, 0));
	metroColors->AddColor(QColor(250, 104, 0));
	metroColors->AddColor(QColor(240, 163, 10));
	metroColors->AddColor(QColor(227, 200, 0));
	metroColors->AddColor(QColor(130, 90, 44));
	metroColors->AddColor(QColor(109, 135, 100));
	metroColors->AddColor(QColor(100, 118, 135));
	metroColors->AddColor(QColor(118, 96, 138));
	metroColors->AddColor(QColor(160, 82, 45));
	m_themes.insert(metroColors->GetName(), metroColors);

	iAVectorColorTheme* sevenShadesOfBlue = new iAVectorColorTheme("Seven Shades of blue (max. 7)");
	sevenShadesOfBlue->AddColor(QColor(193, 217, 252));
	sevenShadesOfBlue->AddColor(QColor(132, 179, 250));
	sevenShadesOfBlue->AddColor(QColor(70, 142, 247));
	sevenShadesOfBlue->AddColor(QColor(9, 104, 245));
	sevenShadesOfBlue->AddColor(QColor(7, 78, 184));
	sevenShadesOfBlue->AddColor(QColor(4, 52, 122));
	sevenShadesOfBlue->AddColor(QColor(2, 26, 61));
	m_themes.insert(sevenShadesOfBlue->GetName(), sevenShadesOfBlue);

	iAVectorColorTheme* DVLColors = new iAVectorColorTheme("DVL-Metro Colors (max. 17)");
	DVLColors->AddColor(QColor(164, 196, 0));
	DVLColors->AddColor(QColor(96, 169, 23));
	DVLColors->AddColor(QColor(0, 138, 0));
	DVLColors->AddColor(QColor(0, 171, 169));
	DVLColors->AddColor(QColor(27, 161, 226));
	DVLColors->AddColor(QColor(0, 80, 239));
	DVLColors->AddColor(QColor(106, 0, 255));
	DVLColors->AddColor(QColor(170, 0, 255));
	DVLColors->AddColor(QColor(244, 114, 208));
	DVLColors->AddColor(QColor(162, 0, 37));
	DVLColors->AddColor(QColor(240, 163, 10));
	DVLColors->AddColor(QColor(227, 200, 0));
	DVLColors->AddColor(QColor(130, 90, 44));
	DVLColors->AddColor(QColor(109, 135, 100));
	DVLColors->AddColor(QColor(100, 118, 135));
	DVLColors->AddColor(QColor(118, 96, 138));
	DVLColors->AddColor(QColor(160, 82, 45));
	m_themes.insert(DVLColors->GetName(), DVLColors);

	iAVectorColorTheme* brewerQual1 = new iAVectorColorTheme("Brewer Qualitaive 1 (max. 8)");
	brewerQual1->AddColor(QColor(27, 158, 119));
	brewerQual1->AddColor(QColor(217, 95, 2));
	brewerQual1->AddColor(QColor(117, 112, 179));
	brewerQual1->AddColor(QColor(231, 41, 138));
	brewerQual1->AddColor(QColor(102, 166, 30));
	brewerQual1->AddColor(QColor(230, 171, 2));
	brewerQual1->AddColor(QColor(166, 118, 29));
	brewerQual1->AddColor(QColor(102, 102, 102));
	m_themes.insert(brewerQual1->GetName(), brewerQual1);
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
	if (it == m_themes.end())
	{
		return m_themes[0];
	}
	return *it;
}

QList<QString> iAColorThemeManager::GetAvailableThemes() const
{
	return m_themes.keys();
}
