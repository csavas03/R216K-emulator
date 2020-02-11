project('r2emu', 'c')

c_compiler = meson.get_compiler('c')
executable('main',
	sources: 'main.c',
	dependencies: [
		c_compiler.find_library('m', required: false),
		dependency('gtk+-3.0')
	],
	link_args: ['-export-dynamic']
)
configure_file(input: 'glade.glade', output: 'glade.glade', copy: true)
