noinst_LTLIBRARIES = libxcms.la

AM_CPPFLAGS= \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/include/X11 \
	-I$(top_builddir)/include \
	-I$(top_builddir)/include/X11 \
	-I$(top_srcdir)/src/xcms \
	-I$(top_srcdir)/src/xkb \
	-I$(top_srcdir)/src/xlibi18n \
	-I$(top_srcdir)/src \
	-D_BSD_SOURCE \
        -DXCMSDIR=\"$(X11dir)\"

AM_CFLAGS= \
	$(X11_CFLAGS) \
	$(BIGFONT_CFLAGS) \
	$(XMALLOC_ZERO_CFLAGS) \
	$(CWARNFLAGS)

libxcms_la_SOURCES = \
		  AddDIC.c \
                  AddSF.c \
                  CCC.c \
                  CvColW.c \
                  CvCols.c \
                  Cv.h \
                  HVC.c \
                  HVCGcC.c \
                  HVCGcV.c \
                  HVCGcVC.c \
                  HVCMnV.c \
                  HVCMxC.c \
                  HVCMxV.c \
                  HVCMxVC.c \
                  HVCMxVs.c \
                  HVCWpAj.c \
                  IdOfPr.c \
                  LRGB.c \
                  Lab.c \
                  LabGcC.c \
                  LabGcL.c \
                  LabGcLC.c \
                  LabMnL.c \
                  LabMxC.c \
                  LabMxL.c \
                  LabMxLC.c \
                  LabWpAj.c \
                  Luv.c \
                  LuvGcC.c \
                  LuvGcL.c \
                  LuvGcLC.c \
                  LuvMnL.c \
                  LuvMxC.c \
                  LuvMxL.c \
                  LuvMxLC.c \
                  LuvWpAj.c \
                  OfCCC.c \
                  PrOfId.c \
                  QBlack.c \
                  QBlue.c \
                  QGreen.c \
                  QRed.c \
                  QWhite.c \
                  QuCol.c \
                  QuCols.c \
                  SetCCC.c \
                  SetGetCols.c \
                  StCol.c \
                  StCols.c \
                  UNDEFINED.c \
                  XRGB.c \
                  XYZ.c \
                  cmsAllCol.c \
                  cmsAllNCol.c \
                  cmsCmap.c \
                  cmsColNm.c \
                  cmsGlobls.c \
                  cmsInt.c \
                  cmsLkCol.c \
                  cmsMath.c \
                  cmsProp.c \
                  cmsTrig.c \
                  uvY.c \
                  xyY.c \
                  Xcmsint.h

X11dir = $(X11_DATADIR)
dist_X11_DATA = Xcms.txt


if LINT
# Check source code with tools like lint & sparse

ALL_LINT_FLAGS=$(LINT_FLAGS) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) \
		$(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS)

lint:
	$(LINT) $(ALL_LINT_FLAGS) $(libxcms_la_SOURCES) -lm

endif LINT
