#!/usr/bin/env python3
"""Run a standalone native regression against an existing build; downloads nothing.

Usage: python3 unittest/run_native_regression.py --build-dir build_release \
    --source unittest/storage/blocksstable/test_string_diff_common_batch.cpp
Reuses the compiler settings and dependency set of that product build.
"""
import argparse
import json
from pathlib import Path
import shlex
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--build-dir', type=Path, required=True)
parser.add_argument('--output', type=Path)
parser.add_argument('--source', type=Path, required=True)
parser.add_argument('--sanitize', action='store_true', help='Instrument the test and inline routines with UBSan')
args = parser.parse_args()
build = args.build_dir.resolve()
output = (args.output or build / args.source.stem).resolve()
output.parent.mkdir(parents=True, exist_ok=True)
flags_file = build / 'src/storage/CMakeFiles/ob_storage_static.dir/flags.make'
flags = dict(line.split(' = ', 1) for line in flags_file.read_text().splitlines() if ' = ' in line)
compiler = next(line.removeprefix('# compile CXX with ') for line in flags_file.read_text().splitlines()
                if line.startswith('# compile CXX with '))
compile_command = [compiler, *shlex.split(flags['CXX_DEFINES']),
    *shlex.split(flags['CXX_INCLUDES']), *shlex.split(flags['CXX_FLAGS']),
    '-g0', '-c', str(args.source.resolve()),
    '-o', str(output) + '.o']
link_file = build / 'src/observer/CMakeFiles/seekdb.dir/link.txt'
link = shlex.split(link_file.read_text())
main_index = next(i for i, arg in enumerate(link) if arg.endswith('main.cpp.o'))
link[main_index] = str(output) + '.o'
link[link.index('-o') + 1] = str(output)
link += ['-Wl,--gc-sections', '-Wl,--strip-debug']
if args.sanitize:
    compile_command += ['-fsanitize=undefined', '-fno-sanitize-recover=all']
    link += ['-fsanitize=undefined', '-fno-sanitize-recover=all']
Path(str(output) + '.commands.json').write_text(
    json.dumps({'compile': compile_command, 'link': link}, indent=2))
subprocess.run(compile_command, check=True)
subprocess.run(link, cwd=link_file.parents[2], check=True)
subprocess.run([str(output)], cwd=output.parent, check=True)
