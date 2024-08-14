// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAColorTheme.h"

#include "iALog.h"

#include <QMap>

#include <limits>
#include <vector>

iAColorTheme::iAColorTheme(QString const & name):
	m_name(name)
{}

iAColorTheme::~iAColorTheme()
{}

QString const & iAColorTheme::name() const
{
	return m_name;
}


//! Simple implementation of a color theme, storing the colors in a vector.
class iAVectorColorTheme : public iAColorTheme
{
public:
	iAVectorColorTheme(QString const& name);
	size_t size() const override;
	QColor const& color(size_t idx) const override;
	//! add a color to the theme (typically only necessary for theme creators)
	void addColor(QColor const&);
private:
	std::vector<QColor> m_colors;
	static QColor ErrorColor;
};

iAVectorColorTheme::iAVectorColorTheme(QString const & name): iAColorTheme(name)
{}

QColor iAVectorColorTheme::ErrorColor(255, 0, 0);

size_t iAVectorColorTheme::size() const
{
	return m_colors.size();
}

void iAVectorColorTheme::addColor(QColor const & color)
{
	m_colors.push_back(color);
}

QColor const & iAVectorColorTheme::color(size_t idx) const
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

QColor const & iASingleColorTheme::color(size_t /*idx*/) const
{
	return m_color;
}

size_t iASingleColorTheme::size() const
{
	return std::numeric_limits<size_t>::max();
}


// iAColorThemeManager

