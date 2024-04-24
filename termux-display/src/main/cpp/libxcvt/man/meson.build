man_conf = configuration_data()
man_conf.set('appmansuffix', '1')
man_conf.set('filemansuffix', '5')
man_conf.set('vendorversion',
	'"libxcvt @0@" "X Version 11"'.format(meson.project_version()))

configure_file(
	input: 'cvt.man',
	output: 'cvt.1',
	install_dir: join_paths(man, 'man1'),
	configuration: man_conf
)
