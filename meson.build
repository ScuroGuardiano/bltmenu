project(
  'bltmenu',
  'cpp',
  version: '0.1',
  default_options: ['warning_level=3', 'cpp_std=c++17'],
)

XINERAMA = true

compiler = meson.get_compiler('cpp')

xft_dep = compiler.find_library('Xft')
fontconfig_dep = compiler.find_library('fontconfig')

deps = [
  dependency('x11'),
  xft_dep,
  fontconfig_dep
]

src = ['bltmenu.cpp', 'src/X11.cpp', 'src/drw.cpp', 'src/utf8.cpp']

cpp_args = [
  '-I/usr/include/freetype2'
]

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