namespace
{
	QMap<QString, std::shared_ptr<iAColorTheme>>& themes()
	{
		static QMap<QString, std::shared_ptr<iAColorTheme>> themes;
		if (themes.empty())
		{
			// TODO: read from file?
			auto white = std::make_shared<iASingleColorTheme>("White", QColor(255, 255, 255));
			themes.insert(white->name(), white);
			auto gray = std::make_shared<iASingleColorTheme>("Gray", QColor(127, 127, 127));
			themes.insert(gray->name(), gray);
			auto black = std::make_shared<iASingleColorTheme>("Black", QColor(0, 0, 0));
			themes.insert(black->name(), black);

			// source: http://mkweb.bcgsc.ca/brewer/swatches/brewer.txt

			auto accent = std::make_shared<iAVectorColorTheme>("Brewer Accent (max. 8)");
			accent->addColor(QColor(127, 201, 127));
			accent->addColor(QColor(190, 174, 212));
			accent->addColor(QColor(253, 192, 134));
			accent->addColor(QColor(255, 255, 153));
			accent->addColor(QColor(56, 108, 176));
			accent->addColor(QColor(240, 2, 127));
			accent->addColor(QColor(191, 91, 23));
			accent->addColor(QColor(102, 102, 102));
			themes.insert(accent->name(), accent);

			auto dark2 = std::make_shared<iAVectorColorTheme>("Brewer Dark2 (max. 8)");
			dark2->addColor(QColor(27, 158, 119));
			dark2->addColor(QColor(217, 95, 2));
			dark2->addColor(QColor(117, 112, 179));
			dark2->addColor(QColor(231, 41, 138));
			dark2->addColor(QColor(102, 166, 30));
			dark2->addColor(QColor(230, 171, 2));
			dark2->addColor(QColor(166, 118, 29));
			dark2->addColor(QColor(102, 102, 102));
			themes.insert(dark2->name(), dark2);

			auto paired = std::make_shared<iAVectorColorTheme>("Brewer Paired (max. 12)");
			paired->addColor(QColor(166, 206, 227));
			paired->addColor(QColor(31, 120, 180));
			paired->addColor(QColor(178, 223, 138));
			paired->addColor(QColor(51, 160, 44));
			paired->addColor(QColor(251, 154, 153));
			paired->addColor(QColor(227, 26, 28));
			paired->addColor(QColor(253, 191, 111));
			paired->addColor(QColor(255, 127, 0));
			paired->addColor(QColor(202, 178, 214));
			paired->addColor(QColor(106, 61, 154));
			paired->addColor(QColor(255, 255, 153));
			paired->addColor(QColor(177, 89, 40));
			themes.insert(paired->name(), paired);

			auto set1 = std::make_shared<iAVectorColorTheme>("Brewer Set1 (max. 9)");
			set1->addColor(QColor(228, 26, 28));
			set1->addColor(QColor(55, 126, 184));
			set1->addColor(QColor(77, 175, 74));
			set1->addColor(QColor(152, 78, 163));
			set1->addColor(QColor(255, 127, 0));
			set1->addColor(QColor(255, 255, 51));
			set1->addColor(QColor(166, 86, 40));
			set1->addColor(QColor(247, 129, 191));
			set1->addColor(QColor(153, 153, 153));
			themes.insert(set1->name(), set1);

			auto set2 = std::make_shared<iAVectorColorTheme>("Brewer Set2 (max. 8)");
			set2->addColor(QColor(102, 194, 165));
			set2->addColor(QColor(252, 141, 98));
			set2->addColor(QColor(141, 160, 203));
			set2->addColor(QColor(231, 138, 195));
			set2->addColor(QColor(166, 216, 84));
			set2->addColor(QColor(255, 217, 47));
			set2->addColor(QColor(229, 196, 148));
			set2->addColor(QColor(179, 179, 179));
			themes.insert(set2->name(), set2);

			auto set3 = std::make_shared<iAVectorColorTheme>("Brewer Set3 (max. 12)");
			set3->addColor(QColor(141, 211, 199));
			set3->addColor(QColor(255, 255, 179));
			set3->addColor(QColor(190, 186, 218));
			set3->addColor(QColor(251, 128, 114));
			set3->addColor(QColor(128, 177, 211));
			set3->addColor(QColor(253, 180, 98));
			set3->addColor(QColor(179, 222, 105));
			set3->addColor(QColor(252, 205, 229));
			set3->addColor(QColor(217, 217, 217));
			set3->addColor(QColor(188, 128, 189));
			set3->addColor(QColor(204, 235, 197));
			set3->addColor(QColor(255, 237, 111));
			themes.insert(set3->name(), set3);

			// source: http://www.mulinblog.com/a-color-palette-optimized-for-data-visualization/
			auto few = std::make_shared<iAVectorColorTheme>("Few (max. 9)");
			few->addColor(QColor(77, 77, 77));
			few->addColor(QColor(93, 165, 218));
			few->addColor(QColor(250, 164, 58));
			few->addColor(QColor(96, 189, 104));
			few->addColor(QColor(241, 124, 176));
			few->addColor(QColor(178, 145, 47));
			few->addColor(QColor(178, 118, 178));
			few->addColor(QColor(222, 207, 63));
			few->addColor(QColor(241, 88, 84));
			themes.insert(few->name(), few);


			auto grayScale17 = std::make_shared<iAVectorColorTheme>("Grayscale (max. 17)");
			grayScale17->addColor(QColor(0, 0, 0));
			grayScale17->addColor(QColor(255, 255, 255));
			grayScale17->addColor(QColor(128, 128, 128));
			grayScale17->addColor(QColor(64, 64, 64));
			grayScale17->addColor(QColor(192, 192, 192));
			grayScale17->addColor(QColor(32, 32, 32));
			grayScale17->addColor(QColor(96, 96, 96));
			grayScale17->addColor(QColor(160, 160, 160));
			grayScale17->addColor(QColor(224, 224, 224));
			grayScale17->addColor(QColor(16, 16, 16));
			grayScale17->addColor(QColor(48, 48, 48));
			grayScale17->addColor(QColor(80, 80, 80));
			grayScale17->addColor(QColor(112, 112, 112));
			grayScale17->addColor(QColor(144, 144, 144));
			grayScale17->addColor(QColor(176, 176, 176));
			grayScale17->addColor(QColor(208, 208, 208));
			grayScale17->addColor(QColor(240, 240, 240));
			themes.insert(grayScale17->name(), grayScale17);

			auto grayScale4 = std::make_shared<iAVectorColorTheme>("Grayscale (ideal&max. 4)");
			grayScale4->addColor(QColor(0, 0, 0));
			grayScale4->addColor(QColor(85, 85, 85));
			grayScale4->addColor(QColor(170, 170, 170));
			grayScale4->addColor(QColor(255, 255, 255));
			themes.insert(grayScale4->name(), grayScale4);

			auto grayScale6 = std::make_shared<iAVectorColorTheme>("Grayscale (ideal&max. 6)");
			grayScale6->addColor(QColor(0, 0, 0));
			grayScale6->addColor(QColor(51, 51, 51));
			grayScale6->addColor(QColor(102, 102, 102));
			grayScale6->addColor(QColor(153, 153, 153));
			grayScale6->addColor(QColor(204, 204, 204));
			grayScale6->addColor(QColor(255, 255, 255));
			themes.insert(grayScale6->name(), grayScale6);

			// themes from https://www.materialui.co/colors
			auto materialRed = std::make_shared<iAVectorColorTheme>("Material red (max. 10)");
			materialRed->addColor(QColor(255, 235, 238));
			materialRed->addColor(QColor(255, 205, 210));
			materialRed->addColor(QColor(239, 154, 154));
			materialRed->addColor(QColor(229, 115, 115));
			materialRed->addColor(QColor(239, 83, 80));
			materialRed->addColor(QColor(244, 67, 54));
			materialRed->addColor(QColor(229, 57, 53));
			materialRed->addColor(QColor(211, 47, 47));
			materialRed->addColor(QColor(198, 40, 40));
			materialRed->addColor(QColor(183, 28, 28));
			themes.insert(materialRed->name(), materialRed);

			auto materialBlue = std::make_shared<iAVectorColorTheme>("Material blue (max. 10)");
			materialBlue->addColor(QColor(227, 242, 253));
			materialBlue->addColor(QColor(187, 222, 251));
			materialBlue->addColor(QColor(144, 202, 249));
			materialBlue->addColor(QColor(100, 181, 246));
			materialBlue->addColor(QColor(66, 165, 245));
			materialBlue->addColor(QColor(33, 150, 243));
			materialBlue->addColor(QColor(30, 136, 229));
			materialBlue->addColor(QColor(25, 118, 210));
			materialBlue->addColor(QColor(21, 101, 192));
			materialBlue->addColor(QColor(13, 71, 161));
			themes.insert(materialBlue->name(), materialBlue);

			auto flatUI = std::make_shared<iAVectorColorTheme>("Flat UI (max. 20)");
			flatUI->addColor(QColor(26, 188, 156));
			flatUI->addColor(QColor(46, 204, 113));
			flatUI->addColor(QColor(52, 152, 219));
			flatUI->addColor(QColor(155, 89, 182));
			flatUI->addColor(QColor(52, 73, 94));
			flatUI->addColor(QColor(22, 160, 133));
			flatUI->addColor(QColor(39, 174, 96));
			flatUI->addColor(QColor(41, 128, 185));
			flatUI->addColor(QColor(142, 68, 173));
			flatUI->addColor(QColor(44, 62, 80));
			flatUI->addColor(QColor(241, 196, 15));
			flatUI->addColor(QColor(230, 126, 34));
			flatUI->addColor(QColor(231, 76, 60));
			flatUI->addColor(QColor(236, 240, 241));
			flatUI->addColor(QColor(149, 165, 166));
			flatUI->addColor(QColor(243, 156, 18));
			flatUI->addColor(QColor(211, 84, 0));
			flatUI->addColor(QColor(192, 57, 43));
			flatUI->addColor(QColor(189, 195, 199));
			flatUI->addColor(QColor(127, 140, 141));
			themes.insert(flatUI->name(), flatUI);

			auto metroColors = std::make_shared<iAVectorColorTheme>("Metro Colors (max. 20)");
			metroColors->addColor(QColor(164, 196, 0));
			metroColors->addColor(QColor(96, 169, 23));
			metroColors->addColor(QColor(0, 138, 0));
			metroColors->addColor(QColor(0, 171, 169));
			metroColors->addColor(QColor(27, 161, 226));
			metroColors->addColor(QColor(0, 80, 239));
			metroColors->addColor(QColor(106, 0, 255));
			metroColors->addColor(QColor(170, 0, 255));
			metroColors->addColor(QColor(244, 114, 208));
			metroColors->addColor(QColor(216, 0, 115));
			metroColors->addColor(QColor(162, 0, 37));
			metroColors->addColor(QColor(229, 20, 0));
			metroColors->addColor(QColor(250, 104, 0));
			metroColors->addColor(QColor(240, 163, 10));
			metroColors->addColor(QColor(227, 200, 0));
			metroColors->addColor(QColor(130, 90, 44));
			metroColors->addColor(QColor(109, 135, 100));
			metroColors->addColor(QColor(100, 118, 135));
			metroColors->addColor(QColor(118, 96, 138));
			metroColors->addColor(QColor(160, 82, 45));
			themes.insert(metroColors->name(), metroColors);

			auto sevenShadesOfBlue = std::make_shared<iAVectorColorTheme>("Seven Shades of blue (max. 7)");
			sevenShadesOfBlue->addColor(QColor(193, 217, 252));
			sevenShadesOfBlue->addColor(QColor(132, 179, 250));
			sevenShadesOfBlue->addColor(QColor(70, 142, 247));
			sevenShadesOfBlue->addColor(QColor(9, 104, 245));
			sevenShadesOfBlue->addColor(QColor(7, 78, 184));
			sevenShadesOfBlue->addColor(QColor(4, 52, 122));
			sevenShadesOfBlue->addColor(QColor(2, 26, 61));
			themes.insert(sevenShadesOfBlue->name(), sevenShadesOfBlue);

			auto DVLColors = std::make_shared<iAVectorColorTheme>("DVL-Metro Colors (max. 17)");
			DVLColors->addColor(QColor(164, 196, 0));
			DVLColors->addColor(QColor(96, 169, 23));
			DVLColors->addColor(QColor(0, 138, 0));
			DVLColors->addColor(QColor(0, 171, 169));
			DVLColors->addColor(QColor(27, 161, 226));
			DVLColors->addColor(QColor(0, 80, 239));
			DVLColors->addColor(QColor(106, 0, 255));
			DVLColors->addColor(QColor(170, 0, 255));
			DVLColors->addColor(QColor(244, 114, 208));
			DVLColors->addColor(QColor(162, 0, 37));
			DVLColors->addColor(QColor(240, 163, 10));
			DVLColors->addColor(QColor(227, 200, 0));
			DVLColors->addColor(QColor(130, 90, 44));
			DVLColors->addColor(QColor(109, 135, 100));
			DVLColors->addColor(QColor(100, 118, 135));
			DVLColors->addColor(QColor(118, 96, 138));
			DVLColors->addColor(QColor(160, 82, 45));
			themes.insert(DVLColors->name(), DVLColors);
		}
		return themes;
	}
}

iAColorTheme const * iAColorThemeManager::theme(QString const & name)
{
	auto it = themes().find(name);
	if (it == themes().end())
	{
		auto t = *themes().begin();
		LOG(lvlWarn, QString("iAColorThemeManager: Did not find requested theme '%1'; returning first available theme '%2' instead.")
			.arg(name).arg(t->name()));
		return t.get();
	}
	return it->get();
}

QStringList iAColorThemeManager::availableThemes()
{
	return QStringList(themes().keys());
}
