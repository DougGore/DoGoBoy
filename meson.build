project('dogoboy', 'c')

sdl_sp = subproject('sdl2')

dogoboy_inc = include_directories('include')
dogoboy_srcs = ['src/main.c', 'src/graphics.c', 'src/memory.c', 'src/sharp_LR35902.c']

executable('dogoboy', dogoboy_srcs,
    dependencies : sdl_sp.get_variable('sdl2_dep'),
    include_directories: dogoboy_inc)