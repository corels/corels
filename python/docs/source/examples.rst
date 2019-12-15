Examples
========

Some of these examples make use of the COMPAS dataset, which `can be found here <https://raw.githubusercontent.com/fingoldin/pycorels/master/tests/data/compas.csv>`_.
Its data is stored with one comma-separated-row per sample, each with 26 binary features, and 1 classification (the 27th column). The first row specifies the feature names.


Toy Dataset
-----------
.. literalinclude:: ../../examples/toy.py
    :language: python


COMPAS Dataset
--------------
.. literalinclude:: ../../examples/compas.py
    :language: python


Scikit-learn
------------
.. literalinclude:: ../../examples/scikit.py
    :language: python
