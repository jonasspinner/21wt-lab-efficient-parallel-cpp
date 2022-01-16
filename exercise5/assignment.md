# Freestyle Assignment

## 1. Concurrent Singly Linked List (7)

Linked lists are important primitives which used for implementing more complex
data structures.

In this task you should implement a singly linked list that can be used
concurrently. Your implementations should support `insert`, `find` and `remove`.

It might be beneficial to defer the actual removal of nodes to a later time.

### a) Implementation with per-Node Mutex (3)

Implement a concurrent linked list. Think about how to make sure that elements
do not get deallocated while some thread wants to use it. You can use one lock
per list item.

### b) Lockless Variant (2)

Adapt your previous implementation and make it lockless by using atomic
operations.

### c) Evaluation (2)

Test your implementations with varying number of threads. Compare a linked list
with one lock, the list from a) with one lock per list item and the list b)
with no locks.

Vary the percentage of read and write operation.

## 2. Concurrent Hashmap (7)

Implement a concurrent hashmap. Every key is mapped to at most one value. The
keys are assigned to buckets by their hash value. Every bucket contains a
linked list.

The map should

+ be templated for key, value, hash function and equal function,
+ provide `insert`, `find` and `remove` operations and
+ sustain heavy load.

You can assume that the hashmap will be initialized with a capacity and does
not need to grow. You don't have to copy behaviour of `std::unordered_map`'s
`operator[]` to default construct the value if the key is not present in the
map. Think about the exact types you want to provide with your operations.

### a) Implementation (3)

Implement the concurrent hashmap by having a mutex for each bucket.

### b) Lockless Variant (2)

Now use the lockless implementation from the previous task for the bucket
data structure.

### c) Bucket bloom filters (2)

It might be beneficial to be able to quickly determine that an element is not
in a bucket. Add a 64-bit bloom filter to each bucket. You can use atomic
operations to update the bloom filter.

## 3. Benchmarking (6)

In many real world application the operations tend to be read-heavy and the
occurrence of data point is skewed. Write benchmarks that compare those
scenarios with more artificial data.

### a) Data Generator (1)

Implement a pseudo-random instance generator to generate uniform and skewed
key distributions and varying ratio between reads and writes.

### b) Evaluation (3)

Evaluate your mutex-based and lockless variants of your concurrent hashmap
implementation.

Vary

+ ratio between reads and writes,
+ expected list size (i.e. number of elements vs. capacity),
+ uniform vs. skewed data distribution and
+ size of the elements to insert.

If you have implemented the bloom filters in task 2 c), then also try
different number of hash functions for the bloom filters.

### c) Comparing Implementations (2)

Compare your implementation with an `std::unordered_map` implementation
protected with a lock and at least one other implementation of a concurrent
hashmap.
