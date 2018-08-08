# Computational-Finance-Parallel-Programming-Assignment
Implementation of Semaphore and Barrier in C++

## Exercises:
### Short version: 
Use mutexes and condition variables to implement semaphore and barrier
classes as outlined on page 71-73 in Antoine Savine (2017), \Elements of Modern Computa-
tional Finance"; the C++11 curriculum. Test your code.

### Hint/intructions:
The implementation can be done in many ways; there isn't an STL version yet. Simple
Google searches should get you useable, running code. From this you can then work forwards
to illustrate what semaphores and barriers do (two test examples below) and work backwards
to how they do it (and possibly improve to code).

### Two test examples:

* A library has 5 copies of a certain book on C++11. 50 people want to borrow the book.
Decribe/simulate the borrowing process with a semaphore. (What if the borrowers take
a random time to finish the book?)
* Use a barrier to implement the classical so-called divide-and-conquer algorithm (page
73 in the C++11 curriculum) for adding up the entires in a vector. (What if the number
of entries isn't a power of 2?)
