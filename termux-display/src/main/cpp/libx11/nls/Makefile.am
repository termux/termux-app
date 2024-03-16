
x11localedir = $(X11_LOCALEDATADIR)
CLEANFILES =
SUFFIXES =

EXTRA_DIST = locale.alias.pre compose.dir.pre locale.dir.pre \
	compose-check.pl

x11locale_DATA = locale.alias locale.dir compose.dir

CLEANFILES += \
	locale.alias locale.alias.l1 locale.alias.l2 \
	compose.dir compose.dir.l1 compose.dir.l2 \
	locale.dir locale.dir.l1 locale.dir.l2 \
	XLC_LOCALE

include $(top_srcdir)/cpprules.in

# Global data files

locale.alias: locale.alias.pre
	$(AM_V_GEN)$(RAWCPP) $(RAWCPPFLAGS) $(CPP_FILES_FLAGS) < $(srcdir)/locale.alias.pre | $(CPP_SED_MAGIC) > locale.alias.l1
	$(C_SED) -e '/^[^#][^	 ]*:/s/://' -e '/^[^#].*[	 ].*:/d' \
        < locale.alias.l1 > locale.alias.l2
	cat locale.alias.l2 locale.alias.l1 > locale.alias

compose.dir: compose.dir.pre
	$(AM_V_GEN)$(RAWCPP) $(RAWCPPFLAGS) $(CPP_FILES_FLAGS) < $(srcdir)/compose.dir.pre | $(CPP_SED_MAGIC) > compose.dir.l1
	$(C_SED) -e '/^[^#][^	 ]*:/s/://' -e '/^[^#].*[	 ].*:/d' \
        < compose.dir.l1 > compose.dir.l2
	cat compose.dir.l2 compose.dir.l1 > compose.dir

locale.dir: locale.dir.pre
	$(AM_V_GEN)$(RAWCPP) $(RAWCPPFLAGS) $(CPP_FILES_FLAGS) < $(srcdir)/locale.dir.pre | $(CPP_SED_MAGIC) > locale.dir.l1
	$(C_SED) -e '/^[^#][^	 ]*:/s/://' -e '/^[^#].*[	 ].*:/d' \
        < locale.dir.l1 > locale.dir.l2
	cat locale.dir.l2 locale.dir.l1 > locale.dir

if HAVE_PERL
LOG_COMPILER = $(PERL)
TESTS = compose-check.pl
endif HAVE_PERL


# Per-locale data files

nobase_dist_x11locale_DATA = $(locales:%=%/XI18N_OBJS)

nobase_x11locale_DATA = $(locales:%=%/XLC_LOCALE) $(locales:%=%/Compose)
EXTRA_DIST += $(nobase_x11locale_DATA:%=%.pre)
CLEANFILES += $(nobase_x11locale_DATA)

