# CS253 Assignment 1: Memory-Efficient Versioned File Indexer

# Author Information
    Name: Chaniyara Yug
    Roll Number: 240299

# Overview
This project generates a comprehensive word-level index for large-scale text files.
It achieves strict memory efficiency by utilizing a fixed-size buffer for incremental text processing.
It completely prevents the underlying file from ever being fully loaded into memory.

# Compile Command
The source code can be compiled using the following command :
g++ 240299_CHANIYARA.cpp -o analyzer

# Commands to run different queries
The following commands can be used for different types of queries :

1. Word Count Query
Returns the frequency of a specific word in a version.
./analyzer --file test_logs.txt --version v1 --buffer 512 --query word --word error

2. Top-K Query
Displays the top-K most frequent words in a version, sorted in descending order.
./analyzer --file test_logs.txt --version v1 --buffer 512 --query top --top 10

3. Difference Query
Returns the frequency difference of a word between two versions.
./analyzer --file1 test_logs.txt --version1 v1 --file2 verbose_logs.txt --version2 v2 --buffer 512 --query diff --word error

# System Design
The solution follows an Object-Oriented Design using four specific classes:
    FileBufferReader : Class for incremental reading from the file.
    Tokenizer : Converts raw text into alphanumeric tokens and handles split words.
    VersionIndex : Class that maintains mapping for multiple versions.
    QueryProcessor : An abstract base class for processing various queries.

# Memory & Performance
Buffer Size : Buffer size is constant throughout and is strictly between 256 KB and 1024 KB.
Memory : Memory usage growns only with the number of unique words.
Runtime : Outputs total execution time in seconds.
