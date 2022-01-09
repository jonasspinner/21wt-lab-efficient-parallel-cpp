
#include <thread>
#include <iomanip>
#include "utils/commandline.h"
#include "implementation/mutex_std_queue.hpp"
#include "implementation/concurrent_queue_cas_element.hpp"
#include "utils/misc.h"
#include "implementation/concurrent_container.hpp"
#include "implementation/concurrent_queue.hpp"


template<class T, class Container>
double microbenchmark(size_t num_producers, size_t num_consumers, size_t num_elements, Container &container) {
    if (!container.empty()) {
        throw std::runtime_error("container is not empty");
    }

    auto producer = [&](size_t producer_thread_id) {
        size_t start = producer_thread_id * num_elements / num_producers;
        size_t end = (producer_thread_id + 1) * num_elements / num_producers;
        for (size_t i = start; i < end; ++i) {
            container.push(T{i} + 1);
        }
    };

    auto consumer = [&](size_t consumer_thread_id) {
        size_t start = consumer_thread_id * num_elements / num_consumers;
        size_t end = (consumer_thread_id + 1) * num_elements / num_consumers;
        for (size_t i = start; i < end; ++i) {
            [[maybe_unused]] auto e = container.pop();
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(num_producers + num_consumers);

    auto t0 = std::chrono::high_resolution_clock::now();

    for (size_t producer_thread_id = 0; producer_thread_id < num_producers; ++producer_thread_id) {
        threads.emplace_back(producer, producer_thread_id);
    }
    for (size_t consumer_thread_id = 0; consumer_thread_id < num_consumers; ++consumer_thread_id) {
        threads.emplace_back(consumer, consumer_thread_id);
    }
    for (auto &t: threads)
        t.join();

    auto t1 = std::chrono::high_resolution_clock::now();

    if (!container.empty()) {
        throw std::runtime_error("container is not empty");
    }

    return utils::to_ms(t1 - t0);
}


void print_header() {
    std::cout
            << std::setw(10) << "num_producers" << " "
            << std::setw(10) << "num_consumers" << " "
            << std::setw(10) << "num_elements" << " "
            << std::setw(10) << "capacity" << " "
            << std::setw(10) << "time" << std::endl;
}

void print_line(size_t num_producers, size_t num_consumers, size_t num_elements, size_t capacity, double time) {
    std::cout
            << std::setw(10) << num_producers << " "
            << std::setw(10) << num_consumers << " "
            << std::setw(10) << num_elements << " "
            << std::setw(10) << capacity << " "
            << std::setw(10) << time << std::endl;
}


int main(int argn, char **argc) {
    CommandLine cl(argn, argc);

    size_t num_producers = cl.intArg("-num-producers", 1);
    size_t num_consumers = cl.intArg("-num-consumers", 1);
    size_t num_elements = cl.intArg("-num-elements", 100000);
    size_t capacity = cl.intArg("-capacity", (1 << 15));
    size_t num_iterations = cl.intArg("-num-iterations", 5);

    size_t max_threads = std::thread::hardware_concurrency();

    {
        ConcurrentQueue<size_t> queue(capacity);
        double time = microbenchmark<size_t, decltype(queue)>(max_threads, max_threads, num_elements, queue);
        print_line(max_threads, max_threads, num_elements, capacity, time);
    }
    {
        MutexStdQueue<size_t> queue(capacity);
        double time = microbenchmark<size_t, decltype(queue)>(max_threads, max_threads, num_elements, queue);
        print_line(max_threads, max_threads, num_elements, capacity, time);
    }
    {
        ConcurrentContainer<size_t> container(capacity);
        double time = microbenchmark<size_t, decltype(container)>(max_threads, max_threads, num_elements, container);
        print_line(max_threads, max_threads, num_elements, capacity, time);
    }
    /*
    print_header();

    for (size_t i = 1; i <= max_threads; ++i) {
        for (size_t j = 1; j <= max_threads; ++j) {
            for (size_t k = 0; k < num_iterations; ++k) {
                ConcurrentQueue<size_t> queue(capacity);
                double time = microbenchmark<size_t, decltype(queue)>(i, j, num_elements, queue);
                print_line(i, j, num_elements, capacity, time);
            }
        }
    }


    print_header();

    for (size_t i = 1; i <= max_threads; ++i) {
        for (size_t j = 1; j <= max_threads; ++j) {
            for (size_t k = 0; k < num_iterations; ++k) {
                ConcurrentContainer<size_t> container(capacity);
                double time = microbenchmark<size_t, ConcurrentContainer<size_t>>(i, j, num_elements, container);
                print_line(i, j, num_elements, capacity, time);
            }
        }
    }
    */

    return 0;
}