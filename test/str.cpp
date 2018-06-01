#include <thread>
#include <iostream>
#include <sstream>
#include <string>

#include "bpcqueue.hpp"


using namespace bpcqueue;
using str = std::string;

void produce(Output<str> dst, int n) {
    for (int i=0; i<n; ++i) {
        std::stringstream ss;
        ss << i << "Lala";
        dst.push(std::move(ss.str()));
    }
}


void count(Input<str> src, Output<int> dst) {
    str x;
    int count = 0;
    while (src.pop(x)) {
        ++count;
    }
    dst.push(count);
}

void print_sum(Input<int> src) {
    int i;
    int sum = 0;
    while (src.pop(i)) {
        sum += i;
    }
    std::cout << sum << '\n';
}


int main() {
	Queue<str> q1(2000);
	Queue<int> q2(10);
        std::thread p1(produce, output(q1), 1000);
        std::thread p2(produce, output(q1), 1000);
        std::thread c1(count, input(q1), output(q2));
        std::thread c2(count, input(q1), output(q2));
        std::thread c3(count, input(q1), output(q2));
        std::thread sum(print_sum, input(q2));
        p1.join();
        p2.join();
        c1.join();
        c2.join();
        c3.join();
        sum.join();
}
