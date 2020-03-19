/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#pragma once

namespace PeriodicTable
{

const int LineCount = 7;
const int ExtraTableLines = 2;

const int ElementsPerLine[LineCount] =
{
	 2,
	 8,
	 8,
	18,
	18,
	32,
	32
};

const int ElementsOnLeftSide[LineCount] =
{
	 1,
	 2,
	 2,
	 2,
	 2,
	 2,
	 2
};

const int MaxElementsPerLine = 18;

struct iAElement
{
	int number;
	std::string shortname;
	std::string name;
	iAElement(int no, std::string const & shortnm, std::string const & nm):
		number(no), shortname(shortnm), name(nm)
	{
	}
};

const int ElementCount = 118;

const iAElement elements[] =
{
	iAElement(1, "H", "Hydrogen"),
	iAElement(2, "He", "Helium"),
	iAElement(3, "Li", "Lithium"),
	iAElement(4, "Be", "Beryllium"),
	iAElement(5, "B", "Boron"),
	iAElement(6, "C", "Carbon"),
	iAElement(7, "N", "Nitrogen"),
	iAElement(8, "O", "Oxygen"),
	iAElement(9, "F", "Fluorine"),
	iAElement(10, "Ne", "Neon"),
	iAElement(11, "Na", "Sodium"),
	iAElement(12, "Mg", "Magnesium"),
	iAElement(13, "Al", "Aluminum"),
	iAElement(14, "Si", "Silicon"),
	iAElement(15, "P", "Phosphorus"),
	iAElement(16, "S", "Sulfur"),
	iAElement(17, "Cl", "Chlorine"),
	iAElement(18, "Ar", "Argon"),
	iAElement(19, "K", "Potassium"),
	iAElement(20, "Ca", "Calcium"),
	iAElement(21, "Sc", "Scandium"),
	iAElement(22, "Ti", "Titanium"),
	iAElement(23, "V", "Vanadium"),
	iAElement(24, "Cr", "Chromium"),
	iAElement(25, "Mn", "Manganese"),
	iAElement(26, "Fe", "Iron"),
	iAElement(27, "Co", "Cobalt"),
	iAElement(28, "Ni", "Nickel"),
	iAElement(29, "Cu", "Copper"),
	iAElement(30, "Zn", "Zinc"),
	iAElement(31, "Ga", "Gallium"),
	iAElement(32, "Ge", "Germanium"),
	iAElement(33, "As", "Arsenic"),
	iAElement(34, "Se", "Selenium"),
	iAElement(35, "Br", "Bromine"),
	iAElement(36, "Kr", "Krypton"),
	iAElement(37, "Rb", "Rubidium"),
	iAElement(38, "Sr", "Strontium"),
	iAElement(39, "Y", "Yttrium"),
	iAElement(40, "Zr", "Zirconium"),
	iAElement(41, "Nb", "Niobium"),
	iAElement(42, "Mo", "Molybdenum"),
	iAElement(43, "Tc", "Technetium"),
	iAElement(44, "Ru", "Ruthenium"),
	iAElement(45, "Rh", "Rhodium"),
	iAElement(46, "Pd", "Palladium"),
	iAElement(47, "Ag", "Silver"),
	iAElement(48, "Cd", "Cadmium"),
	iAElement(49, "In", "Indium"),
	iAElement(50, "Sn", "Tin"),
	iAElement(51, "Sb", "Antimony"),
	iAElement(52, "Te", "Tellurium"),
	iAElement(53, "I", "Iodine"),
	iAElement(54, "Xe", "Xenon"),
	iAElement(55, "Cs", "Cesium"),
	iAElement(56, "Ba", "Barium"),
	iAElement(57, "La", "Lanthanum"),
	iAElement(58, "Ce", "Cerium"),
	iAElement(59, "Pr", "Praseodymium"),
	iAElement(60, "Nd", "Neodymium"),
	iAElement(61, "Pm", "Promethium"),
	iAElement(62, "Sm", "Samarium"),
	iAElement(63, "Eu", "Europium"),
	iAElement(64, "Gd", "Gadolinium"),
	iAElement(65, "Tb", "Terbium"),
	iAElement(66, "Dy", "Dysprosium"),
	iAElement(67, "Ho", "Holmium"),
	iAElement(68, "Er", "Erbium"),
	iAElement(69, "Tm", "Thulium"),
	iAElement(70, "Yb", "Ytterbium"),
	iAElement(71, "Lu", "Lutetium"),
	iAElement(72, "Hf", "Hafnium"),
	iAElement(73, "Ta", "Tantalum"),
	iAElement(74, "W", "Tungsten"),
	iAElement(75, "Re", "Rhenium"),
	iAElement(76, "Os", "Osmium"),
	iAElement(77, "Ir", "Iridium"),
	iAElement(78, "Pt", "Platinum"),
	iAElement(79, "Au", "Gold"),
	iAElement(80, "Hg", "Mercury"),
	iAElement(81, "Tl", "Thallium"),
	iAElement(82, "Pb", "Lead"),
	iAElement(83, "Bi", "Bismuth"),
	iAElement(84, "Po", "Polonium"),
	iAElement(85, "At", "Astatine"),
	iAElement(86, "Rn", "Radon"),
	iAElement(87, "Fr", "Francium"),
	iAElement(88, "Ra", "Radium"),
	iAElement(89, "Ac", "Actinium"),
	iAElement(90, "Th", "Thorium"),
	iAElement(91, "Pa", "Protactinium"),
	iAElement(92, "U", "Uranium"),
	iAElement(93, "Np", "Neptunium"),
	iAElement(94, "Pu", "Plutonium"),
	iAElement(95, "Am", "Americium"),
	iAElement(96, "Cm", "Curium"),
	iAElement(97, "Bk", "Berkelium"),
	iAElement(98, "Cf", "Californium"),
	iAElement(99, "Es", "Einsteinium"),
	iAElement(100, "Fm", "Fermium"),
	iAElement(101, "Md", "Mendelevium"),
	iAElement(102, "No", "Nobelium"),
	iAElement(103, "Lr", "Lawrencium"),
	iAElement(104, "Rf", "Rutherfordium"),
	iAElement(105, "Db", "Dubnium"),
	iAElement(106, "Sg", "Seaborgium"),
	iAElement(107, "Bh", "Bohrium"),
	iAElement(108, "Hs", "Hassium"),
	iAElement(109, "Mt", "Meitnerium"),
	iAElement(110, "Ds", "Darmstadtium"),
	iAElement(111, "Rg", "Roentgenium"),
	iAElement(112, "Cn", "Copernicum"),
	iAElement(113, "Uut", "Ununtrium"),
	iAElement(114, "Fl", "Flerovium"),
	iAElement(115, "Uup", "Ununpentium"),
	iAElement(116, "Lv", "Livermorium"),
	iAElement(117, "Uus", "Ununseptium"),
	iAElement(118, "Uuo", "Ununoctium")
};

} // namespace PeriodicElements
