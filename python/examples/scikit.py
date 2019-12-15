# Example integration of corels with pandas and scikit-learn

import pandas as pd
from sklearn import datasets
from sklearn.model_selection import train_test_split
import numpy as np
from corels import CorelsClassifier


# Load the iris dataset
iris = datasets.load_iris()
feature_names = list(iris.feature_names)

data = iris.data
targets = iris.target

# Binarize the features
for f in range(iris.data.shape[1]):
    mean = round(np.mean(data[:,f]), 3) 
    data[:,f] = (data[:,f] >= mean)
    feature_names[f] += " >= " + str(mean)

X, y = pd.DataFrame(data, columns=feature_names), targets

X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.1)

C_Setosa = CorelsClassifier(verbosity=[])
C_Versicolour = CorelsClassifier(verbosity=[])

C_Setosa.fit(X_train, y_train == 0, features=feature_names, prediction_name="Setosa")
s_Setosa = C_Setosa.score(X_test, y_test == 0)

C_Versicolour.fit(X_train, y_train == 1, features=feature_names, prediction_name="Versicolour")
s_Versicolour = C_Versicolour.score(X_test, y_test == 1)

print("SETOSA:")
print(C_Setosa.rl())
print("Setosa score = " + str(s_Setosa))

print("\n\nVERSICOLOUR:")
print(C_Versicolour.rl())
print("Versicolour score = " + str(s_Versicolour))
