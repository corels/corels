from corels import load_from_csv, RuleList, CorelsClassifier as C
import corels
import pytest
import sys
import time
import numpy as np
import os
import warnings

compas_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "compas.csv")
compas_X, compas_y, compas_features, compas_prediction = load_from_csv(compas_path)

toy_X, toy_y = [[1, 0, 1], [0, 1, 1], [1, 1, 1]], [1, 0, 1]

warnings.simplefilter("ignore")

def test_fit_Xy():
    c = C(verbosity=[])

    valid = [
        ([[1, 0]], [1]),
        (np.array([[1, 0]]), np.array([1])),
        ([[1, 0, 1], [0, 1, 0]], np.array([1, 0]))
    ]

    invalid_type = [
        ("wrong_type", [1, 0]),
        ([[1, 0]], "wrong_type"),
        (1, 1)
    ]

    invalid_value = [
        ([1, 0], [1]),
        ([[1, 0]], [1, 0]),
        ([[1, 0]], [[1]]),
        ([[2, 0]], [1]),
        ([[1, 0]], [2])
    ]

    for pair in valid:
        c.fit(pair[0], pair[1])
        c.score(pair[0], pair[1])
        c.predict(pair[0])

    for pair in invalid_type:
        with pytest.raises(TypeError):
            c.fit(pair[0], pair[1])
            c.score(pair[0], pair[1])
    
    for pair in invalid_value:
        with pytest.raises(ValueError):
            c.fit(pair[0], pair[1])
            c.score(pair[0], pair[1])

def test_predict():
    # Test predict
    invalid_type = ["wrong_type", 1]
    invalid_value = [[1, 0], [[2, 0]]]

    c = C(verbosity=[])
    
    c.fit([[1, 0]], [1])
     
    for p in invalid_type:
        with pytest.raises(TypeError):
            c.predict(p)
    
    for p in invalid_value:
        with pytest.raises(ValueError):
            c.predict(p)

    c.fit(toy_X, toy_y)
    assert np.array_equal(c.predict(toy_X), toy_y)
    
    c.fit(compas_X, compas_y)
    assert np.sum(c.predict(compas_X)) == 3057

def test_score():
    c = C(verbosity=[])

    # Test score with a predict vector
    assert c.score([1, 0], [1, 0]) == 1.0

    # Can it learn simple datasets?
    assert c.fit(toy_X, toy_y).score(toy_X, toy_y) == 1.0

    c.fit(compas_X, compas_y)
    c_score = c.score(compas_X, compas_y)
    assert abs(c_score - 0.6609370) < 0.0001
    assert c_score == c.score(c.predict(compas_X), compas_y)

def test_notfitted():
    c = C()

    with pytest.raises(ValueError) as ex_info:
        c.predict([[1, 0]])
    
    assert "not fitted yet" in str(ex_info.value)

    with pytest.raises(ValueError) as ex_info:
        c.score([[1, 0]], [1])

    assert "not fitted yet" in str(ex_info.value)

def test_general_params():
    # Format: name, type, default_value, valid_values, invalid_values, invalid_type_value
    params = [
        ("c", float, 0.01, [0.0, 1.0, 0.01, 0.05, 0.5], [-1.01, 1.01, 100.0], "str"),
        ("n_iter", int, 10000, [0, 1, 1000, 1000000000], [-1, -100], 1.4),
        ("ablation", int, 0, [0, 1, 2], [-1, 3, 100], 1.5),
        ("min_support", float, 0.01, [0.0, 0.25, 0.5], [-0.01, 0.6, 1.5], "str"),
        ("map_type", str, "prefix", ["none", "prefix", "captured"], ["yay", "asdf"], 4),
        ("policy", str, "lower_bound", ["bfs", "curious", "lower_bound", "objective", "dfs"], ["yay", "asdf"], 4),
        ("max_card", int, 2, [1, 2, 3], [0, -1], 1.4),
        ("verbosity", list, ["rulelist"], [["rule", "label", "mine"], ["label"], ["samples", "mine", "minor"], ["progress", "loud"]], [["whoops"], ["rule", "label", "nope"]], "str")
    ]

    for param in params:
        # Test constructor default initialization
        c = C()
        assert getattr(c, param[0]) == param[2]
        assert type(getattr(c, param[0])) == param[1]
        
        # Test constructor assignment and valid arguments
        for v in param[3]:
            c = C(**{param[0]: v}).fit(toy_X, toy_y)
            assert getattr(c, param[0]) == v
            assert type(getattr(c, param[0])) == param[1]

        # Test value errors
        for v in param[4]:
            with pytest.raises(ValueError):
                C(**{param[0]: v}).fit([[1, 0]], [1])
         
        # Test type error
        with pytest.raises(TypeError):
            C(**{param[0]: param[5]}).fit([[1, 0]], [1])
       
