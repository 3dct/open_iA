from copy import copy
from copy import copy

#======================================================
# Python auxiliary code
#======================================================
def inverse(val):
    "Returns an inverse value of a color component"
    return 255-val

class Rgba():
    """Class representing color in rgba.
    Has some auxiliary functions and basic overriden operators"""
    def __init__(self, a_rgba) :
        def crop(e) :
            "Perform [0, 255] boundary check"
            if e>255 :
                e = 255
            elif e<0:
                e = 0
            return e        
        self.rgba = list(map(crop, a_rgba))
    def invCol(self):
        "Inverse the color (not opacity), modify self"
        res = [255 - e for e in self.rgba[:3]] + self.rgba[3:]
        self.rgba = res
    def inverse(self):
        "Inverse the color (not opacity), ruturn new Rgba"
        return Rgba([255 - e for e in self.rgba[:3]] + self.rgba[3:])
    def __add__(self, other) :
        if(type(other) == int) :
            return Rgba( [i + other for i in self.rgba[:3]] + self.rgba[3:] )
        else:
            return Rgba( [i + j for i,j in zip(self.rgba[:3],other.rgba[:3])].append(255) )
    def __sub__(self, other) :
        if(type(other) == int) :
            return Rgba( [i - other for i in self.rgba[:3]] + self.rgba[3:] )
        else:
            return Rgba( [i - j for i,j in zip(self.rgba,other.rgba)] )
    def __mul__(self, other) :
        if(type(other) == int or type(other) == float) :
            return Rgba( [int(i * other) for i in self.rgba] )
        else:
            return Rgba([i*j for i,j in zip(self.rgba,other.rgba)])
    def __str__(self) :
        res = "rgba(" + str(self.rgba[0])
        for e in self.rgba[1:] :
            res = res + ", " + str(e)
        res+= ")"
        return res

def rgb(r, g, b) :
    """Create Rgba instance with a==255, based on r,g,b values"""
    return Rgba( [r, g, b, 255] )

def rgba(r, g, b, a = 255) :
    """Create Rgba instance based on r,g,b,a values"""
    return Rgba( [r, g, b, a] )

def gray(gr) :
    """Create Rgba instance for gray color with intensity == gr"""
    return Rgba( [gr, gr, gr, 255] )

def interpolate(col1, col2, t) :
    """Interpolate between col1 and col2 based on t[0,1].
    t == 0 corresponds to col1"""
    l = [int(i + (j-i)*t) for i,j in zip(col1.rgba[:3], col2.rgba[:3])]
    l.append(255)
    return Rgba(l)

def gradientWithBreak(pt1, pt2, t, col1, col2, step = 0) :
    """Generates a gradient where color brakes at t=[0,1].
    The break magnitude is step. Returns a proper QSS gradient description."""
    intCol = interpolate(col1, col2, t)
    res_str  = "background: qlineargradient(spread:pad, \n\t\t";
    res_str += "x1:" + str( pt1[0] ) + ", y1:" + str( pt1[1] ) + ", \n\t\t"
    res_str += "x2:" + str( pt2[0] ) + ", y2:" + str( pt2[1] ) + ", \n\t\t"
    res_str += "stop:0 " + str(col1) + ", \n\t\t"
    res_str += "stop:" + str(t - 0.001) + " " + str(intCol + step) + ", \n\t\t"
    res_str += "stop:" + str(t) + " " + str(intCol - step) + ", \n\t\t"
    res_str += "stop:1 " + str(col2) + ");"
    return res_str

#======================================================
# Dynamic QSS parameters
#======================================================
#Constants:
golden = 0.618034		#golden ratio
gradStep = 0			#amount of color jump on gradients with breaks
circGradientPos1 = 0.176136	#position 1 on circular gradient
circGradientPos2 = 0.664773	#position 1 on circular gradient

#Colors:
black = rgb(0,0,0)
#Indices                    0   1   2   3   4   5   6   7   8    9    10   11   12   13   14   15
Grays = [gray(e) for e in [20, 30, 40, 50, 60, 70, 80, 90, 100, 120, 140, 150, 180, 200, 230, 255] ]

brightGray       = copy(Grays[9])
evenBrighterGray = copy(Grays[11])
veryBrightGray   = copy(Grays[13])
brightestGray    = copy(Grays[14])

