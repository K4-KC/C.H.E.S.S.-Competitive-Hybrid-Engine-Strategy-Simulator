from distutils.core import setup, Extension
import os

module1 = Extension('chess', sources = ['chessLib/chess.cpp'])

setup (name = 'chess',
       version = '1.0',
       description = 'This is a chess module in C',
       ext_modules = [module1])