def test_set_params():
    c = C()

    r = 0.1
    n_iter = 10
    map_type = "captured"
    policy = "dfs"
    verbosity = ["loud", "samples"]
    ablation = 2
    max_card = 4
    min_support = 0.1

    c.set_params(c=r, n_iter=n_iter, map_type=map_type, policy=policy,
                 verbosity=verbosity, ablation=ablation, max_card=max_card,
                 min_support=min_support)

    assert c.c == r
    assert c.n_iter == n_iter
    assert c.map_type == map_type
    assert c.policy == policy
    assert c.verbosity == verbosity
    assert c.ablation == ablation
    assert c.max_card == c.max_card
    assert c.min_support == c.min_support

    # Test singular assignment
    c.set_params(n_iter=1234)
    assert c.n_iter == 1234

    with pytest.raises(ValueError):
        c.set_params(random_param=4)

def test_get_params():
    c = C()

    params = c.get_params()

    assert params["c"] == 0.01
    assert params["n_iter"] == 10000
    assert params["map_type"] == "prefix"
    assert params["policy"] == "lower_bound"
    assert params["verbosity"] == ["rulelist"]
    assert params["ablation"] == 0
    assert params["max_card"] == 2
    assert params["min_support"] == 0.01

def test_rl():
    c = C(verbosity=[])

    with pytest.raises(ValueError) as ex_info:
        _ = c.rl()
    assert "not fitted yet" in str(ex_info.value)

    c.fit(toy_X, toy_y)

    assert c.rl_ == c.rl()

    with pytest.raises(ValueError):
        c.rl(0)
    
    with pytest.raises(TypeError):
        c.rl(RuleList(1, 1, 1))

    assert str(c.rl()) != "RULELIST:\npred = 0"
    c.rl(RuleList([{"antecedents":[0],"prediction":0}], ["feature1"], "pred"))
    assert str(c.rl()) == "RULELIST:\npred = 0"


def test_store(tmp_path):
    c = C(verbosity=[]).fit(toy_X, toy_y)

    rl = c.rl()
    params = c.get_params()
   
    fname = str(tmp_path / "r.save")
    c.save(fname)

    for name,_ in params.items():
        c.set_params(**{ name: 0 })
    c.rl_ = 0;

    assert c.get_params() != params

    c.load(fname)

    assert c.get_params() == params
    assert c.rl().rules == rl.rules

def test_maxcard():
    # Test cardinality cannot be greater than n_features
    with pytest.raises(ValueError):
        C(max_card=10).fit(toy_X, toy_y)

    max_cards = [1, 2, 3]
    rule_cards = [] 
    
    for max_card in max_cards:
        c = C(c=0.0, max_card=max_card, verbosity=[]).fit(compas_X, compas_y)

        rule_cards.append(len(c.rl().rules[0]["antecedents"]))

    for i in range(1, len(rule_cards)):
        assert rule_cards[i] > rule_cards[i - 1]

def test_niter():
    times = []
    n_iters = [10, 100000]

    for n_iter in n_iters:
        s = time.time()
        C(n_iter=n_iter, verbosity=[]).fit(compas_X, compas_y)
        t = time.time() - s
        
        times.append(t)

    for i in range(1, len(times)):
        assert times[i] > times[i - 1]

