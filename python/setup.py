#!/usr/bin/env python

from distutils.core import setup, Extension

module1 = Extension ('gmediadb',
                     include_dirs = ['/usr/include/glib-2.0', '/usr/lib/glib-2.0/include'],
                     libraries = ['glib-2.0', 'gmediadb'],
                     sources = ['gmediadb-python.c', 'gmediadb-GMediaDB.c'])

setup (name = 'GMediaDB',
       version = '0.7',
       description = 'GMediaDB python bindings',
       author = 'Brett Mravec',
       author_email = 'brett.mravec@gmail.com',
       url = 'http://github.com/bmravec/GMediaDB',
       ext_modules = [module1])
