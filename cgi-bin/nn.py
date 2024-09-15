#!/usr/bin/env python3
try:
    import sys
    import os
    from urllib.parse import parse_qs
    from sklearn.datasets import fetch_20newsgroups
    from sklearn.feature_extraction.text import TfidfVectorizer
    from sklearn.naive_bayes import MultinomialNB
    import pickle
    import ssl
    ssl._create_default_https_context = ssl._create_unverified_context
except Exception as e:
    print(f"Content-Type: text/html")
    print("Status: 500 Internal Server Error")
    print()
    print(f"Error importing libraries: {str(e)}")
    sys.exit(1)


# Global model and vectorizer (kept in memory)
model = None
vectorizer = None


def train_model():
    global model, vectorizer

    # Load the dataset (subset for faster training)
    newsgroups_train = fetch_20newsgroups(
        subset="train",
        categories=["sci.space", "rec.sport.hockey", "talk.politics.misc"],
    )

    # Transform the text data into TF-IDF feature vectors
    vectorizer = TfidfVectorizer()
    X_train = vectorizer.fit_transform(newsgroups_train.data)
    y_train = newsgroups_train.target

    # Train a simple Naive Bayes classifier
    model = MultinomialNB()
    model.fit(X_train, y_train)


def parse_input():
    """Parse the CGI input to get the user input text"""
    if os.environ.get("REQUEST_METHOD") == "POST":
        content_length = int(os.environ.get("CONTENT_LENGTH", 0))
        return parse_qs(sys.stdin.read(content_length))
    else:
        return parse_qs(os.environ.get("QUERY_STRING", ""))


def predict(text):
    """Use the trained model to predict the category of the text"""
    if model and vectorizer:
        text_vector = vectorizer.transform([text])  # Transform input text to vector
        prediction = model.predict(text_vector)
        return prediction[0]  # Return the predicted category index
    else:
        raise ValueError("Model is not trained yet.")


# Train the model at startup
train_model()

# Print headers for the CGI response
print("Content-Type: text/html")
print("Status: 200 OK")
print()

# Start HTML output
print("<html><body>")
print("<h1>Text Classification Using Pre-trained Model</h1>")

form_data = parse_input()

# Check if 'text' is in the form data
if not form_data or "text" not in form_data:
    print("<p>Error: Please provide some text in the input.</p>")
else:
    try:
        # Get the input text
        input_text = form_data["text"][0]

        # Use the pre-trained model for prediction
        prediction = predict(input_text)

        # Map prediction to the actual category name
        categories = ["sci.space", "rec.sport.hockey", "talk.politics.misc"]
        predicted_category = categories[prediction]

        # Output the prediction
        print(f"<p><strong>Predicted Category:</strong> {predicted_category}</p>")

    except Exception as e:
        print(f"<p>Error: {str(e)}</p>")

print("</body></html>")
