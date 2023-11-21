# Engel
The official, C++ implementation of Engel.


**UPDATE:** this is _scrapped_. As of November 2023 I am creating a new implementation from scratch, still in C++ but this time using the standard libraries and the full features of the language, rather than this "C with classes" style that has allowed enough bugs to creep in to make this implementation unviable; I have surpassed that middle part of the C/C++ learning curve where pointers are cool, now I see them as too much trouble for their worth, as the C++ standard library provides low-cost and often free abstraction. in the end, only the runtime loop matters and that won't have to change much, I can still use my low-level hacks there while keeping the rest of the interpreter easy to reason about. If it doesn't have to do with Engel objects (ie, what the garbage collector handles), the implementation should not be concerned with the exact structure of the memory, as the standard library can handle that during the compilation pass. Nobody is asking for compilers* to be faster in 2023, only the bytecode VM matters.



* well, except for C++ compilers
