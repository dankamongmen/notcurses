from setuptools import setup
from setuptools.command.install import install
import os
import sys

class ManPageGenerator(install):
    def run(self):
        here = os.path.dirname(__file__) or '.'
        files = []
        outfile = 'notcurses-pydemo.1'
        pypandoc.convert_file(os.path.join(here, 'notcurses-pydemo.1.md'), 'man', outputfile=outfile, extra_args=['-s'])
        files.append(outfile)
        outfile = 'ncdirect-pydemo.1'
        pypandoc.convert_file(os.path.join(here, 'ncdirect-pydemo.1.md'), 'man', outputfile=outfile, extra_args=['-s'])
        files.append(outfile)
        # this breaks when using --user without --prefix
        ipage = (os.path.join(self.prefix, 'share', 'man', 'man1'), files)
        self.distribution.data_files.append(ipage)
        print("data_files: ", self.distribution.data_files)
        super().run()

try:
    import pypandoc
except ImportError:
    print("warning: pypandoc module not found, won't generate man pages")
    manpageinstaller=dict()
else:
    manpageinstaller=dict(
        install=ManPageGenerator,
    )

def read(fname):
    return open(os.path.join(os.path.dirname(__file__), fname)).read()

setup(
    name="notcurses",
    version="2.3.17",
    packages=['notcurses'],
    scripts=['notcurses-pydemo', 'ncdirect-pydemo'],
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
    data_files=[],
    install_requires=["cffi>=1.0.0"],
    setup_requires=["cffi>=1.0.0", "pypandoc>=1.4"],
    cffi_modules=["src/notcurses/build_notcurses.py:ffibuild"],
    # see https://pypi.org/pypi?%3Aaction=list_classifiers
    classifiers=[
        'Development Status :: 4 - Beta',
        'Environment :: Console',
        'License :: OSI Approved :: Apache Software License',
        'Natural Language :: English',
        'Programming Language :: Python',
    ],
    include_package_data=True,
    cmdclass=manpageinstaller
)
