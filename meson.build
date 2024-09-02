project('bltmenu', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3', 'cpp_std=c++14'])

deps = [ dependency('x11') ]
src = [
  'bltmenu.cpp',
  'src/X11.cpp'
]

executable('bltmenu',
           sources: src,
           install : true,
           dependencies : deps)
