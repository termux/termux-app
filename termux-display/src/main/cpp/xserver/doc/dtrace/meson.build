
if build_docs
    basename = 'Xserver-DTrace'

    input_xml = basename + '.xml'

    custom_target(
        basename + '.html',
        output: basename + '.html',
        input: [input_xml],
        command: [xmlto] + docs_xmlto_search_flags + [
            '-x',  join_paths(doc_stylesheet_srcdir, 'xorg-xhtml.xsl'),
            '--stringparam', 'target.database.document=' + join_paths(doc_sgml_path, 'X11/dbs/masterdb.html.xml'),
            '--stringparam', 'current.docid=' + basename,
            '-o', meson.current_build_dir(),
            'xhtml-nochunks', '@INPUT0@'],
        build_by_default: true,
        install: true,
        install_dir: join_paths(get_option('datadir'), 'doc/xorg-server'),
    )

    if build_docs_pdf
        foreach format : ['ps', 'pdf']
            output_fn = basename + '.' + format
            custom_target(
                output_fn,
                output: output_fn,
                input: [input_xml],
                command: [xmlto] + docs_xmlto_search_flags + [
                    '-x',  join_paths(doc_stylesheet_srcdir, 'xorg-fo.xsl'),
                    '--stringparam', 'img.src.path=' + meson.current_build_dir(),
                    '--stringparam', 'target.database.document=' + join_paths(doc_sgml_path, 'X11/dbs/masterdb.pdf.xml'),
                    '--stringparam', 'current.docid=' + basename,
                    '-o', meson.current_build_dir(),
                    '--with-fop', format, '@INPUT0@'],
                build_by_default: true,
                install: true,
                install_dir: join_paths(get_option('datadir'), 'doc/xorg-server'),
            )
        endforeach
    endif

    foreach format_data : [['html', 'xorg-xhtml.xsl'], ['pdf', 'xorg-fo.xsl']]
        format = format_data[0]
        stylesheet = format_data[1]
        output_fn = basename + '.' + format + '.db'
        custom_target(
            output_fn,
            output: output_fn,
            input: [input_xml],
            command: [xsltproc] + docs_xslt_search_flags + [
                '--stringparam', 'targets.filename', output_fn,
                '--stringparam', 'collect.xref.targets', 'only',
                '--stringparam', 'olink.base.uri', basename + '.' + format,
                '--nonet',
                '--output', join_paths(meson.current_build_dir(), output_fn),
                '--xinclude', join_paths(doc_stylesheet_srcdir, stylesheet),
                '@INPUT0@'],
            build_by_default: true,
            install: true,
            install_dir: join_paths(get_option('datadir'), 'doc/xorg-server'),
        )
    endforeach
endif
