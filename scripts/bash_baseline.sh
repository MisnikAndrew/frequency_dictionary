#!/usr/bin/env python3

import re

def count_distinct_words(filename):
    with open(filename, "r", encoding="utf-8", errors="replace") as f:
        text = f.read()
        cleaned_text = re.sub(r"[^a-zA-Z]+", " ", text)
        words = cleaned_text.lower().split()
        distinct_words = set(words)
        return len(distinct_words)

file_name = "pg.txt"
total_distinct = count_distinct_words(file_name)
print(f"Number of distinct words: {total_distinct}")


