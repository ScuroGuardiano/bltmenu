project(
  'bltmenu',
  'cpp',
  version: '0.1',
  default_options: ['warning_level=3', 'cpp_std=c++14'],
)

XINERAMA = true

compiler = meson.get_compiler('cpp')
deps = [dependency('x11')]
src = ['bltmenu.cpp', 'src/X11.cpp']

cpp_args = []

if XINERAMA
  xinerama_dep = compiler.find_library('Xinerama')
  deps += xinerama_dep
  cpp_args += '-DXINERAMA'
endif

executable(
  'bltmenu',
  sources: src,
  install: true,
  cpp_args: cpp_args,
  dependencies: deps,
)
