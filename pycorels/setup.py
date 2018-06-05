from distutils.core import setup, Extension
import numpy as np

pycorels = Extension('pycorels',
                    sources = ['pycorels.c'],
                    libraries = ['corels', 'gmpxx', 'gmp'],
                    library_dirs = ['../src'],
                    include_dirs = [np.get_include()],
		            extra_compile_args = ["-DGMP"])

setup (name = 'pycorels',
       version = '0.1',
       description = 'Python binding of CORELS algorithm',
       ext_modules = [pycorels])