borderColor     = copy(Grays[8])
backgroundGray  = copy(Grays[2])
mdiBackground   = copy(Grays[2])
listItemBg      = [ copy(black), copy(Grays[0]) ]

tabBorderColor = copy(Grays[6])
menuBackground = copy(Grays[7])
textColor      = copy(Grays[14])      #basic text color
menuTextColor  = copy(Grays[10])      #menu text color
orangeHighlightColor = rgb(255,168,0)#text color of hovered menu item
disabledMenuItemCol = copy(Grays[5])
sbText = rgb(255, 167, 29); #QStatusBar
sbBorder = darkkhaki = rgb(187, 187, 102)
listBg = [copy(Grays[14]), copy(Grays[15])]
listSelected = copy(Grays[12])

#Gradient colors:
toolbarBg = [ copy(Grays[1]), copy(Grays[0]) ]
blueGradient = [ rgb(150, 186, 230), rgb(70, 122, 201) ]
redGradient = [ e.inverse() for e in blueGradient ]
pbGradient = [ copy(Grays[8]), copy(Grays[4]) ]    #QPushButton gradient

iwGradient = [ copy(Grays[3]), copy(Grays[0]) ]#Basic interactive widget background
hoverDelta = 6 #Basic interactive widget background hovered
iwHoveredGradient = [ e + hoverDelta for e in iwGradient ]	
grooveGradient = [ copy(Grays[1]), copy(Grays[4]) ]#Slider groove
handleColor = [ copy(Grays[12]), copy(Grays[9]) ] #Slider handle
splitterGradient = [ copy(Grays[6]), copy(Grays[3]) ]#QSplitter

tabGrad = [ copy(Grays[1]), copy(Grays[0]) ]
seltabGrad = [ e + 50 for e in tabGrad ] 

#Borders
brdTB = 20 #difference between background and borders - top/bottom
brdSide = 10 #sides
toolbarBrd = [ toolbarBg[0] + brdTB, toolbarBg[1] - brdTB ]

pbBrd = [ pbGradient[0] + brdTB, pbGradient[1] - brdTB ]  #QPushButton normal
pbSideBrd = pbGradient[1] - brdSide

blueBrd = [ blueGradient[0] + brdTB, blueGradient[1] - brdTB ] #QPushButton hovered
blueSideBrd = blueGradient[1]- brdSide

listHoverJump = 10


baseColor = rgb(40, 40, 40)
secondColor = rgb(80, 80, 80)
colorPallete = [ rgb(102, 102, 102), rgb(80, 80, 80), rgb(60, 60, 60), rgb(45, 45, 45), rgb(30, 30, 30), rgb(15, 15, 15) ]

#======================================================
# Conditional code that generates bright GUI skin 
#======================================================
def darkToBright() :
    #Mirror:
    lst =\
        [\
        mdiBackground,\
        black,\
        borderColor,\
        brightGray,\
        evenBrighterGray,\
        veryBrightGray,\
        backgroundGray,\
        tabBorderColor,\
        textColor,\
        menuTextColor,\
        pbSideBrd,\
        sbText,
        ] + \
	pbGradient + \
	pbBrd + \
        iwGradient + \
        iwHoveredGradient + \
        grooveGradient + \
        splitterGradient + \
        tabGrad + \
        listBg + \
        toolbarBg + toolbarBrd + \
        listItemBg
    lst = [e.invCol() for e in lst]
	
    for e in colorPallete:
	    e.invCol()
		
    #Swap:
    for e in [  pbGradient, iwGradient, grooveGradient, splitterGradient,\
                tabGrad, iwHoveredGradient, pbBrd, toolbarBg,\
                toolbarBrd, listItemBg] :
        e[0], e[1] = e[1], e[0]
    #Adjust:
    tabAdd = 80
    for e in listBg : e -= tabAdd
    
    seltabGrad[0] = tabGrad[0] + 50
    seltabGrad[1] = tabGrad[1] + 50

    blueAdd = 20
    for e in blueBrd : e += blueAdd
    for e in blueGradient : e += blueAdd
    global blueSideBrd
    blueSideBrd += blueAdd
    
    
    
#======================================================
# QSS enhanced with Python insertions 
#======================================================
		
