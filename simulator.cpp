#include <iostream>
#include <chrono>
#include <thread>
#include <assert.h>
#include "merge_sort.h"
#include "default_merge_sort.h"

int main(int argc, char* argv[]){
    std::cout<<"welcome to the merge-sort simulator\n";
    std::cout<<"args: {thread}, {input_size}"<<std::endl;
    assert(argc==3);
    int threads = std::atoi(argv[1]);
    int input = std::atoi(argv[2]);
    std::cout<<"running with "<<threads<<" threads and "<<input<<" elements"<<std::endl;

    std::vector<int> nums;
    for(int i=input; i>0; i--){
        nums.push_back(i);
    }

    if(threads <= 0){
        auto start = std::chrono::high_resolution_clock::now();

        mergeSort(nums,0,nums.size()-1);

        auto end = std::chrono::high_resolution_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end-start);
        std::cout<<"Threads: "<<threads<<"\tInput: "<<input<<"\tTotal Time: "<<duration.count()<<"ms\n";

        for(int i=1;i<nums.size();i++){
            assert(nums[i-1]<=nums[i]);
        }
        return 0;
    }

    DISTRIBUTED::SORT::SortTree x(input,threads);

    auto start = std::chrono::high_resolution_clock::now();

    x.sort(&nums);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end-start);
    std::cout<<"Threads: "<<threads<<"\tInput: "<<input<<"\tTotal Time: "<<duration.count()<<"ms\n";

    for(int i=1;i<nums.size();i++){
        // std::cout<<nums[i-1]<<", "<<nums[i];
        assert(nums[i-1]<=nums[i]);
    }
    std::cout<<std::endl;

    // std::cout<<"dont crash yet\n";

    return 0;
}
