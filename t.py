import os
import re

def remove_multiline_comments(code):
    """Remove multi-line comments from code."""
    pattern = re.compile(r'/\*.*?\*/', re.DOTALL)
    return pattern.sub('', code)

def remove_single_line_comments(code):
    """Remove single-line comments from code, preserving strings."""
    # This pattern matches strings and comments
    pattern = re.compile(r'(".*?"|//.*?$)', re.MULTILINE)

    def replacer(match):
        s = match.group(0)
        if s.startswith('//'):
            return ''  # Remove the comment
        return s  # Keep the string literal

    return pattern.sub(replacer, code)

def replace_single_statement_blocks(code):
    """Replace single-statement code blocks with braces to a single line without braces."""
    # Pattern to match control statements with single-line blocks
    pattern = re.compile(
        r'''(^\s*          # Start of the line with optional whitespace
            (if|for|while|else\s*if|else|do|switch)  # Control statements
            [^\{]*         # Anything except '{'
            \{             # Opening brace
            \s*            # Optional whitespace
            (              # Start of inner group (the block)
                (?:[^{}]|\{[^{}]*\})*  # Non-brace characters or nested braces
            )
            \s*            # Optional whitespace
            \}             # Closing brace
            )''',
        re.MULTILINE | re.VERBOSE
    )

    def replacer(match):
        full_match = match.group(0)
        leading_whitespace = re.match(r'^\s*', full_match).group(0)
        control_statement = re.match(r'^\s*(if|for|while|else\s*if|else|do|switch)[^\{]*', full_match).group(0)
        inner_code = match.group(3).strip()

        # Check if inner_code contains only one statement
        statements = [s for s in inner_code.split(';') if s.strip()]
        if len(statements) == 1:
            # Reconstruct the code without braces
            new_code = f"{control_statement.strip()} {statements[0].strip()};"
            # Maintain proper indentation
            return leading_whitespace + new_code
        else:
            return full_match  # Return the original code if conditions aren't met

    return pattern.sub(replacer, code)

def process_file(file_path):
    with open(file_path, 'r') as f:
        code = f.read()

    # Remove single-line comments
    code = remove_single_line_comments(code)

    # Replace single-statement code blocks
    code = replace_single_statement_blocks(code)

    # Write the modified code back to the file
    with open(file_path, 'w') as f:
        f.write(code)

def find_and_process_files(directory):
    for root, _, files in os.walk(directory):
        for file in files:
            if file.endswith(('.cpp', '.hpp')):
                file_path = os.path.join(root, file)
                print(f"Processing {file_path}...")
                process_file(file_path)

if __name__ == '__main__':
    # Ask the user for the directory to work on
    directory = input("Enter the directory to process: ")
    if not os.path.isdir(directory):
        print("Invalid directory.")
    else:
        find_and_process_files(directory)
