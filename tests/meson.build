test_cpp_args = []
test_deps = [
	telesto_dep,
	subproject('frigg').get_variable('frigg_dep'),
	declare_dependency(include_directories: include_directories('../subprojects/libasync/include'))
]

if meson.get_compiler('cpp').has_argument('-fcoroutines')
	test_cpp_args += '-fcoroutines'
elif meson.get_compiler('cpp').has_argument('-fcoroutines-ts')
	test_cpp_args = ['-fcoroutines-ts', '-DLIBASYNC_FORCE_USE_EXPERIMENTAL']
	test_deps += subproject('cxxshim').get_variable('clang_coroutine_dep')
else
	error('unsupported compiler')
endif

unix_socket = executable('unix-socket',
	'unix-socket.cpp',
	cpp_args: test_cpp_args,
	dependencies: test_deps)

test('unix socket test', unix_socket)
