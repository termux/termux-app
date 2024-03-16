
appmandir = $(APP_MAN_DIR)
appman_PRE = xkbcomp.man
appman_DATA = $(appman_PRE:man=$(APP_MAN_SUFFIX))

EXTRA_DIST = $(appman_PRE)
CLEANFILES = $(appman_DATA)
SUFFIXES = .$(APP_MAN_SUFFIX) .man

# String replacements in MAN_SUBSTS now come from xorg-macros.m4 via configure
MAN_SUBSTS += -e 's|__xkbconfigroot__|$(XKBCONFIGROOT)|'

.man.$(APP_MAN_SUFFIX):
	$(AM_V_GEN)$(SED) $(MAN_SUBSTS) < $< > $@
