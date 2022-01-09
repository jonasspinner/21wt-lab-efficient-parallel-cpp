
#include <thread>
#include <iomanip>
#include "utils/commandline.h"
#include "implementation/mutex_std_queue.hpp"
#include "implementation/concurrent_queue_cas_element.hpp"
#include "utils/misc.h"
#include "implementation/concurrent_container.hpp"
#include "implementation/concurrent_queue.hpp"


template<class T, class Container>
std::chrono::nanoseconds
microbenchmark(size_t num_producers, size_t num_consumers, size_t num_elements, Container &container) {
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

    return t1 - t0;
}


void print_header() {
    std::cout
            << std::setw(22) << "\"data_structure\"" << " "
            << std::setw(10) << "\"num_producers\"" << " "
            << std::setw(10) << "\"num_consumers\"" << " "
            << std::setw(10) << "\"num_elements\"" << " "
            << std::setw(10) << "\"capacity\"" << " "
            << std::setw(10) << "\"time (ms)\"" << std::endl;
}

void print_line(const std::string &data_structure, size_t num_producers, size_t num_consumers, size_t num_elements,
                size_t capacity,
                std::chrono::nanoseconds time) {
    std::cout
            << std::setw(22) << data_structure << " "
            << std::setw(10) << num_producers << " "
            << std::setw(10) << num_consumers << " "
            << std::setw(10) << num_elements << " "
            << std::setw(10) << capacity << " "
            << std::setw(10) << utils::to_ms(time) << std::endl;
}


template<class Container>
void run_grid(const std::string &data_structure, size_t max_threads, size_t num_iterations, size_t num_elements,
              size_t capacity) {
    print_header();

    for (size_t i = 1; i <= max_threads; ++i) {
        for (size_t j = 1; j <= max_threads; ++j) {
            for (size_t k = 0; k < num_iterations; ++k) {
                Container queue(capacity);
                auto time = microbenchmark<size_t, decltype(queue)>(i, j, num_elements, queue);
                print_line(data_structure, i, j, num_elements, capacity, time);
            }
        }
    }
}


int main(int argn, char **argc) {
    CommandLine cl(argn, argc);

    size_t max_threads = cl.intArg("-max-threads", (int) std::thread::hardware_concurrency());
    size_t num_elements = cl.intArg("-num-elements", 100000);
    size_t capacity = cl.intArg("-capacity", (1 << 15));
    size_t num_iterations = cl.intArg("-num-iterations", 5);
    std::string data_structure = cl.strArg("-data-structure", "concurrent-queue");


    if (data_structure == "concurrent-queue") {
        run_grid<ConcurrentQueue<int>>(data_structure, max_threads, num_iterations, num_elements, capacity);
    } else if (data_structure == "mutex-std-queue") {
        run_grid<MutexStdQueue<int>>(data_structure, max_threads, num_iterations, num_elements, capacity);
    } else if (data_structure == "concurrent-container") {
        run_grid<ConcurrentContainer<int>>(data_structure, max_threads, num_iterations, num_elements, capacity);
    } else {
        std::cout << "invalid  \"-data-structure " << data_structure << "\"" << std::endl;
        return 1;
    }

    return 0;
}