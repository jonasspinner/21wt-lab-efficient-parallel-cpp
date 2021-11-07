#include <iostream>
#include <fstream>
#include <random>
#include <chrono>
#include <limits>
#include <vector>

#include "utils/commandline.h"


/* DEFINITIONS FOR SUBTASK A **************************************************/
#ifdef PQ_A
#include "implementation/pq_a.h"
#define PQ_TYPE PriQueueA
#define CALL_TEST(testfunction, it, n, deg, sec) \
    testfunction<PQ_TYPE<size_t> >(it,n,deg,sec)
#define PQ_CONSTR(n,deg) n, deg
#define OUTPUT "pq_a.txt"
#endif

/* DEFINITIONS FOR SUBTASK B **************************************************/
#ifdef PQ_B
#include "implementation/pq_b.h"
#define PQ_TYPE   PriQueueB
#define CALL_TEST(testfunction, it, n, deg, sec) \
    switch(deg) { \
    case 2:  testfunction<PQ_TYPE<size_t, 2 > >(it,n,deg,sec); break; \
    case 4:  testfunction<PQ_TYPE<size_t, 4 > >(it,n,deg,sec); break; \
    case 8:  testfunction<PQ_TYPE<size_t, 8 > >(it,n,deg,sec); break; \
    case 16: testfunction<PQ_TYPE<size_t, 16> >(it,n,deg,sec); break; \
    case 6:  testfunction<PQ_TYPE<size_t, 6 > >(it,n,deg,sec); break; \
    case 7:  testfunction<PQ_TYPE<size_t, 7 > >(it,n,deg,sec); break; \
    case 10: testfunction<PQ_TYPE<size_t, 10> >(it,n,deg,sec); break; \
    case 12: testfunction<PQ_TYPE<size_t, 12> >(it,n,deg,sec); break; \
    default: testfunction<PQ_TYPE<size_t> >(it,n,deg,sec); break; \
    }
#define PQ_CONSTR(n,deg) n
#define OUTPUT    "pq_b.txt"
#endif

/* DEFINITIONS FOR SUBTASK C **************************************************/
#ifdef PQ_C
#include "implementation/pq_c.h"
size_t log(size_t deg)
{
    size_t res  = 0;
    size_t temp = 1;
    while (temp < deg) { temp <<= 1; res++; }
    std::cout << std::endl << res << std::endl;
    return res;
}
#define PQ_TYPE PriQueueC
#define CALL_TEST(testfunction, it, n, deg, sec) \
    testfunction<PQ_TYPE<size_t> >(it,n,deg,sec)
#define PQ_CONSTR(n,deg) n, log(deg)
#define OUTPUT "pq_c.txt"
#endif

/* DEFINITIONS FOR SUBTASK D **************************************************/
#ifdef PQ_D
#include "implementation/pq_d.h"
size_t log(size_t deg)
{
    size_t res  = 0;
    size_t temp = 1;
    while (temp < deg) { temp <<= 1; res++; }
    std::cout << std::endl << res << std::endl;
    return res;
}
#define PQ_TYPE PriQueueD
#define CALL_TEST(testfunction, it, n, deg, sec) \
    testfunction<PQ_TYPE<size_t> >(it,n,deg,sec)
#define PQ_CONSTR(n,deg) n, log(deg)
#define OUTPUT "pq_d.txt"
#endif

/* DEFINITIONS FOR THE STD BASELINE IMPLEMENTATION ****************************/
#ifdef PQ_STD
#include <queue>
#define PQ_TYPE std::priority_queue
#define CALL_TEST(testfunction, it, n, deg, sec) \
    testfunction<PQ_TYPE<size_t> >(it,n,deg,sec)
#define PQ_CONSTR(n,deg)
#define OUTPUT "pq_std.txt"
#endif



/* MAIN TEST DESCRIPTION ******************************************************/
namespace test_time
{
    template <class T>
    void print(std::ostream& out, const T& t, size_t w)
    {
        out.width(w);
        out       << t << " " << std::flush;
        std::cout.width(w);
        std::cout << t << " " << std::flush;
    }

    void print_headline(std::ostream& out)
    {
        print(out, "#it"    , 3);
        print(out, "sec"    , 4);
        print(out, "deg"    , 4);
        print(out, "n_start", 9);
        print(out, "n_end"  , 9);
        print(out, "insert" , 8);
        print(out, "pop"    , 8);
        print(out, "errors" , 8);
        out       << std::endl;
        std::cout << std::endl;
    }

    void print_timing(std::ostream& out,
                      size_t i, size_t s, size_t deg, size_t n_s, size_t n_e,
                      double in, double pop,
                      size_t err)
    {
        print(out, i   , 3);
        print(out, s   , 4);
        print(out, deg , 4);
        print(out, n_s , 9);
        print(out, n_e , 9);
        print(out, in  , 8);
        print(out, pop , 8);
        print(out, err , 8);
        out       << std::endl;
        std::cout << std::endl;
    }

    template<class PQ>
    int test(size_t it, size_t n, size_t deg, size_t sec)
    {
        /* setup some data to use *********************************************/
        constexpr size_t range      = (1ull<<63)-1;
        size_t*          priorities = new size_t[n];
        size_t           sec_size   = n/sec;

        std::uniform_int_distribution<uint64_t> dis(1,range);
        std::mt19937_64 re;

        for (size_t i = 0; i < n; ++i)
        {
            priorities[i] = dis(re);
        }

        /* setup outputs ******************************************************/
        std::ofstream file(OUTPUT);
        print_headline(file);

        /* perform tests ******************************************************/
        for (size_t i = 0; i < it; ++i)
        {
            size_t              errors(0);
            PQ                  table = PQ( PQ_CONSTR(n, deg) );
            std::vector<double> times;

            size_t c = 0;

            /* insert n elements **********************************************/
            for (size_t s = 0; s < sec; ++s)
            {
                auto t0 = std::chrono::high_resolution_clock::now();
                for (size_t i = 0; i < sec_size; ++i)
                {
                    table.push(priorities[i+c]);
                }
                auto t1 = std::chrono::high_resolution_clock::now();

                times.push_back(
                    std::chrono::duration_cast<std::chrono::microseconds>
                    (t1 - t0).count()/1000.);
                c += sec_size;
            }

            /* lookup n elements (successfully) *******************************/
            size_t prev = std::numeric_limits<size_t>::max();
            for (size_t s = 0; s < sec; ++s)
            {
                auto t0 = std::chrono::high_resolution_clock::now();
                for (size_t i = 0; i < sec_size; ++i)
                {
                    auto cur = table.top();
                    if (cur > prev) errors++;
                    prev = cur;
                    table.pop();
                }
                auto t1 = std::chrono::high_resolution_clock::now();

                times.push_back(
                    std::chrono::duration_cast<std::chrono::microseconds>
                    (t1 - t0).count()/1000.);
            }

            /* print current measurement **************************************/
            for (size_t s = 0; s < sec; ++s)
            {
                print_timing(file, i, s, deg,  s*sec_size, (s+1)*sec_size,
                             times[s], times[2*sec-1-s], errors);
            }
        }

        delete[] priorities;
        return 0;
    }
}

int main (int argn, char** argc)
{
    CommandLine c(argn, argc);
    size_t it = c.intArg ("-it" , 5);
    size_t n  = c.intArg ("-n"  , 10000000);
    size_t sec = c.intArg("-sec", 10);
    size_t deg = c.intArg("-deg", 8);

    //return test_time::test<PQ_TYPE<size_t> >(it, n, deg,  sec);
    CALL_TEST(test_time::test, it, n, deg, sec);
    return 0;
}
