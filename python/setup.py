from setuptools import setup, find_packages
import os

def read(fname):
    return open(os.path.join(os.path.dirname(__file__), fname)).read()

setup(
    name="notcurses",
    version="1.0.0",
    package_dir={'': 'src'},
    packages=find_packages('src'),
    author="Nick Black",
    author_email="nickblack@linux.com",
    description="Blingful TUI construction library",
    keywords="ncurses curses tui console graphics",
    license='Apache License, Version 2.0',
    url='https://github.com/dankamongmen/notcurses',
    zip_safe=True,
    platforms=["any"],
    long_description=read('README.md'),
    long_description_content_type="text/markdown",
    setup_requires=["pytest-runner"],
    tests_require=["pytest"],
    # see https://pypi.org/pypi?%3Aaction=list_classifiers
    classifiers=[
        'Development Status :: 3 - Alpha',
        'Environment :: Console',
        'License :: OSI Approved :: Apache Software License',
        'Natural Language :: English',
        'Programming Language :: Python',
    ],
)