#Enhanved QSS goes here. Use: $<your Python code>$ to embed Python code.
qss_str = """
/****************************************************************************
General
****************************************************************************/

	QWidget
	{
		background-color: $colorPallete[3]$;
	}
			
	QWidgetToolbar
	{
		/*$gradientWithBreak((0,0), (0,1), golden, toolbarBg[0], toolbarBg[1], 0)$*/
		color: $textColor$;
		border-top: 1px solid 		$toolbarBrd[0]$;
		border-bottom: 1px solid 	$toolbarBrd[1]$;
	}
	
	QLineEdit:hover,
	QSpinBox:hover,
	QDoubleSpinBox:hover,
	QComboBox:hover,
	.QDateTimeEdit:hover,
	QTabBar::tab:hover
	{
		/*$gradientWithBreak((0,0), (0,1), golden, iwHoveredGradient[0], iwHoveredGradient[1], gradStep)$*/
		/*background-color: $colorPallete[2]$;*/
		color: $orangeHighlightColor$;
	}

/****************************************************************************
QToolBar
QToolButton
****************************************************************************/
	QToolBar
	{
		color: $textColor$;
		background: $colorPallete[1]$
		/*spacing: 4px;*/ /* spacing between items in the tool bar */
	}

	QToolButton
	{
		background: transparent;
		color: $textColor$;
		/*border: 0px;*/
	}
	
	QToolButton:hover
	{
		background: $colorPallete[0]$;
	}
	
	QToolButton:checked
	{
		background: $colorPallete[3]$;
	}
	
	QToolButton#qt_toolbar_ext_button
	{
		/*gradientWithBreak((0,0), (0,1), golden, pbGradient[0], pbGradient[1], gradStep)$*/
	}
	
/****************************************************************************
QStatusBar
****************************************************************************/
	QStatusBar
	{
		color: $textColor$;
		background: $colorPallete[2]$
		/*spacing: 4px;*/ /* spacing between items in the tool bar */
	}

/****************************************************************************
QMenuBar
****************************************************************************/

	QMenuBar
	{
		background: $colorPallete[1]$;
	}

	QMenuBar::item
	{
		spacing: 3px; /* spacing between menu bar items */
		padding: 6px 12px;
		background: transparent;
		color: $textColor$;
	}

	QMenuBar::item:selected
	{
		/* when selected using mouse or keyboard */
		background: $colorPallete[3]$;
		color: $orangeHighlightColor$;
	}

	QMenuBar::item:pressed
	{
		background: $colorPallete[2]$;
	}

/****************************************************************************
QMenu
****************************************************************************/

	QMenu::item
	{
		/* sets background of menu item. set this to something non-transparent if you want menu color and menu item color to be different */
		/* background-color: transparent; */
		color: $textColor$;
	}

	QMenu::item:selected
	{
		/* when user selects item using mouse or keyboard */
		/* background: $menuBackground$; */
		color: $orangeHighlightColor$;
	}

	QMenu::separator
	{
		height: 2px;
		margin-left: 18px;
		margin-right: 18px;
		/*$gradientWithBreak((0,0), (0,1), golden, blueGradient[0], blueGradient[1], gradStep)$*/
		background-color: $colorPallete[2]$
	}

	QMenu::item:disabled
	{
		/* when user selects item using mouse or keyboard */
		/* background-color: $black$; */
		color: $menuTextColor$;
	}

/****************************************************************************
QPushButton
****************************************************************************/

	QPushButton
	{
		border-style: outset;
		border-top: 1px solid 		$colorPallete[3]$;
		border-bottom: 1px solid 	$colorPallete[3]$;
		border-left: 1px solid 		$colorPallete[3]$;
		border-right: 1px solid		$colorPallete[3]$;
		padding: 6px;
		color: $textColor$;
		background-color: $colorPallete[2]$;
	}

	QPushButton:hover
	{
		border-top: 1px solid 		$colorPallete[3]$;
		border-bottom: 1px solid 	$colorPallete[3]$;
		border-left: 1px solid 		$colorPallete[3]$;
		border-right: 1px solid		$colorPallete[3]$;
		color: $orangeHighlightColor$
	}

	QPushButton:pressed
	{
		border-top: 1px solid 		$colorPallete[3]$;
		border-bottom: 1px solid 	$colorPallete[3]$;
		border-left: 1px solid 		$colorPallete[3]$;
		border-right: 1px solid		$colorPallete[3]$;
		background-color: $colorPallete[3]$;
		border-style: inset;
	}

	QPushButton:flat
	{
		border: none; /* no border for a flat push button */
		/* background: transparent; */
	}

/****************************************************************************
QTabWidget
QTabBar
****************************************************************************/

	QTabWidget
	{
	}
	QTabWidget::pane
	{
		/* The tab widget frame */
		/*position: absolute;*/
		top: -1px;
		border: 1px solid $tabBorderColor$;
		/*border-radius: 5px;*/
	}

	QTabWidget::tab-bar
	{
		alignment: center;
	}

	QTabBar::tab
	{
		/* Style the tab using the tab sub-control. Note thatit reads QTabBar _not_ QTabWidget */
		color: $textColor$;
		/*$gradientWithBreak((0,0), (0,1), golden, tabGrad[0], tabGrad[1], gradStep)$*/
		border: 1px solid $borderColor$;
		/*border-top-left-radius: 4px;
		border-top-right-radius: 10px;*/
		/*min-width:36ex;*/
		padding:1px 4px;
	}

	QTabBar::tab:selected, QTabBar::tab:selected:hover
	{
		border-top: 1px solid $brightGray$;
		border-left: 1px solid $brightGray$;
		border-right: 1px solid $brightGray$;
		/*$gradientWithBreak((0,0), (0,1), golden, seltabGrad[0], seltabGrad[1], gradStep)$*/
		background: $colorPallete[2]$;
	}
	QTabBar::tab:!selected
	{
		margin-top: 3px;
	}

	QTabBar::scroller
	{       /* the width of the scroll buttons */
		/* background: none; */
		width: 10px;
	}

	QTabBar QToolButton
	{	/* the scroll buttons are tool buttons */
		border-width: 1px;
		color: $orangeHighlightColor$;
		/* background: none; */
	}

/****************************************************************************
QLabel
****************************************************************************/
	QLabel
	{
		/* padding: 0 3px; */
		color: $textColor$;
		/* background: none; */
		/* font: normal; */
	}

/****************************************************************************
QLineEdit
****************************************************************************/
	QLineEdit
	{
		color: $textColor$;
		border: 1px solid $borderColor$;
	}

/****************************************************************************
QSpinBox
QDoubleSpinBox
****************************************************************************/
	QSpinBox, 
	QDoubleSpinBox
	{
		border: 1px solid $borderColor$;
		color: $textColor$;
	}

	QSpinBox::up-button, 
	QDoubleSpinBox::up-button
	{
		border-width: 0px;
	}

	QSpinBox::down-button, 
	QDoubleSpinBox::down-button
	{
		border-width: 0px;
	}

	QSpinBox::up-arrow, 
	QDoubleSpinBox::up-arrow
	{
		image: url(:/images/up_arrow.png);
	}

	QSpinBox::down-arrow, 
	QDoubleSpinBox::down-arrow
	{
		image: url(:/images/down_arrow.png);
	}

	QSpinBox::up-button:hover, 
	QDoubleSpinBox::up-button:hover,
	QSpinBox::down-button:hover, 
	QDoubleSpinBox::down-button:hover
	{
		/*$gradientWithBreak((0,0), (0,1), golden, pbGradient[0], pbGradient[1], gradStep)$*/
	}
	
	QSpinBox::up-button:pressed, 
	QDoubleSpinBox::up-button:pressed,
	QSpinBox::down-button:pressed, 
	QDoubleSpinBox::down-button:pressed
	{
		/*$gradientWithBreak((0,0), (0,1), 1 - golden, pbGradient[1], pbGradient[0], gradStep)$*/
	}

/****************************************************************************
QProgressBar
****************************************************************************/
	QProgressBar
	{
		border: 0px solid $borderColor$;
		color: $textColor$;
		border-bottom-right-radius: 0px;
		border-top-right-radius:  0px;
		border-bottom-left-radius:  0px;
		border-top-left-radius:  0px;
	}

	QProgressBar::chunk
	{
		/*$gradientWithBreak((0,0), (0,1), golden, blueGradient[0], blueGradient[1], gradStep)$*/
		color: $textColor$;
		/*border-radius: 9px;*/
		border-bottom-right-radius: 0px;
		border-top-right-radius: 0px;
		border-bottom-left-radius: 0px;
		border-top-left-radius: 0px;
	}


/****************************************************************************
QListView
****************************************************************************/
	QListView,
	QListWidget
	{
		background: $colorPallete[4]$;
		color: $listBg[0]$;
	}
	
	QListView::item,
	QListWidget::item
	{
		/* background: $listItemBg[0]$; */
		color: $listBg[0]$;
	}
	
	QListView::item:selected,
	QListWidget::item:selected
	{
		background: $colorPallete[5]$;
		color: $listBg[0]$;
		border: 0px;
	}
	
	QListView::item:alternate,
	QListWidget::item:alternate
	{
		background: $colorPallete[3]$;
		color: $listBg[0]$;
	}
	
	QListView::item:hover,
	QListWidget::item:hover
	{
		background: $colorPallete[3]$;
		color: $listBg[1]$;
		border: 1px solid $brightGray$;
	}
	
	QListView::item:alternate:hover,
	QListWidget::item:alternate:hover
	{
		/* background: $listItemBg[1] + listHoverJump$; */
		color: $listBg[1]$;
		border: 1px solid $brightGray$;
	}
	
/****************************************************************************
QTreeView
****************************************************************************/

	QTreeView::item
	{
		color: $listBg[0]$;
	}

/****************************************************************************
QGroupBox
QRadioButton
QCheckBox
****************************************************************************/
	QRadioButton, 
	QCheckBox
	{
		border-radius: 1px;
		border: 1px;
		/*background: none;*/
		color: $textColor$;
	}

/****************************************************************************
QGroupBox
****************************************************************************/


/****************************************************************************
QSlider
****************************************************************************/
	QSlider
	{
		/*background: none;*/
	}
	QSlider::groove:horizontal
	{
		border: 1px solid $borderColor$;
		height: 5px; /* the groove expands to the size of the slider by default. by giving it a height, it has a fixed size */
		$gradientWithBreak((0,0), (0,1), 1 - golden, grooveGradient[0], grooveGradient[1], gradStep)$
		margin: 3px 0;
		border-radius: 3px;
	}

	QSlider::handle:horizontal
	{
		background: qconicalgradient(cx:0.5, cy:0.5, angle:0, 
			stop:0 			$handleColor[0]$, 
			stop:$circGradientPos1$ $handleColor[1]$, 
			stop:0.5		$handleColor[0]$, 
			stop:$circGradientPos2$ $handleColor[1]$, 
			stop:1			$handleColor[0]$);
		border: 0px solid rgb(142,142,142);
		width: 13px;
		margin: -4px 0; /* handle is placed by default on the contents rect of the groove. Expand outside the groove */
		border-radius: 6px;
	}

	QSlider::groove:vertical
	{
		border: 1px solid $evenBrighterGray$;
		width: 6px; /* the groove expands to the size of the slider by default. by giving it a height, it has a fixed size */
		$gradientWithBreak((0,0), (1,0), 1 - golden, grooveGradient[0], grooveGradient[1], gradStep)$
		margin: 0 3px;
		border-radius: 3px;
	}

	QSlider::handle:vertical
	{
		background: qconicalgradient(cx:0.5, cy:0.5, angle:0, 
			stop:0 			$handleColor[0]$, 
			stop:$circGradientPos1$	$handleColor[1]$, 
			stop:0.5		$handleColor[0]$, 
			stop:$circGradientPos2$	$handleColor[1]$, 
			stop:1			$handleColor[0]$);
		border: 1px solid rgb(142,142,142);
		height: 12px;
		margin: 0 -4px; /* handle is placed by default on the contents rect of the groove. Expand outside the groove */
		border-radius: 6px;
	}

/****************************************************************************
QComboBox
****************************************************************************/
	QComboBox
	{
		padding-right: 20px; /* make room for the arrows */
		border: 1px solid $borderColor$;
		color: $textColor$;
	}
	QComboBox::drop-down, 
	.QDateTimeEdit::drop-down 
	{
		border: none;
	}
	
	QComboBox::down-arrow, 
	.QDateTimeEdit::down-arrow 
	{
		image: url(:/images/down_arrow.png);
	}
	
	QComboBox:on
	{
		/*$gradientWithBreak((0,0), (0,1), golden, iwHoveredGradient[0], iwHoveredGradient[1], gradStep)$*/
	}
	.QDateTimeEdit:pressed 
	{
		/*background: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1, stop:0 $grooveGradient[0]$, stop:1 $grooveGradient[1]$);*/ 
	}

/****************************************************************************
QStatusBar
****************************************************************************/
	QStatusBar
	{
		color: $sbText$;
		font: normal;
	}

	QStatusBar::item
	{
		spacing: 3px; /* spacing between menu bar items */
		padding: 6px 12px;
		border: 0;
		/*background: transparent;*/
		color: $sbText$;
	}

/****************************************************************************
QScrollBar
****************************************************************************/
	QScrollBar:horizontal
	{
		border: 0;
		background: $backgroundGray$;
		height: 15px;
		margin: 0 18px 0 18px;
	}

	QScrollBar::handle:horizontal
	{
		$gradientWithBreak((0,0), (0,1), golden, blueGradient[0], blueGradient[1], gradStep)$
		min-width:20px;
	}

	QScrollBar::add-line:horizontal
	{
		border: 0;
		background: $backgroundGray$;
		width: 15px;
		subcontrol-position: right;
		subcontrol-origin: margin;
	}

	QScrollBar::sub-line:horizontal
	{
		border: 0;
		background: $backgroundGray$;
		width: 15px;
		subcontrol-position: left;
		subcontrol-origin: margin;
	}

	QScrollBar::left-arrow:horizontal, QScrollBar::right-arrow:horizontal
	{
		border: 0;
		width: 5px;
		height: 5px;
		background: solid $borderColor$;
	}

	QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal
	{
		background: none;
	}

	QScrollBar:vertical
	{
		border: 0;
		background: $backgroundGray$;
		width: 15px;
		margin: 18px 0 18px 0;
	}

	QScrollBar::handle:vertical
	{
		$gradientWithBreak((0,0), (1,0), golden, blueGradient[0], blueGradient[1], gradStep)$
		min-height: 20px;
	}

	QScrollBar::add-line:vertical
	{
		border: 0;
		background: $backgroundGray$;
		height: 15px;
		subcontrol-position: bottom;
		subcontrol-origin: margin;
	}

	QScrollBar::sub-line:vertical
	{
		border: 0;
		background: $backgroundGray$;
		height: 15px;
		subcontrol-position: top;
		subcontrol-origin: margin;
	}

	QScrollBar::up-arrow:vertical, QScrollBar::down-arrow:vertical
	{
		border: 0;
		width: 5px;
		height: 5px;
		background: solid $borderColor$;
	}

	QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical
	{
		background: none;
	}

/****************************************************************************
QDockWidget
****************************************************************************/
	QDockWidget
	{
		background-color: $colorPallete[1]$;
		color: $textColor$; /* font-color */
		font: bold;
		/*border: 2px;*/
		titlebar-normal-icon: url(Resource/Icons/dock-normal.png);
	}

	QDockWidget::title
	{
		margin-top: 2px;
		margin-left: 2px;
		font-size: 14px;
		/*background: transparent;*/
	}

	QDockWidget::float-button
	{
	}

	QDockWidget::float-button:hover
	{
		image: url(Resource/Icons/dock-hover.png);
	}

	QDockWidget::float-button:pressed
	{
		image: url(Resource/Icons/dock-hover.png);
	}
	
/****************************************************************************
QSplitter
****************************************************************************/
	
	QSplitter::handle 
	{
		border-radius: 0px;
		width: 0px;
		height: 0px;
		margin: 2px;
	}

/****************************************************************************
QSizeGrip
****************************************************************************/
	QSizeGrip
        {
            /*background: transparent;*/
            image: url(:/images/sizeGrip.png);
            width: 16px;
            height: 16px;
        }

"""

#======================================================
# Parsing
#======================================================
def parse(string):
    "Parse QSS for python insertions, evaluate python code and insert it back"
    import re
    toEval = re.findall("\$(.+?)\$", string)
    evalRes = [str(eval(e)) for e in toEval]
    toEval = ["$" + e + "$" for e in toEval]
    new_str = string
    for i in range(len(toEval)):
        new_str = new_str.replace( toEval[i], evalRes[i] )
    return new_str

if __name__ == "__main__":
    a_file = open("dark_2.qss", "w")
    dark = parse(qss_str)
    print (dark), ("\n\n\n\n")
    a_file.write(dark)
    a_file.close()
    darkToBright()
    a_file = open("bright_2.qss", "w")
    bright = parse(qss_str)
    print (bright)
    a_file.write(bright)
    a_file.close()
