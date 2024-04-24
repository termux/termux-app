
if ENABLE_SPECS
if HAVE_ASCIIDOC

doc_DATA = XI2proto.html XIproto.html
dist_doc_DATA = XI2proto.txt XIproto.txt

%.html: %.txt
	$(AM_V_GEN)TZ=UTC $(ASCIIDOC) -o $@ $<

CLEANFILES = $(doc_DATA)

endif
endif
