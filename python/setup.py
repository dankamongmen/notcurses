from setuptools import setup
import os

def read(fname):
    return open(os.path.join(os.path.dirname(__file__), fname)).read()

setup(
    name="notcurses",
    version="1.5.3",
    packages=['notcurses'],
    package_dir={'': 'src'},
    author="Nick Black",
    author_email="nickblack@linux.com",
    description="Blingful TUI construction library (python bindings)",
    keywords="ncurses curses tui console graphics",
    license='Apache License, Version 2.0',
    url='https://github.com/dankamongmen/notcurses',
    zip_safe=True,
    platforms=["any"],
    long_description=read('README.md'),
    long_description_content_type="text/markdown",
    install_requires=["cffi>=1.0.0"],
    setup_requires=["cffi>=1.0.0"],
    cffi_modules=["src/notcurses/build_notcurses.py:ffibuild"],
    # see https://pypi.org/pypi?%3Aaction=list_classifiers
    classifiers=[
        'Development Status :: 4 - Beta',
        'Environment :: Console',
        'License :: OSI Approved :: Apache Software License',
        'Natural Language :: English',
        'Programming Language :: Python',
    ],
)
