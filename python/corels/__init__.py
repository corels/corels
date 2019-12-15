from __future__ import print_function, division, with_statement
from .corels import CorelsClassifier
from .utils import load_from_csv, RuleList
import os

with open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'VERSION')) as f:
    __version__ = f.read().strip()

__all__ = ["CorelsClassifier", "load_from_csv", "RuleList"]