def test_min_support():
    X = [[1, 0, 0], [0, 1, 0], [0, 0, 1]]
    y = [1, 0, 0]

    nrules1 = len(C(verbosity=[]).fit(X, y).rl().rules)
    nrules2 = len(C(verbosity=[], min_support=0.4).fit(X, y).rl().rules)

    assert nrules1 > 1
    assert nrules2 == 1

def test_prediction_name():
    with pytest.raises(TypeError):
        C().fit(toy_X, toy_y, prediction_name=4)

    name = "name"
    rname = C(verbosity=[]).fit(toy_X, toy_y, prediction_name=name).rl().prediction_name
    assert rname == name
    
    rname = C(verbosity=[]).fit(toy_X, toy_y).rl().prediction_name
    assert rname == "prediction"

def test_features():
    c = C(verbosity=[])

    with pytest.raises(ValueError):
        c.fit(toy_X, toy_y, features=["only_one"])

    with pytest.raises(TypeError):
        c.fit(toy_X, toy_y, features="wrong_type")
    
    with pytest.raises(TypeError):
        c.fit(toy_X, toy_y, features=["wrong_inner_type", 4])
    
    f1 = c.fit(toy_X, toy_y).rl().features
    assert f1 == ["feature" + str(i + 1) for i in range(len(toy_y)) ]

    custom = ["custom" + str(i + 1) for i in range(len(toy_y)) ]
    f2 = c.fit(toy_X, toy_y, features=custom).rl().features
    assert f2 == custom

def test_general_compas():
    # Test the whole algorithm
    c = C(verbosity=[], c=0.001)

    rl = c.fit(compas_X, compas_y, compas_features, compas_prediction).rl()

    assert rl.__str__() == "RULELIST:\nif [Age=24-30 && Prior-Crimes=0]:\n  Recidivate-Within-Two-Years = False\nelse if [not Age=18-25 && not Prior-Crimes>3]:\n  Recidivate-Within-Two-Years = False\nelse \n  Recidivate-Within-Two-Years = True"

    # Test using the same classifier twice    
    c.set_params(max_card=1)
    rl = c.fit(compas_X, compas_y, compas_features, compas_prediction).rl()

    assert rl.__str__() == "RULELIST:\nif [Prior-Crimes>5]:\n  Recidivate-Within-Two-Years = True\nelse if [Age<=40]:\n  Recidivate-Within-Two-Years = False\nelse if [Age=18-20]:\n  Recidivate-Within-Two-Years = True\nelse if [Prior-Crimes>3]:\n  Recidivate-Within-Two-Years = True\nelse if [Age>=30]:\n  Recidivate-Within-Two-Years = False\nelse if [Prior-Crimes=0]:\n  Recidivate-Within-Two-Years = False\nelse \n  Recidivate-Within-Two-Years = True"

def test_general_other():
    c = C(verbosity=[])
    
    data_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "data_train.csv")
    data_X, data_y, data_features, data_prediction = load_from_csv(data_path)

    rl = c.fit(data_X, data_y, data_features, data_prediction).rl()

    assert rl.__str__() == "RULELIST:\nlabel = False"

def test_c():
    cl = C(verbosity=[], n_iter=100000)
    cs = [0.3, 0.01, 0.0]
    rls = []

    for c in cs:
        cl.set_params(c=c)
        rls.append(cl.fit(compas_X, compas_y).rl())
    
    for i in range(1, len(rls)):
        assert len(rls[i].rules) > len(rls[i - 1].rules)

    warnings.simplefilter("error", RuntimeWarning)

    # Sanity check
    cl.set_params(c=0.01)
    cl.fit(compas_X, compas_y)

    # Test warnings
    with pytest.warns(RuntimeWarning):
        cl.set_params(c=(0.8 / compas_X.shape[0]))
        cl.fit(compas_X, compas_y)
 
    with pytest.warns(RuntimeWarning):
        cl.set_params(c=0.49)
        cl.fit(compas_X, compas_y)
