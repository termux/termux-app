SUBDIRS = dtrace

if ENABLE_DEVEL_DOCS
if HAVE_XMLTO

# Main DocBook/XML files (DOCTYPE book)
docbook = Xserver-spec.xml Xinput.xml

# Generate DocBook/XML output formats with or without stylesheets
include $(top_srcdir)/devbook.am

endif HAVE_XMLTO
endif ENABLE_DEVEL_DOCS

EXTRA_DIST = smartsched filter-xmlto.sh
