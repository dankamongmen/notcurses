from setuptools import setup
from setuptools.command.install import install
from setuptools.command.sdist import sdist
from distutils.command.build import build
import os
import shutil

here = os.path.dirname(__file__) or '.'


def read(fname):
    return open(os.path.join(os.path.dirname(__file__), fname)).read()


def copy_include_dir():
    src = os.path.join(here, "../include")
    dst = os.path.join(here, "include")
    if os.path.exists(src):
        if os.path.exists(dst):
            shutil.rmtree(dst)
        shutil.copytree(src, dst)


class ManPageGenerator(install):
    def run(self):
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


copy_include_dir()


cmdclass = {}
try:
    import pypandoc
    cmdclass["install"] = ManPageGenerator
except ImportError:
    print("warning: pypandoc module not found, won't generate man pages")

setup(
    name="notcurses",
    version="3.0.8",
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
    cmdclass=cmdclass,
)
