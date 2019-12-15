from corels import RuleList as R, CorelsClassifier as C
toy_X, toy_y = [[1, 0, 1], [0, 1, 1], [1, 1, 1]], [1, 0, 1]

def test_init():
    rules = "rules"
    features = "features"
    prediction = "prediction"

    r = R(rules, features, prediction)
    
    assert r.rules == rules
    assert r.features == features
    assert r.prediction_name == prediction

def test_store(tmp_path):
    r = C(verbosity=[]).fit(toy_X, toy_y).rl()

    p_rules = r.rules
    p_features = r.features
    p_prediction = r.prediction_name

    fname = str(tmp_path / "r.save")

    r.save(fname)

    r.rules = "garbage"
    r.features = "garbage"
    r.prediction_name = "garbage"


    r.load(fname)

    assert r.rules == p_rules
    assert r.features == p_features
    assert r.prediction_name == p_prediction

    # Sanity check
    assert r.rules != "garbage"

def test_str():
    r = C(verbosity=[]).fit(toy_X, toy_y).rl()

    assert str(r) == "RULELIST:\nif [feature1]:\n  prediction = True\nelse \n  prediction = False"

def test_repr():
    r = C(verbosity=[]).fit(toy_X, toy_y).rl()

    assert repr(r) == "RULELIST:\nif [feature1]:\n  prediction = True\nelse \n  prediction = False\nAll features: (['feature1', 'feature2', 'feature3'])"
