SUBDIRS = SIAddresses

if ENABLE_SPECS

# Main DocBook/XML files (DOCTYPE book)
docbook = x11protocol.xml

# Included chapters, appendix, images
chapters = 		\
	encoding.xml	\
	glossary.xml	\
	keysyms.xml	\
	sect1-9.xml

# The location where the DocBook/XML files and their generated formats are installed
shelfdir = $(docdir)

# Generate DocBook/XML output formats with or without stylesheets
include $(top_srcdir)/docbook.am

endif ENABLE_SPECS
