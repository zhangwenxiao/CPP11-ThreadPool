#include <iostream>
#include <vector>
#include <string>
#include <future>
#include <thread>
#include <chrono>

#include "ThreadPool.h"

int main()
{
    ThreadPool pool(4);

    std::vector<std::future<std::string>> results;

    for(int i = 0; i < 8; ++i) {
        results.emplace_back(
            pool.enqueue([i] {
                printf("hello %d\n", i);
                //std::cout << "hello " << i << std::endl;
                std::this_thread::sleep_for(
                        std::chrono::seconds(1));
                printf("world %d\n", i);
                //std::cout << "world " << i << std::endl;

                return std::string("---thread ") + 
                    std::to_string(i) + 
                    std::string(" finished.---");
            })
        );
    }

    for(auto && result: results)
        printf("%s \n", result.get().data());
        //std::cout << result.get() << ' ' << std::endl;

    return 0;
}

