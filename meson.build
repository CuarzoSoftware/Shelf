project(
    'Shelf',
    'c','cpp',
    version : '0.1.0',
    meson_version: '>= 0.62.0',
    default_options: [
        'warning_level=2',
        'buildtype=release',
        'cpp_std=c++20'])

skia_dep = dependency('Skia')
kay_dep = dependency('Kay')
marco_dep = dependency('Marco')
xdgkit_dep = dependency('XDGKit')

sources = run_command('find', './src', '-type', 'f', '-name', '*[.cpp,.c]', check : false).stdout().strip().split('\n')

executable(
    'shelf',
    sources : sources,
    include_directories : ['./src'],
    dependencies : [
        skia_dep,
        kay_dep,
        marco_dep,
        xdgkit_dep
    ],
    install : true)
