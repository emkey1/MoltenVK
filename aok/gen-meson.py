#!/usr/bin/env python3
# Regenerates ../meson.build's source lists from the tree (MoltenVK's own
# CMake build globs the same directories). Run after rebasing onto a new
# MoltenVK release: python3 aok/gen-meson.py > meson.build
import os
root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
def sources(d, exts=('.c', '.cpp', '.m', '.mm')):
    out = []
    for dp, dn, fn in os.walk(os.path.join(root, d)):
        dn.sort()
        for f in sorted(fn):
            if f.endswith(exts):
                out.append(os.path.relpath(os.path.join(dp, f), root))
    return sorted(out)
def fmt(name, files):
    return name + ' = files(\n' + ''.join("  '%s',\n" % f for f in files) + ')\n'
spirv_cross = ['aok/external/SPIRV-Cross/' + f for f in (
    'spirv_cfg.cpp', 'spirv_cross.cpp', 'spirv_cross_parsed_ir.cpp', 'spirv_parser.cpp',
    'spirv_glsl.cpp', 'spirv_msl.cpp', 'spirv_reflect.cpp')]
print(open(os.path.join(root, 'aok/meson.build.in')).read()
      .replace('@SOURCES@', fmt('mvk_common_sources', sources('Common'))
               + fmt('mvk_converter_sources', sources('MoltenVKShaderConverter/MoltenVKShaderConverter'))
               + fmt('mvk_sources', sources('MoltenVK/MoltenVK'))
               + fmt('spirv_cross_sources', spirv_cross)), end='')
