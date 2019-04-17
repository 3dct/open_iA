#ifndef MACROS_H_
#define MACROS_H_

// include <float.h> to use DBL_EPSILON
#include <float.h>

#ifndef MIN
# define MIN(X,Y) ((X)<(Y)?(X):(Y))
#endif
#ifndef MAX
# define MAX(X,Y) ((X)>(Y)?(X):(Y))
#endif

#define BITCLEAR(var,bits) {var&=(0xffff^(bits));}
#define BITSET(var,bits) {var|=(bits);}
#define BITTEST(var,bits) (((var)&(bits))==bits)

#define CLAMP(A,L,H) (((A)<(L))?(L):( ((A)>(H))?(H):A ))

// Anil - [2/9/2004]
#define SAFE_DELETE(x)		{if(x) delete (x); x= 0;}
#define SAFE_DELETE_ARR(x)	{if(x) delete [] (x); x= 0;}

// The margin width of canvas
#define CANVAS_BORD 0.1

// The distance between the color bar and bottom
// in the structure-based brush dialog
#define SBB_BARHIGHT 0.05

// The contour resolution
// in the structure-based brush dialog
#define SBB_CONTOUR_RES 25


// XMDV_DBL_EPSILON is used to judge whether some double values satisfy
// a condition including ">=" or "<=".  For example, a>=b should be
// written in a>b-XMDV_DBL_EPSILON because it is not safe to judge
// whether two double values is equal to each other
#define XMDV_DBL_EPSILON (DBL_EPSILON*1000.0)

////////////////////////////////////////////////////////////////////
// Some constants for the pipeline with type PLTYPE_FLATNORMAL
////////////////////////////////////////////////////////////////////
// In the main transformation
// The number of operators
#define PL_FN_OP_CNT 6
// PCADeriveOperator and sortRowsOperator should be put together,
// because we use the input of these two operators to generate the dimension
// name list in the glyph customization dialog.  If these two operators are
// put at different sides of dimension on/off/reordering operator, they will have different
// dimension name list, which will cause troubles in glyph customization dialog.
// The order of PCADeriveOperator
#define PL_FN_OP_PCADERIVE 0
// The order of SortOperator
#define PL_FN_OP_SORT 1
// The order of DimOOOOperator
#define PL_FN_OP_DIMOOO 2
// The order of BrushOperator
#define PL_FN_OP_BRUSH 3
// The order of HistogramOperator
#define PL_FN_OP_HISTOGRAM 4
// The order of RowIndexOnAllDimOperator
#define PL_FN_OP_SORTROWALLDIM 5
//
// The number of assistant transformations
#define PL_FN_ASSIS_CNT 2
//
// In the assistant transformation for brush storage
// The order of this assistant transformation
#define PL_FN_ASSIS_BRUSH_INDEX 0
// The number of operators
#define PL_FN_ASSIS_BRUSH_OP_CNT 1
// The order of operator
#define PL_FN_ASSIS_BRUSH_OP_DIMOOO 0
//
// In the assistant transformation for cardinality storage
// The order of this assistant transformation
#define PL_FN_ASSIS_CARD_INDEX 1
// The number of operators
#define PL_FN_ASSIS_CARD_OP_CNT 1
// The order of operator
#define PL_FN_ASSIS_CARD_OP_DIMOOO 0


////////////////////////////////////////////////////////////////////
// Some constants for the pipeline with type PLTYPE_SBB
////////////////////////////////////////////////////////////////////
// The number of operators
#define PL_SBB_OP_CNT 5
// The order of ClusterOperator
#define PL_SBB_OP_CLUSTER 0
// The order of SBBOperator
#define PL_SBB_OP_SBB 1
// The order of PCADeriveOperator
#define PL_SBB_OP_PCADERIVE 2
// The order of SortOperator
#define PL_SBB_OP_SORT 3
// The order of DimOOOOperator
#define PL_SBB_OP_DIMOOO 4
//
// The number of assistant transformations
#define PL_SBB_ASSIS_CNT 1
//
// In the assistant transformation for cardinality storage
// The order of this assistant transformation
#define PL_SBB_ASSIS_CARD_INDEX 0
// The number of operators
#define PL_SBB_ASSIS_CARD_OP_CNT 1
// The order of operator
#define PL_SBB_ASSIS_CARD_OP_DIMOOO 0


////////////////////////////////////////////////////////////////////
// Some constants for the pipeline with type PLTYPE_DIMR
////////////////////////////////////////////////////////////////////
// The number of operators
#define PL_DIMR_OP_CNT 6
// The order of DimTreeOperator
#define PL_DIMR_OP_DIMTREE 0
// The order of DimReductViewOperator
#define PL_DIMR_OP_DRVIEW 1
// The order of DimOOOOperator
#define PL_DIMR_OP_DIMOOO 2
// The order of BrushOperator
#define PL_DIMR_OP_BRUSH 3
// The order of HistogramOperator
#define PL_DIMR_OP_HISTOGRAM 4
// The order of RowIndexOnAllDimOperator
#define PL_DIMR_OP_SORTROWALLDIM 5

