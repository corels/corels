from corels import *
import numpy as np

# Train split proportion
train_proportion = 0.8

X, y, features, prediction = load_from_csv("data/compas.csv")
# A maximum cardinality of 3 makes CORELS search all rule antecedents
# with up to three features combined together
c = CorelsClassifier(max_card=3, n_iter=1000000, verbosity=["progress","rulelist"])

# Generate train and test sets
train_split = int(train_proportion * X.shape[0])

X_train = X[:train_split]
y_train = y[:train_split]

X_test = X[train_split:]
y_test = y[train_split:]

# Fit the model. Features is a list of the feature names
c.fit(X_train, y_train, features=features, prediction_name=prediction)

# Score the model on the test set
a = c.score(X_test, y_test)

print("Test Accuracy: " + str(a))

# Print the rulelist
print(c.rl())
