#include <cstdio>
#include <thread>
#include <vector>
#include "bpcqueue.hpp"

using namespace bpcqueue;

void produce(Output<int> dst, int n) {
    for (int i=0; i<n; ++i)
	dst.push(i);
}

void count(Input<int> src, Output<int> dst) {
    int x, count=0;
    while (src.pop(x))
        ++count;
    dst.push(count);
}

void print_sum(Input<int> src) {
    int x, sum=0;
    while (src.pop(x))
        sum += x;
    printf("%d\n", sum);
}

int main() {
    // Arguments are capacities
    Queue<int> numbers(7), counts(4);
    const int n_producers = 1;
    const int n_counters = 1;

    // Start producers before consumers
    std::vector<std::thread> producers;
    for (int i=0; i < n_producers; ++i)
        producers.emplace_back(produce, output(numbers), 100);
    
    // Continue in topological order
    std::vector<std::thread> counters;
    for (int i=0; i < n_counters; ++i)
        counters.emplace_back(count, input(numbers), output(counts));

    // Sinks go last
    std::thread printer(print_sum, input(counts));

    // Now join in the same order
    for (int i=0; i < n_producers; ++i) producers[i].join();
    for (int i=0; i < n_counters; ++i) counters[i].join();
    printer.join();
}