//
// The number of assistant transformations
#define PL_DIMR_ASSIS_CNT 2
// In the assistant transformation for brush storage
// The order of this assistant transformation
#define PL_DIMR_ASSIS_BRUSH_INDEX 0
// The number of operators
#define PL_DIMR_ASSIS_BRUSH_OP_CNT 1
// The order of operator
#define PL_DIMR_ASSIS_BRUSH_OP_DIMOOO 0
//
// In the assistant transformation for cardinality storage
// The order of this assistant transformation
#define PL_DIMR_ASSIS_CARD_INDEX 1
// The number of operators
#define PL_DIMR_ASSIS_CARD_OP_CNT 1
// The order of operator
#define PL_DIMR_ASSIS_CARD_OP_DIMOOO 0


// The maximum length for a file name with path
// This is used to define char array to store file name
#define FILENAME_MAXLEN 1024

// The prefix of cg file
#define CGFILE_MAGIC_CODE 0xcecfcecf

#define IS_CONTOUR0  1<<0
#define IS_CONTOUR1  1<<1
#define IS_MAGNIFIED 1<<2
#define IS_BRUSHED   1<<3
#define IS_BRUSHED_ANY   1<<4

// If settings in HierDisplayInformation are changed,
// the data member m_changeFlag in SBBPipeline will be
// marked with the addition of following flags.
#define DIRTY_CLUSTER_RADIUS 1<<0
#define DIRTY_BRUSHED_RADIUS 1<<1
#define DIRTY_HANDLE_POSITION 1<<2


#define SBBSEMANTIC_ALL 0
#define SBBSEMANTIC_ANY 1

/*Once the number of children of a cluster lower than
TREEMAP_LIMIT*number of data points of the data set,
do not draw the details of this cluster on tree map.*/
#define TREEMAP_LIMIT 0.01

// The bit operations
//
// Clear those bits indicated by the argument "bits"
// For example, if "bits=0xFFFF", all of lower 16 bites
// will be set to 0 and other bits will keep unchanged
#define BITCLEAR(var,bits) {var&=(0xffff^(bits));}
// Set those bits indicated by the argument "bits"
#define BITSET(var,bits) {var|=(bits);}
// Test whether some bits are ones
#define BITTEST(var,bits) (((var)&(bits))==bits)

// All of icons
//
// The icon for parallel coordinates
#define ICON_PA QIcon(QString::fromUtf8(":/new/prefix2/icon/imgpa.png"))
// The icon for scatterplot matrices
#define ICON_SC QIcon(QString::fromUtf8(":/new/prefix2/icon/imgsc.png"))
// The icon for glyphs
#define ICON_ST	QIcon(QString::fromUtf8(":/new/prefix2/icon/imgst.png"))
// The icon for dimension stacking
#define ICON_DI QIcon(QString::fromUtf8(":/new/prefix2/icon/imgdi.png"))
// The icon for pixel-oriented display
#define ICON_PI QIcon(QString::fromUtf8(":/new/prefix2/icon/imgpi.png"))
// The icon for hierarchical parallel coordinates
#define ICON_HPA QIcon(QString::fromUtf8(":/new/prefix2/icon/imghpa.png"))
// The icon for hierarchical scatterplot matrices
#define ICON_HSC QIcon(QString::fromUtf8(":/new/prefix2/icon/imghsc.png"))
// The icon for hierarchical glyphs
#define ICON_HST QIcon(QString::fromUtf8(":/new/prefix2/icon/imghst.png"))
// The icon for hierarchical dimension stacking
#define ICON_HDI QIcon(QString::fromUtf8(":/new/prefix2/icon/imghdi.png"))
// The icon for hierarchical pixel-oriented display
#define ICON_HPI QIcon(QString::fromUtf8(":/new/prefix2/icon/imgpi.png"))
// The icon for opening file
#define ICON_OPEN QIcon(QString::fromUtf8(":/new/prefix1/icon/imgopen.png"))
// The icon for structure-based brush
#define ICON_SBB QIcon(QString::fromUtf8(":/new/prefix1/icon/imgsb.png"))
// The icon for zooming in
#define ICON_ZOOMIN QIcon(QString::fromUtf8(":/new/prefix3/icon/zoomin.png"))
// The icon for zooming out
#define ICON_ZOOMOUT QIcon(QString::fromUtf8(":/new/prefix3/icon/zoomout.png"))
// The icon for reset size
#define ICON_RESETSIZE QIcon(QString::fromUtf8(":/new/prefix3/icon/resize.png"))
// The icon for dataset
#define ICON_DB QIcon(QString::fromUtf8(":/new/prefix4/icon/dataset.png"))
// The icon for pipeline
#define ICON_PL QIcon(QString::fromUtf8(":/new/prefix4/icon/gear.png"))

#endif /*MACROS_H_*/

