from setuptools import setup, Extension, find_packages
from setuptools.command.build_ext import build_ext
from setuptools.command.build_ext import build_ext
from setuptools.glob import glob
import os, sys, subprocess

__version__ = "0.0.1"


ext_modules = [
    Extension(
        name="sprtop.sprtop_core",
        sources=["sprtop/sprtop_core.cpp"],
    ),
]


class cmake_build_ext(build_ext):
    def build_extension(self, ext: Extension):
        # Ensure build_temp exists
        self.mkpath(self.build_temp)

        # Get the path to the source
        build_temp = os.path.abspath(self.build_temp)
        path_to_source = os.path.dirname(os.path.realpath(__file__))

        # Get pybind11 cmake dir
        try:
            import pybind11
            pybind11_dir = pybind11.get_cmake_dir()
        except ImportError:
            pybind11_dir = ""

        # Call cmake
        args = [
            f"cmake",
            f"-DCMAKE_BUILD_TYPE=Release",
            f"-DCMAKE_C_COMPILER=/usr/bin/gcc-13",
            f"-DCMAKE_CXX_COMPILER=/usr/bin/g++-13",
            f"-DPYTHON_EXECUTABLE={sys.executable}",
            f"-DPYTHON_LIBRARY_OUTPUT_DIRECTORY={build_temp}",
        ]
        if pybind11_dir:
            args.append(f"-Dpybind11_DIR={pybind11_dir}")
        args.append(path_to_source)
        print(" ".join(args))
        subprocess.call(args, cwd=build_temp)

        # Call make
        args = ["make", "-j2"]
        print(" ".join(args))
        subprocess.call(args, cwd=build_temp)

        # Copy library
        ext_fullpath = self.get_ext_fullpath(ext.name)
        self.mkpath(os.path.dirname(ext_fullpath))

        if sys.platform in ["win32", "cygwin"]:
            ext_fullpaths_temp = glob(os.path.join(build_temp, "sprtop_core.*.pyd"))
        else:
            ext_fullpaths_temp = glob(os.path.join(build_temp, "sprtop_core.*.so"))
        if len(ext_fullpaths_temp) == 0:
            print("Error: did not build Python extension sprtop_core")
            exit(1)
        if len(ext_fullpaths_temp) >= 2:
            print("Error: built multiple Python extensions sprtop_core")
            exit(1)
        ext_fullpath_temp = ext_fullpaths_temp[0]
        self.copy_file(ext_fullpath_temp, ext_fullpath)


setup(
    name="sprtop",
    version=__version__,
    description="Show Topology for Sapphire Rapids",
    author="victoryang00",
    author_email="victoryang00@ucsc.edu",
    install_requires=["pybind11>=2.4"],
    ext_modules=ext_modules,
    cmdclass={"build_ext": cmake_build_ext},
    packages=find_packages(),
    entry_points={
        "console_scripts": [
            "sprtop=sprtop:run",
        ],
    },
)
