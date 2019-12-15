from corels import CorelsClassifier

# ["loud", "samples"] is the most verbose setting possible
C = CorelsClassifier(max_card=2, c=0.8, verbosity=["loud", "samples"])

# 4 samples, 3 features
X = [[1, 0, 1], [0, 0, 0], [1, 1, 0], [0, 1, 0]]
y = [1, 0, 0, 1]
# Feature names
features = ["Mac User", "Likes Pie", "Age < 20"]

# Fit the model
C.fit(X, y, features=features, prediction_name="Has a dirty computer")

# Print the resulting rulelist
print(C.rl())

# Predict on the training set
print("Prediction: " + str(C.predict(X)))
