test_cpp_args = []
test_deps = [
	telesto_dep,
	subproject('frigg').get_variable('frigg_dep'),
	declare_dependency(include_directories: include_directories('../subprojects/libasync/include')),
	dependency('libuv')
]

if meson.get_compiler('cpp').has_argument('-fcoroutines')
	test_cpp_args += '-fcoroutines'
elif meson.get_compiler('cpp').has_argument('-fcoroutines-ts')
	test_cpp_args = ['-fcoroutines-ts', '-DLIBASYNC_FORCE_USE_EXPERIMENTAL']
	test_deps += subproject('cxxshim').get_variable('clang_coroutine_dep')
else
	error('unsupported compiler')
endif

tcp = executable('tcp',
	'tcp.cpp',
	cpp_args: test_cpp_args,
	dependencies: test_deps)

test('tcp test', tcp)
