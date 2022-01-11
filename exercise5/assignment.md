# Freestyle Assignment


## 1. Linked List (6)

Linked lists are important primitives which used for implementing more complex data structures.


### a) Implementation (5)

Implement a concurrent linked list. Think about how to make sure that elements do not get deallocated while some thread
wants to use it. The implementation should use atomic operations to ensure thread-safety.

Your linked list should support iteration, insertion and removal of elements. It might be beneficial to deffer the
actual removal of nodes to a later time.


### b) Evaluation (1)

Test your linked list with varying number of threads. Find a way to track how often atomic operations, such as
`compare_and_swap`, fail.



## 2. Concurrent Hashmap (10)

Implement a concurrent hashmap. The keys are assigned to
buckets by their hash value. Every bucket contains a linked list.

The map should
+ be templated for key, value, hash function and equal function
+ sustain heavy load

You can assume that the hashmap will be initialized with a capacity and does not need to grow.


### a) Implemenation (6)

Implement the concurrent hashmap by having a mutex for each bucket.


### b) Lockless variant (2)

Now use the linked list implementation from the previous task for the bucket data structure.



## 3. Benchmarking (4)

In many real world application the operations tend to be read-heavy and the occurrence of data point is skewed. Write
benchmarks that compare those scenarios with more artificial data.


### a) Data generation (1)

Implement a pseudo-random instance generator to generate uniform and skewed key distributions and varying ratio between
reads and writes.


### b) Evaluation (2)

Evaluate your lock-based and lockless variants of your concurrent hashmap implementation.

Vary
+ ratio between reads and writes
+ expected list size
+ uniform vs. skewed data distribution
+ size of the elements to insert.


### c) Comparison to other implementations (1)

Compare your implementation with an `unordered_map` implementation protected with a RWLock and at least one other
implementation of a concurrent hashmap.
