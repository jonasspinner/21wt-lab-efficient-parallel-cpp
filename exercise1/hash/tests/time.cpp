#include <iostream>
#include <fstream>
#include <random>
#include <chrono>
#include <string>
#include <limits>

#include "utils/commandline.h"

#ifdef HASH_STD
#include <unordered_map>
#define HASHTYPE std::unordered_map
#define OUTPUT   "hash_std.txt"
#endif

#ifdef HASH_ORIGINAL
#include "implementation/hash_original.h"
#define HASHTYPE HashOriginal
#define OUTPUT   "hash_original.txt"
#endif

#ifdef HASH_A
#include "implementation/hash_a.h"
#define HASHTYPE HashA
#define OUTPUT   "hash_a.txt"
#endif

#ifdef HASH_B
#include "implementation/hash_b.h"
#define HASHTYPE HashB
#define OUTPUT   "hash_b.txt"
#endif

namespace test_time
{
    template <class T>
    void print(std::ostream& out, const T& t, size_t w)
    {
        out.width(w);
        out  << t << " " << std::flush;
        std::cout.width(w);
        std::cout << t << " " << std::flush;
    }

    void print_headline(std::ostream& out)
    {
        print(out, "#it"    , 3);
        print(out, "sec"    , 4);
        print(out, "n_start", 9);
        print(out, "n_end"  , 9);
        print(out, "insert" , 8);
        print(out, "find_+" , 8);
        print(out, "find_-" , 8);
        print(out, "errors" , 8);
        out       << std::endl;
        std::cout << std::endl;
    }

    void print_timing(std::ostream& out,
                      size_t i, size_t s, size_t ns, size_t ne,
                      double in, double fi_p, double fi_m,
                      size_t err)
    {
        print(out, i   , 3);
        print(out, s   , 3);
        print(out, ns  , 9);
        print(out, ne  , 9);
        print(out, in  , 8);
        print(out, fi_p, 8);
        print(out, fi_m, 8);
        print(out, err , 8);
        out       << std::endl;
        std::cout << std::endl;
    }

    int test(size_t it, size_t n, size_t sec)
    {
        /* setup some data to use *********************************************/
        size_t           step  = n/sec;
        size_t*          keys  = new size_t[2*n];
        std::string      str[8] = {"a) this is a string, that is quite long",
                                   "b) we make the strings quite long to"   ,
                                   "c) evade short string optimization"     ,
                                   "d) lets do at least 8 different strings",
                                   "e) bla bla bla bla bla"                 ,
                                   "f) blub blub blub blub blub blub"       ,
                                   "g) this is for the hash table exercise" ,
                                   "h) find out why we use strings"};

        std::uniform_int_distribution<uint64_t> dis(1,std::numeric_limits<size_t>::max());
        std::mt19937_64 re;

        for (size_t i = 0; i < 2*n; ++i)
        {
            keys[i] = dis(re);
        }

        /* setup outputs ******************************************************/
        std::ofstream file(OUTPUT);
        print_headline(file);

        /* perform tests ******************************************************/
        for (size_t i = 0; i < it; ++i)
        {
            size_t errors(0);
            HASHTYPE<size_t, std::string, std::hash<size_t> > table(n);

            for (size_t s = 0; s < sec; ++s)
            {
                auto t0 = std::chrono::high_resolution_clock::now();

                /* insert n elements ******************************************/
                for (size_t i = s*step; i < (s+1)*step; ++i)
                {
                    table.insert( std::make_pair(keys[i], str[i&7]) );
                }
                auto t1 = std::chrono::high_resolution_clock::now();

                /* lookup n elements (unsuccessfully) *************************/
                for (size_t i = n+s*step; i < n+(s+1)*step; ++i)
                {
                    auto it = table.find(keys[i]);
                    if (it != table.end()) ++errors;
                }
                auto t2 = std::chrono::high_resolution_clock::now();

                /* lookup n elements (successfully) ***************************/
                for (size_t i = s*step; i < (s+1)*step; i++)
                {
                    auto it = table.find(keys[i]);
                    if (it == table.end()) ++errors;
                }
                auto t3 = std::chrono::high_resolution_clock::now();

                /* print current measurement **********************************/
                double d_insert = std::chrono::duration_cast<std::chrono::microseconds> (t1 - t0).count()/1000.;
                double d_find_0 = std::chrono::duration_cast<std::chrono::microseconds> (t2 - t1).count()/1000.;
                double d_find_1 = std::chrono::duration_cast<std::chrono::microseconds> (t3 - t2).count()/1000.;

                print_timing(file, i, s, s*step, (s+1)*step,
                             d_insert, d_find_1, d_find_0, errors);
            }
        }

        delete[] keys;
        return 0;
    }
}

int main (int argn, char** argc)
{
    CommandLine c(argn, argc);
    size_t it   = c.intArg("-it" , 5);
    size_t n    = c.intArg("-n"  , 5000000);
    size_t sec  = c.intArg("-sec", 10);

    test_time::test(it, n, sec);


}
