/*
 * XmdvTool.h
 *
 *  Created on: Apr 1, 2008
 *      Author: Zaixian Xie
 *
 */

/*
 * This header file should be included by all of cpp
 * files in this project (XmdvTool).
 * It includes some important header files to declare
 * macros, global variables, data types.
 * It also declares a namespace XmdvTool for miscellaneous identifiers
 * that need to be global-like, mainly some enumeration constants.
 */

#ifndef XMDVTOOL_H_
#define XMDVTOOL_H_

#include "macros.h"
#include "Globals.h"
#include <cmath>

#include <QString>
#include <QIcon>

extern Globals g_globals;

namespace XmdvTool {
	// the type of pipeline
	enum PLTYPE {
		// the normal type for flat display
		PLTYPE_FLATNORMAL			= 0,
		// The pipeline type to support structure-based brush
		PLTYPE_SBB					= 1,
		// The pipeline type to support dimension reduction (InterRing)
		PLTYPE_DIMR					= 2,
	};

	// visualization type
	enum VISTYPE {
		VISTYPE_PA = 0, //Parallel coordinates
		VISTYPE_SC = 1, //Scatterplot matrices
		VISTYPE_GL = 2, //glyphs
		VISTYPE_DI = 3, //Dimension Stacking
		VISTYPE_PI = 4, //Pixel-oriented display
	};

	// operator type
	enum OPERATORTYPE {
		OPERATOR_NORMAL 	= 0, //a regular operator
		OPERATOR_VIEW 		= 1, //a view operator, e.g. dimension on/off
	};

	// the data modifier type
	enum MODIFIERTYPE {
		MODIFIER_NULL			 	= 0,	// the base class of modifier, which is not a real modifier
		MODIFIER_HIGHLIGHT			= 1, 	// the modifier to highlight a subset of dataset
		MODIFIER_DIMOOO				= 2, 	// the modifier to do dimension on/off/reordering
		MODIFIER_ROWINDEX			= 3, 	// the modifier to do ordering or sampling.
		// A new row index will be generated to represent the selected rows after ordering and sampling
		MODIFIER_CLUSTERNODEMAX		= 5,	// the maximum values for dimensions in a cluster node
		MODIFIER_CLUSTERNODEMIN		= 6,	// the minimum values for dimensions in a cluster node
		MODIFIER_CLUSTERCOLOR		= 7,	// the cluster color used in SBB mode
		MODIFIER_CLUSTERHIGHLIGHT	= 8,	// the cluster color used in SBB mode
		MODIFIER_CLUSTERENTRIES		= 9,	// the number of leaf nodes in the descendent of a cluster
		MODIFIER_HISTOGRAM			= 10,	// the modifier to do histogram for diagonal plot in scatterplot
		MODIFIER_ALLDIMROWINDEX		= 11,	// the modifier to sort the data based on each dimension
		MODIFIER_PCADERIVE			= 12, 	// new dimensions representing principal components derived from the PCA algorithm
	};

	// the assistant data type
	enum ASSISDATATYPE {
		ASSISDATA_BRUSH		= 0, // the brush storage to represent a brush range operator
		ASSISDATA_CARD		= 1, // the cardinality storage to represent the cardinality (# of bins) for the OkcData
	};

	// the glyph placement
	enum GLYPHPLACE_MODE {
		GLYPHPLACE_ORIGINAL		= 0, // place the glyphs in the original order in the file
		GLYPHPLACE_ORDERED		= 1, // sorting glyphs in terms of an arbitrary dimension
		GLYPHPLACE_DATA_DRIVEN	= 2, // Choosing two dimensions from original dimensions as X and Y for glyphs
		GLYPHPLACE_DERIVED		= 3, // Choosing two dimensions from PCA dimensions or original dimension
		//as X and Y for glyphs
	};

	// the star shape
	enum GLYPHSHAPE {
		GLYPHSHAPE_PROFILE		= 0, // profile shape
		GLYPHSHAPE_STAR			= 1, // star shape
	};

	// the scatterplot point shape
	enum SCATTERPLOTSHAPE {
		SCATTERPLOTSHAPE_SQUARE		= 0, // square shape
		SCATTERPLOTSHAPE_CIRCLE		= 1, // circle shape
	};

	// what is shown for the diagonal plot in scatterplot view
	enum DIAGPLOT_MODE {
		DIAGPLOT_ORIGINAL = 0, // show original
		DIAGPLOT_HISTOGRAM = 1, // show histogram
		DIAGPLOT_ONE_DIM = 2, // show one dimension
		DIAGPLOT_TWO_DIM_IMPLICIT = 3, // show two dimension (implicit position)
		DIAGPLOT_TWO_DIM_DATA_DRIVEN = 4,  // show two dimension, the position of x and y dimension is selected
		DIAGPLOT_TWO_DIM_DERIVED = 5,  // show two dimension, the position of x and y are from PCA dimensions or original dimension

	};

	// representing original and derived dimensions,
	// it is used in glyphs and pixel-oriented displays
	// to denote a dimension based on which we sort records.
	enum OKCDIM {
		OKCDIM_ORIGINAL			= 0, // the original dimension in the input dataset
		OKCDIM_PCADERIVE		= 1, // the principal components computed from PCA
	};

	// the sort order
	enum SORTORDER {
		SORTORDER_INCREASE		= 0,
		SORTORDER_DECREASE		= 1,
	};

	enum CHECK_RENDERABLE_RESULT {
		FAIL_RENDERABLE	= 0,
		PASS_RENDERABLE	= 1,
		WARN_RENDERABLE	= 2
	};

	// The default vis type for a new vis.  When we open the first file under unix,
	// we ignore this value and set to glyph by default, because the QGLWidget sometimes
	// cannot work correctly with text rendering
	const int DEFAULT_VISTYPE = 0;


	// the math constant pi
	const double PI = 4.0*atan(1.0);

	const int SUBWIN_WIDTH = 400;
	const int SUBWIN_HEIGHT = 400;
}
#endif /*XMDVTOOL_H_*/
