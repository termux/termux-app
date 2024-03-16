include $(top_srcdir)/manpages.am
appman_PRE = Xorg.man
fileman_PRE = xorg.conf.man xorg.conf.d.man

if SUID_WRAPPER
appman_PRE += Xorg.wrap.man
fileman_PRE += Xwrapper.config.man
else
EXTRA_DIST += Xorg.wrap.man Xwrapper.config.man
endif
