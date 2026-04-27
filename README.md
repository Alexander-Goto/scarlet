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
