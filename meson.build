project('telesto', 'cpp',
	license: 'Zlib',
	default_options: ['cpp_std=c++20'])

includes = include_directories('include')
telesto_dep = declare_dependency(include_directories: includes)

if get_option('build_tests')
	subdir('tests')
endif
