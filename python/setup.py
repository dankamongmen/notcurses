# SPDX-License-Identifier: Apache-2.0

# Copyright 2020, 2021 igo95862

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

# http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
from __future__ import annotations

from os import environ

from setuptools import Extension, setup

from sys import platform

if environ.get('CFLAGS') is None:
    if platform == "darwin":
        environ['CFLAGS'] = (
            "-Wextra -Wconversion -Wall")
    else:
        environ['CFLAGS'] = (
            "-Werror "
            "-Wextra -Wconversion -Wall")

if environ.get('LDFLAGS') is None:
    if platform == "darwin":
        environ['LDFLAGS'] = "-Wl,-all_load"
    else:
        environ['LDFLAGS'] = "-Wl,--no-as-needed"

setup(
    name="notcurses",
    version="3.0.9",
    packages=['notcurses'],
    ext_modules=[
        Extension(
            name='notcurses.notcurses',
            sources=[
                'notcurses/channels.c',
                'notcurses/context.c',
                'notcurses/functions.c',
                'notcurses/main.c',
                'notcurses/misc.c',
                'notcurses/plane.c',
            ],
            libraries=['notcurses'],
            language='c',
        ),
    ],
    author="Nick Black",
    author_email="nickblack@linux.com",
    description="Blingful TUI construction library (python bindings)",
    keywords="ncurses curses tui console graphics",
    license='Apache License, Version 2.0',
    url='https://github.com/dankamongmen/notcurses',
    zip_safe=True,
    # see https://pypi.org/pypi?%3Aaction=list_classifiers
    classifiers=[
        'Development Status :: 4 - Beta',
        'Environment :: Console',
        'License :: OSI Approved :: Apache Software License',
        'Natural Language :: English',
        'Programming Language :: Python',
    ],
    package_data={
        'notcurses': [
            'py.typed',
        ],
    }
)
