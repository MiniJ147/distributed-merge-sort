#ifndef MERGE_SORT_H 
#define MERGE_SORT_H 
#include <thread>
#include <mutex>
#include <vector>
#include <condition_variable>

namespace DISTRIBUTED{
namespace SORT{

inline thread_local int thread_sort_iter = 0;

enum class STATE{
    SORT_NODE,
    GO_LEFT,
    GO_RIGHT,
    GO_UP
};

struct Node {
    int* temp_arr; // shared temp array
    int left = -1; // left array bound [inclusive]
    int right = -1; // right array idx bound [inclusive]
    int sort_snapshot = 0; // nodes last time sorted
    int temp_arr_snapshot = 0; // temp array snapshot
    int is_leaf = 0; // boolean (is leaf)
    std::mutex mtx;
};

class SortTree{
private:
    int sort_iteration = 0;
    int max_arr_size;
    int num_leaves;
    int num_nodes;

    int num_threads;

    // sync logic for first call
    std::condition_variable cv;
    std::mutex cv_mtx;
    bool ready = false;


    std::vector<std::thread> threads;
    std::vector<Node> nodes;
    std::vector<int>* sort_vec;

    // math helper
    // will write num_leaves and num_nodes with the necessary nodes in order to build
    // our sort tree
    void calculate_leaves_and_size(int arr_size);

    // leaves logic 

    // main function which fills leaves 
    // will handle all logic and extra helper functions
    void fill_leaves();
    
    // merges the leaves once they have be placed inside the vector
    void merge_leaves_helper();

    // does the necessary calculations to place are leaves in the proper spots
    void fill_leaves_helper(
        int start_adjustment, 
        int start_range, 
        int end_range, 
        bool has_last_leaf); 
    
    // gets state for the next operation of our algorithm given the current node
    STATE get_state(int node_idx);

    // each thread sorting logic
    void per_thread_sort(int start_node_idx);
public:
    SortTree(int max_array_size, int num_threads);
    void sort(std::vector<int>* vec);
};
}}

#endif

