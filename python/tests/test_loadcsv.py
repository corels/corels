from corels import load_from_csv
import numpy as np
import os

def test_compas():
    compas_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "compas.csv")
    X, y, features, prediction = load_from_csv(compas_path)

    assert len(features) == X.shape[1]
    assert X.shape[0] == y.shape[0]

    assert X.shape == (7214, 27) 
    assert y.shape == (7214, )

    assert features[0] == "Age=18-20"

    assert prediction == "Recidivate-Within-Two-Years"

    assert np.array_equal(y[:5], [0, 1, 1, 0, 0])
    assert np.array_equal(X[0,:10], [0, 0, 0, 0, 0, 1, 1, 1, 1, 0])
