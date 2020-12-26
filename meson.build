project('telesto', 'cpp', default_options: ['cpp_std=c++2a'])

includes = include_directories('include')
telesto_dep = declare_dependency(include_directories: includes)

if get_option('build_tests')
	subdir('tests')
endif
