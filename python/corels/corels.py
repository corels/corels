from __future__ import print_function, division, with_statement
from ._corels import fit_wrap_begin, fit_wrap_end, fit_wrap_loop, predict_wrap
from .utils import check_consistent_length, check_array, check_is_fitted, get_feature, check_in, check_features, check_rulelist, RuleList
import numpy as np
import pickle
import warnings

class CorelsClassifier:
    """Certifiably Optimal RulE ListS classifier.

    This class implements the CORELS algorithm, designed to produce human-interpretable, optimal
    rulelists for binary feature data and binary classification. As an alternative to other
    tree based algorithms such as CART, CORELS provides a certificate of optimality for its 
    rulelist given a training set, leveraging multiple algorithmic bounds to do so.

    In order to use run the algorithm, create an instance of the `CorelsClassifier` class, 
    providing any necessary parameters in its constructor, and then call `fit` to generate
    a rulelist. `printrl` prints the generated rulelist, while `predict` provides
    classification predictions for a separate test dataset with the same features. To determine 
    the algorithm's accuracy, run `score` on an evaluation dataset with labels.
    To save a generated rulelist to a file, call `save`. To load it back from the file, call `load`.

    Attributes
    ----------
    c : float, optional (default=0.01)
        Regularization parameter. Higher values penalize longer rulelists.

    n_iter : int, optional (default=10000)
        Maximum number of nodes (rulelists) to search before exiting.

    map_type : str, optional (default="prefix")
        The type of prefix map to use. Supported maps are "none" for no map,
        "prefix" for a map that uses rule prefixes for keys, "captured" for
        a map with a prefix's captured vector as keys.

    policy : str, optional (default="lower_bound")
        The search policy for traversing the tree (i.e. the criterion with which
        to order nodes in the queue). Supported criteria are "bfs", for breadth-first
        search; "curious", which attempts to find the most promising node; 
        "lower_bound" which is the objective function evaluated with that rulelist
        minus the default prediction error; "objective" for the objective function
        evaluated at that rulelist; and "dfs" for depth-first search.

    verbosity : list, optional (default=["rulelist"])
        The verbosity levels required. A list of strings, it can contain any
        subset of ["rulelist", "rule", "label", "minor", "samples", "progress", "mine", "loud"].
        An empty list ([]) indicates 'silent' mode.

        - "rulelist" prints the generated rulelist at the end.
        - "rule" prints a summary of each rule generated.
        - "label" prints a summary of the class labels.
        - "minor" prints a summary of the minority bound.
        - "samples" produces a complete dump of the rules, label, and/or minor data. You must also provide at least one of "rule", "label", or "minor" to specify which data you want to dump, or "loud" for all data. The "samples" option often spits out a lot of output.
        - "progress" prints periodic messages as corels runs.
        - "mine" prints debug information while mining rules, including each rule as it is generated.
        - "loud" is the equivalent of ["progress", "label", "rule", "mine", "minor"].

    ablation : int, optional (default=0)
        Specifies addition parameters for the bounds used while searching. Accepted
        values are 0 (all bounds), 1 (no antecedent support bound), and 2 (no
        lookahead bound).

    max_card : int, optional (default=2)
        Maximum cardinality allowed when mining rules. Can be any value greater than
        or equal to 1. For instance, a value of 2 would only allow rules that combine
        at most two features in their antecedents.

    min_support : float, optional (default=0.01)
        The fraction of samples that a rule must capture in order to be used. 1 minus
        this value is also the maximum fraction of samples a rule can capture.
        Can be any value between 0.0 and 0.5.

    References
    ----------
    Elaine Angelino, Nicholas Larus-Stone, Daniel Alabi, Margo Seltzer, and Cynthia Rudin.
    Learning Certifiably Optimal Rule Lists for Categorical Data. KDD 2017.
    Journal of Machine Learning Research, 2018; 19: 1-77. arXiv:1704.01701, 2017

    Examples
    --------
    >>> import numpy as np
    >>> from corels import CorelsClassifier
    >>> X = np.array([ [1, 0, 1], [0, 1, 0], [1, 1, 1] ])
    >>> y = np.array([ 1, 0, 1])
    >>> c = CorelsClassifier(verbosity=[])
    >>> c.fit(X, y)
    ...
    >>> print(c.predict(X))
    [ True False  True ]
    """
    
    _estimator_type = "classifier"

    def __init__(self, c=0.01, n_iter=10000, map_type="prefix", policy="lower_bound",
                 verbosity=["rulelist"], ablation=0, max_card=2, min_support=0.01):
        self.c = c
        self.n_iter = n_iter
        self.map_type = map_type
        self.policy = policy
        self.verbosity = verbosity
        self.ablation = ablation
        self.max_card = max_card
        self.min_support = min_support

    def fit(self, X, y, features=[], prediction_name="prediction"):
        """
        Build a CORELS classifier from the training set (X, y).

        Parameters
        ----------
        X : array-like, shape = [n_samples, n_features]
            The training input samples. All features must be binary, and the matrix
            is internally converted to dtype=np.uint8.

        y : array-line, shape = [n_samples]
            The target values for the training input. Must be binary.
        
        features : list, optional(default=[])
            A list of strings of length n_features. Specifies the names of each
            of the features. If an empty list is provided, the feature names
            are set to the default of ["feature1", "feature2"... ].

        prediction_name : string, optional(default="prediction")
            The name of the feature that is being predicted.

        Returns
        -------
        self : obj
        """
        if not isinstance(self.c, float):
            raise TypeError("Regularization constant (c) must be a float, got: " + str(type(self.c)))
        if self.c < 0.0 or self.c > 1.0:
            raise ValueError("Regularization constant (c) must be between 0.0 and 1.0, got: " + str(self.c))
        if not isinstance(self.n_iter, int):
            raise TypeError("Max nodes must be an integer, got: " + str(type(self.n_iter)))
        if self.n_iter < 0:
            raise ValueError("Max nodes must be positive, got: " + str(self.n_iter))
        if not isinstance(self.ablation, int):
            raise TypeError("Ablation must be an integer, got: " + str(type(self.ablation)))
        if self.ablation > 2 or self.ablation < 0:
            raise ValueError("Ablation must be between 0 and 2, inclusive, got: " + str(self.ablation))
        if not isinstance(self.map_type, str):
            raise TypeError("Map type must be a string, got: " + str(type(self.map_type)))
        if not isinstance(self.policy, str):
            raise TypeError("Policy must be a string, got: " + str(type(self.policy)))
        if not isinstance(self.verbosity, list):
            raise TypeError("Verbosity must be a list of strings, got: " + str(type(self.verbosity)))
        if not isinstance(self.min_support, float):
            raise TypeError("Minimum support must be a float, got: " + str(type(self.min_support)))
        if self.min_support < 0.0 or self.min_support > 0.5:
            raise ValueError("Minimum support must be between 0.0 and 0.5, got: " + str(self.min_support))
        if not isinstance(self.max_card, int):
            raise TypeError("Max cardinality must be an integer, got: " + str(type(self.max_card)))
        if self.max_card < 1:
            raise ValueError("Max cardinality must be greater than or equal to 1, got: " + str(self.max_card))
        if not isinstance(prediction_name, str):
            raise TypeError("Prediction name must be a string, got: " + str(type(prediction_name)))
       
        label = check_array(y, ndim=1)
        labels = np.stack([ np.invert(label), label ])
        samples = check_array(X, ndim=2)
        check_consistent_length(samples, labels)

        n_samples = samples.shape[0]
        n_features = samples.shape[1]
        if self.max_card > n_features:
            raise ValueError("Max cardinality (" + str(self.max_card) + ") cannot be greater"
                             " than the number of features (" + str(n_features) + ")")

        if self.c < 1.0 / n_samples:
            warnings.warn("Regularization parameter should not be less than 1/nsamples = " +
                           str(1.0 / n_samples), RuntimeWarning, stacklevel=2)
        
        l_ones = np.sum(label)
        
        if self.c > min(l_ones, n_samples - l_ones) / n_samples:
            warnings.warn("Regularization parameter should not be greater than min(negative_examples, positive_examples) / n_samples = " +
                           str(min(l_ones, n_samples - l_ones) / n_samples), RuntimeWarning, stacklevel=2)

        n_labels = labels.shape[0]
        
        rl = RuleList()
        
        if features:
            check_features(features)
            rl.features = list(features)
        else:
            rl.features = []
            for i in range(n_features):
                rl.features.append("feature" + str(i + 1))

        if rl.features and len(rl.features) != n_features:
            raise ValueError("Feature count mismatch between sample data (" + str(n_features) + 
                             ") and feature names (" + str(len(rl.features)) + ")")
        
        rl.prediction_name = prediction_name

        allowed_verbosities = ["rulelist", "rule", "label", "samples", "progress", "loud", "mine", "minor"]
        for v in self.verbosity:
            if not isinstance(v, str):
                raise TypeError("Verbosity flags must be strings, got: " + str(v))

            check_in("Verbosities", allowed_verbosities, v)
        
        if "samples" in self.verbosity \
              and "rule" not in self.verbosity \
              and "label" not in self.verbosity \
              and "minor" not in self.verbosity \
              and "loud" not in self.verbosity:
            raise ValueError("'samples' verbosity option must be combined with at" + 
                             " least one of 'rule', 'label', 'minor', or 'loud'")

        # Verbosity for rule mining and minority bound. 0 is quiet, 1 is verbose
        mine_verbose = 0
        if "loud" in self.verbosity or "mine" in self.verbosity:
            mine_verbose = 1
        
        minor_verbose = 0
        if "loud" in self.verbosity or "minor" in self.verbosity:
            minor_verbose = 1
        
        verbose = ",".join([ v for v in self.verbosity if v != "rulelist" and v != "mine" ])

        map_types = ["none", "prefix", "captured"]
        policies = ["bfs", "curious", "lower_bound", "objective", "dfs"]

        check_in("Map type", map_types, self.map_type)
        check_in("Search policy", policies, self.policy)

        map_id = map_types.index(self.map_type)
        policy_id = policies.index(self.policy)

        fr = fit_wrap_begin(samples.astype(np.uint8, copy=False),
                             labels.astype(np.uint8, copy=False), rl.features,
                             self.max_card, self.min_support, verbose, mine_verbose, minor_verbose,
                             self.c, policy_id, map_id, self.ablation, False)
        
        if fr:
            early = False
            try:
                while fit_wrap_loop(self.n_iter):
                    pass
            except:
                print("\nExiting early")
                rl.rules = fit_wrap_end(True)
                
                self.rl_ = rl
                
                if "rulelist" in self.verbosity:
                    print(self.rl_)

                raise
             
            rl.rules = fit_wrap_end(False)
            
            self.rl_ = rl

            if "rulelist" in self.verbosity:
                print(self.rl_)
        else:
            print("Error running model! Exiting")

        return self

    def predict(self, X):
        """
        Predict classifications of the input samples X.

        Arguments
        ---------
        X : array-like, shape = [n_samples, n_features]
            The training input samples. All features must be binary, and the matrix
            is internally converted to dtype=np.uint8. The features must be the same
            as those of the data used to train the model.

        Returns
        -------
        p : array of shape = [n_samples].
            The classifications of the input samples.
        """

        check_is_fitted(self, "rl_")
        check_rulelist(self.rl_)        

        samples = check_array(X, ndim=2)
        
        if samples.shape[1] != len(self.rl_.features):
            raise ValueError("Feature count mismatch between eval data (" + str(X.shape[1]) + 
                             ") and feature names (" + str(len(self.rl_.features)) + ")")

        return np.array(predict_wrap(samples.astype(np.uint8, copy=False), self.rl_.rules), dtype=np.bool)

    def score(self, X, y):
        """
        Score the algorithm on the input samples X with the labels y. Alternatively,
        score the predictions X against the labels y (where X has been generated by 
        `predict` or something similar).

        Arguments
        ---------
        X : array-like, shape = [n_samples, n_features] OR shape = [n_samples]
            The input samples, or the sample predictions. All features must be binary.
        
        y : array-like, shape = [n_samples]
            The input labels. All labels must be binary.

        Returns
        -------
        a : float
            The accuracy, from 0.0 to 1.0, of the rulelist predictions
        """

        labels = check_array(y, ndim=1)
        p = check_array(X)
        check_consistent_length(p, labels)
        
        if p.ndim == 2:
            p = self.predict(p)
        elif p.ndim != 1:
            raise ValueError("Input samples must have only 1 or 2 dimensions, got " + str(p.ndim) +
                             " dimensions")

        a = np.mean(np.invert(np.logical_xor(p, labels)))

        return a

    def get_params(self):
        """
        Get a list of all the model's parameters.
        
        Returns
        -------
        params : dict
            Dictionary of all parameters, with the names of the parameters as the keys
        """

        return {
            "c": self.c,
            "n_iter": self.n_iter,
            "map_type": self.map_type,
            "policy": self.policy,
            "verbosity": self.verbosity,
            "ablation": self.ablation,
            "max_card": self.max_card,
            "min_support": self.min_support
        }

    def set_params(self, **params):
        """
        Set model parameters. Takes an arbitrary number of keyword parameters, all of which
        must be valid parameter names (i.e. must be included in those returned by get_params).

        Returns
        -------
        self : obj
        """
        
        valid_params = self.get_params().keys()

        for param, val in params.items():
            if param not in valid_params:
                raise ValueError("Invalid parameter '" + str(param) + "' given in set_params. "
                                 "Check the list of valid parameters with get_params()")
            setattr(self, param, val)

        return self

    def save(self, fname):
        """
        Save the model to a file, using python's pickle module.

        Parameters
        ----------
        fname : string
            File name to store the model in
        
        Returns
        -------
        self : obj
        """

        with open(fname, "wb") as f:
            pickle.dump(self, f)

        return self

    def load(self, fname):
        """
        Load a model from a file, using python's pickle module.
        
        Parameters
        ----------
        fname : string
            File name to load the model from
        
        Returns
        -------
        self : obj
        """

        with open(fname, "rb") as f:
            model = pickle.load(f)
           
            if not hasattr(model, "get_params"):
                raise ValueError("Invalid model provided, model must have get_params() method")
                
            self.set_params(**model.get_params())

            if hasattr(model, "rl_"):
                self.rl_ = model.rl_

        return self

    def rl(self, set_val=None):
        """
        Return or set the learned rulelist
        
        Parameters
        ----------
        set_val : RuleList, optional
            Rulelist to set the model to

        Returns
        -------
        rl : obj
            The model's rulelist
        """

        if set_val != None:
            check_rulelist(set_val)

            self.rl_ = set_val
        else:
            check_is_fitted(self, "rl_")
        
        return self.rl_
    
    def __str__(self):
        s = "CorelsClassifier (" + str(self.get_params()) + ")"

        if hasattr(self, "rl_"):
            s += "\n" + self.rl_.__str__()

        return s
    
    def __repr__(self):
        s = "CorelsClassifier (" + str(self.get_params()) + ")"

        if hasattr(self, "rl_"):
            s += "\n" + self.rl_.__repr__()

        return s
