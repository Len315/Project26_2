# Project26_2

This project implements four Unix utilities:
- wcat
- wgrep
- wzip
- wunzip

## Build
```bash
make
```

# wcat.c

Description:
Prints the contents of one or more text files to standard output.

Case 1, one input file:
```bash
./wcat file1.txt
```
Result:
```
one
two
three
```

Case 2, several input files:
```bash
./wcat file1.txt file2.txt file3.txt
```
Result:
```
one
two
three
file2 contents
file3 contents
```

Example:
```bash
make
./wcat tests/8.in
./test-wcat.sh
```

### Edge cases / Error handling
Run the provided test suite:
```bash
./test-wcat.sh
```

# wgrep.c

Description:
Searches for a string and prints all matching lines.

Case 1, searching from standard input:
```bash
echo -e "hello\nworld\nhello again" | ./wgrep hello
```
Result:
```
hello
hello again
```

Case 2, searching in one file:
```bash
./wgrep hello file.txt
```
Result:
```
hello
hello again
hello world
```

Case 3, searching in multiple files:
```bash
./wgrep second file1.txt file2.txt file3.txt
```
Result:
```
second line of file1
second line of file2
second line of file3
```

Example:
```bash
make
cat tests/4.in | ./wgrep words
./test-wgrep.sh
```

### Edge cases / Error handling
Run the provided test suite:
```bash
./test-wgrep.sh
```

# wzip.c

Description:
Compresses one or more files using run-length encoding (RLE).

Case 1, compressing one file:
```bash
./wzip input.txt > output.z
```

Result:
Writes the compressed binary data to `stdout`.

Case 2, compressing multiple files:
```bash
./wzip file1.txt file2.txt > output.z
```

Result:
Writes the compressed binary data to stdout.

Example:
```bash
make
./wzip tests/4.in > output.z
./test-wzip.sh
```

### Edge cases / Error handling
Run the provided test suite:
```bash
./test-wzip.sh
```

# wunzip.c

Description:
Decompresses files created by `wzip`.

Case 1, decompressing one file:
```bash
./wunzip output.z
```

Result:
Writes the decompressed text to `stdout`.

Case 2, decompressing to a file:
```bash
./wunzip output.z > restored.txt
```

Result:
The decompressed text is written to `restored.txt`.

Case 3, decompressing multiple compressed files:
```bash
./wunzip part1.z part2.z > restored.txt
```

Result:
The decompressed contents of all files are written to `restored.txt`.

Example:
```bash
make
./wunzip output.z
./test-wunzip.sh
```

### Edge cases / Error handling
Run the provided test suite:
```bash
./test-wunzip.sh
```
