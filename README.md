# scarlet
Compiler for the Scar programming language.

# Key features of Scar:
* Immutability. There are no mutable structures in scar at all.
* Scar has a mechanism that allows it to very quickly detect at runtime situations where new data is created based on old data, but the old data is no longer used. In such situations, the old data is modified instead of creating new data.
* Memory management is automatic, but no garbage collector is used.
* Scar is a functional language with support for prototype-based programming.
* Compilation on the application developer's machine occurs in a platform-independent temporary representation, and the final compilation of the application into an executable file occurs on the end user's machine, being adapted to their machine.
* Clean and readable syntax.
* The language is designed to be suitable for a very wide range of tasks.
* Very strong dynamic typing.
* Minimum runtime size. ("hello world" - ~47 Kb)

# Build
The clang compiler is required for build.

Supported platforms:
* linux x86_64

`git clone https://github.com/Alexander-Goto/scarlet`

`cd scarlet/release/0.1`

`sh build.sh`

The `sh build.sh` command takes a few minutes.

# Frequently Asked Questions
## Why was the language created?
There are two reasons:
1. Programs are often created for an abstract average computer (yes, there are caveats, but still).
If a program is originally adapted to specific hardware, it will run better on that hardware than the same program written for an abstract average machine.
For this reason, Scar was created. It is initially designed to compile to a cross-platform intermediate representation,
with final compilation taking place on the end user's machine, adapting to their hardware.
2. No existing language met the author's requirements, so the language was created.
Requirements:
- At the language level, all data must be immutable. This avoids a class of errors that are very difficult to find and fix.
- Typically, languages that use immutable data employ data structures originally adapted for immutable data,
and in many cases they are less efficient than structures used in languages with mutable data. The goal was a combination of efficient data structures and their immutability.
- Automatic memory management without a garbage collector.
- Simple and readable syntax.
- When a developer uses a particular programming language and has extensive knowledge of that language and its ecosystem, they are likely to develop any application
in that language, even if it is poorly suited to the task. Therefore, the goal was to create a language that is sufficiently suitable for a wide range of tasks,
even if it is not the optimal choice for any single one.
- Strict typing.
- No code generation during compilation. This is important because code generation during compilation slows down compilation and can lead to errors
that are very hard to find and fix.
- Convenient splitting of a project into multiple files.
- Every expression must return a result.

## How does memory management work?
The language uses reference counting.
Not all data has a reference count, only data that uses the heap (and not even all of that).
During compilation, the compiler determines where data is used for the last time (within a single function or global constant), for example:

    fn (array) {
        foo(array) # this is not the last use of array
        bar(array) # this is the last use of array
    }
If some data is not being used for the last time, the reference count is incremented by one; if it is the last use, the count is not changed.
Whoever directly uses the data must decrement the reference count and free the memory if necessary
(in the example, function `foo` decrements the count by one but certainly does not free the data, while function `bar` frees the data if the reference count is one).
This mechanism also makes it possible to determine whether heap memory can be modified: if the reference count is 1, it can be modified; if it is 0, the data is a
global constant and cannot be modified, including for changes to the reference count.
Under the hood, many pieces of data are modified in place rather than creating new data, for example:

    let array   = [array 1, 2, 3]         # reference count 0 because data is known at compile time
    let array_1 = new (array)[0] <= rnd() # the `new` operator checks the reference count, sees it is not 1, so it must create a new array and replace the element at index 0 with the value returned by `rnd()`
    let array_2 = array_1.replace(2, 7)   # the `replace` function checks the reference count, sees it is 1 (since `array_1` is not used anywhere else), so it can simply reuse the memory from `array_1`
    array_2.println()                     # the function prints the array and frees it

`_1`, `_2` are used for clarity; Scar supports variable shadowing, and the following code does the same thing:

    let array = [array 1, 2, 3]
    let array = new (array)[0] <= rnd()
    let array = array.replace(2, 7)
    array.println()

## Who is the author of the language?
The author of the language is an ordinary person, without any titles, who has never worked as a professional programmer, and has been programming as a hobby for over 25 years.

## What is the current state of the language?
- The compiler is written in the language it compiles.
- Multithreading is supported (and works well).
- Lambda functions and closures are supported.
- The compiler supports optimizations that are unexpected in a version 0.1 compiler.
- The core module (built into the compiler) is already optimized at a level unexpected for a version 0.1 language.
For example, substring search in a string is optimized using SIMD instructions, and when searching for a long string in a very long string, the Rabin-Karp algorithm is used.
- The core module contains approximately 240 functions of various purposes.
- Around 31,300 unit tests have been written for the core module.
- There is a std module that contains functions and types for working with the operating system (files, launching programs, etc.).
- Around 950 unit tests have been written for the std module.

## What is the future development of the language?
The language's author has ideas for a very large number of programs, and they will be written in Scar.
If in the process something is missing from the language, it will be added; if bugs are found, they will be fixed.
When the language reaches a stage where everything satisfies the author sufficiently, version 1.0 will be released,
and any changes that seriously break backward compatibility will be prohibited.

## Which languages influenced Scar?
The author of Scar created 11 languages and 12 compilers before Scar, and the main influence on Scar came from its predecessors, as well as to some extent from Haskell and Go.