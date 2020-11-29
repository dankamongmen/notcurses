# SPDX-License-Identifier: Apache-2.0

# Copyright 2020 igo95862

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

# http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from setuptools import setup, Extension

setup(
    name="notcurses",
    version="2.0.2",
    packages=['notcurses'],
    ext_modules=[
        Extension('notcurses/_notcurses', ['notcurses/_notcurses.c']),
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
)
