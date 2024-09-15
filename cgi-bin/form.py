#!/usr/bin/env python3
import sys
import os
from urllib.parse import parse_qs


def parse_input():
    return parse_qs(os.environ.get("QUERY_STRING", ""))


# Print headers
print("Content-Type: text/html")
print("Status: 200 OK")
print()

# Start HTML output
print("<html><body>")
print("<h1>CGI Form Script</h1>")

print("<h2>Environment Variables:</h2>")

print("<ul>")
for key, value in os.environ.items():
    print(f"<li><strong>{key}:</strong> {value}</li>")
print("</ul>")

print("<h2>Query Parameters:</h2>")
form_data = parse_input()
if form_data:
    print("<ul>")
    for key, values in form_data.items():
        for value in values:
            print(f"<li><strong>{key}:</strong> {value}</li>")
    print("</ul>")
else:
    print("<p>No query parameters received.</p>")
