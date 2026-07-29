public_inc = include_directories('../include')
public_headers = files('../include/libpkgcheck-exec/error.h','../include/libpkgcheck-exec/model.h','../include/libpkgcheck-exec/executor.h','../include/libpkgcheck-exec/libpkgcheck-exec.h')
public_deps = [libpkgcheck_dep, libpkgexec_dep]
libpkgcheck_exec = library('pkgcheck-exec', ['error.cpp','model.cpp','executor.cpp'], include_directories: public_inc, dependencies: public_deps + [libcrypto_dep], install: true, version: meson.project_version(), soversion: '0')
libpkgcheck_exec_dep = declare_dependency(link_with: libpkgcheck_exec, include_directories: public_inc, dependencies: public_deps + [libcrypto_dep])
meson.override_dependency('libpkgcheck-exec', libpkgcheck_exec_dep)
install_headers(public_headers, subdir: 'libpkgcheck-exec')
pkgconf.generate(libpkgcheck_exec, name: 'libpkgcheck-exec', filebase: 'libpkgcheck-exec', description: 'Native check execution adapter', subdirs: '.', requires: ['libpkgcheck >= 0.1.0','libpkgexec >= 1.3.0'], requires_private: ['libcrypto'])
