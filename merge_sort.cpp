#include "merge_sort.h"
#include <chrono>
#include <assert.h>
#include <math.h>
#include <iostream>

using namespace DISTRIBUTED::SORT;

// helper

inline void build_node(Node* node, int left, int right){
    constexpr uint8_t TEMP_ARR_BUFFER = 8;

    // writing values
    node->left = left;
    node->right = right;
    node->is_leaf = right-left <= 1;
    node->sort_snapshot = 0;
    node->temp_arr_snapshot = 0;
    node->temp_arr = new int[(right-left)+TEMP_ARR_BUFFER];
}

// private functions

void SortTree::calculate_leaves_and_size(int arr_size)
{
    // calculation
    int leaves = (arr_size>>1)+(arr_size&1);
    int size = (leaves<<1) - 1;

    // set values in class
    this->num_leaves = leaves;
    this->num_nodes = size;
};

// leaves logic 

// main function which fills leaves 
// will handle all logic and extra helper functions
void SortTree::fill_leaves(){
    // build root
    build_node(&this->nodes[1],0,this->max_arr_size-1);

    // checking underflow
    int bottom_lvl_size = 1<<(std::__bit_width(this->num_nodes)-1);
    int underflow = bottom_lvl_size - this->num_leaves;

    // get the start of our end leaf
    // (adjust in odd index)
    int start_end_leaf = (this->max_arr_size-1) - ((this->max_arr_size-1)&1);

    // fill since we have no overflow
    if (underflow <= 0){
        // start_adsjt 0, start_range 0, end_range, has end leaf
        fill_leaves_helper(
            0,0, this->num_leaves, true);
        return;
    }

    int st_adjst;
    if(underflow==1){
        st_adjst = start_end_leaf;
    }else{
        st_adjst = start_end_leaf - ((underflow-1)<<1);
    }

    // build underflow leaves
    fill_leaves_helper(
        st_adjst,
        0,
        underflow,
        true);
    
    // fill the rest of the leaves
    fill_leaves_helper(
        0,
        underflow,
        this->num_leaves,
        false);
};
    
// merges the leaves once they have be placed inside the vector
void SortTree::merge_leaves_helper(){
    for(int idx=this->num_nodes-this->num_leaves; idx>0; idx--){
        int left = this->nodes[(idx<<1)].left;
        int right = this->nodes[(idx<<1)+1].right;

        build_node(&this->nodes[idx],left,right);
    }
};

// does the necessary calculations to place are leaves in the proper spots
void SortTree::fill_leaves_helper(
    int start_adjustment, 
    int start_range, 
    int end_range, 
    bool has_last_leaf)
{
    for(int i=start_range; i<end_range; i++)
    {
        int leaf_idx = (this->num_nodes - this->num_leaves)+i+1;
        int a = start_adjustment;
        int b = start_adjustment+1;

        if(has_last_leaf && i==end_range-1 && this->max_arr_size & 1 == 1){
            b -= 1;
        }

        assert(a<=b && b-a <= 1);

        // build node
        build_node(&this->nodes[leaf_idx],a,b); 

        start_adjustment += 2;
    }
}; 
    
 // gets state for the next operation of our algorithm given the current node
STATE SortTree::get_state(int node_idx){
    int curr_iter = this->sort_iteration;

    if(this->nodes[node_idx].is_leaf){
        return STATE::SORT_NODE;
    }

    if(this->nodes[(node_idx<<1)].sort_snapshot != curr_iter){
        return STATE::GO_LEFT;
    }

    if(this->nodes[(node_idx<<1)+1].sort_snapshot != curr_iter){
        return STATE::GO_RIGHT;
    }

    return STATE::SORT_NODE;
};

// each thread sorting logic
void SortTree::per_thread_sort(int start_node_idx)
{
while(true) // eventually get this to kill when deconstructed
{
    // std::cout<<"thread starting on "<<start_node_idx<<std::endl;
    // wait
    // printf("thread %d spawning and inside function\n",start_node_idx);
    auto& sort_vec_ref = *sort_vec; // grabbing our reference
    // std::mutex tst;
    // std::unique_lock<std::mutex> lck(tst);

    // prev_sort iter is given before our loop
    // checks to see if we have a spurious wake (lamada)
    // cv.wait(lck,[this,start_node_idx](){
    //     // snap iter is behind next iter
    //     printf("thread %d | %d waiting...\n",start_node_idx,thread_sort_iter);

    //     return thread_sort_iter < this->sort_iteration; 
    // });

    // spin
    while(thread_sort_iter>=this->sort_iteration){
        // std::cout<<"waiting\n";
    }

    // begin sort 
    // std::cout<<"going to sort as a thread!"<<std::endl;
    int curr_idx = start_node_idx;
    while(this->nodes[1].sort_snapshot < this->sort_iteration){
        Node* curr_node = &this->nodes[curr_idx]; 
        // std::cout<<"current node "<<curr_idx<<std::endl;

        STATE state = this->get_state(curr_idx);
        if(state==STATE::GO_LEFT){
            curr_idx = curr_idx<<1;
            continue;
        }

        if(state==STATE::GO_RIGHT){
            curr_idx = (curr_idx<<1)+1;
            continue;
        }

        // attempting sort

        // check if we've already sorted this node
        if(curr_node->sort_snapshot == this->sort_iteration){
            curr_idx = (curr_idx>>1); // go up
            continue;
        }

        // printf("thread: %d | attempting to sort node: %d |\n",start_node_idx,curr_idx);
        // LOCK_FREE VERSION
        
        // int mid;
        // // since we don't have an algorithm on a leaf we can just take the true mid
        // if(curr_node->is_leaf){
        //     mid = (curr_node->left + curr_node->right)>>1;
        // }else{
        //     // we are in an internal node
        //     // we pull left node's max value since that is our mid
        //     mid = this->nodes[(curr_idx<<1)].right;
        // }

        // // applying typical merge sort
        // int i = curr_node->left;
        // int j = mid+1;
        // int k = 0;

        // // std::cout<<sort_iteration<<" "<<curr_node->temp_arr_snapshot<<std::endl;
        // // ensure our sort iteration is correct
        // while(i<=mid&& j<=curr_node->right && curr_node->temp_arr_snapshot<sort_iteration)
        // {
        //     int i_inc = 0;
        //     int j_inc = 0;
        //     int new_val = 0;

        //     if(sort_vec_ref[i] <= sort_vec_ref[j]){
        //         new_val = sort_vec_ref[i];
        //         i_inc = 1;
        //     }else{
        //         new_val = sort_vec_ref[j];
        //         j_inc = 1;
        //     }   

        //     // someone already filled our temp_arr
        //     if(curr_node->temp_arr_snapshot<sort_iteration){
        //         // std::cout<<"someone already sorted\n";
        //         //write changes
        //         curr_node->temp_arr[k] = new_val;
        //         i += i_inc;
        //         j += j_inc;
        //         k++;
        //     }
        // }

        // while(i<=mid && curr_node->temp_arr_snapshot<sort_iteration){
        //     int new_val = sort_vec_ref[i];

        //     // no on has filled our temp array our fetch is valid
        //     if(curr_node->temp_arr_snapshot<sort_iteration){
        //         // write changes
        //         curr_node->temp_arr[k] = new_val;
        //         i+=1;
        //         k++;
        //     }
        // }

        // while(j<=curr_node->right && curr_node->temp_arr_snapshot<sort_iteration){
        //     int new_val = sort_vec_ref[j];

        //     // someone already filled our temp_arr
        //     if(curr_node->temp_arr_snapshot<sort_iteration){ 
        //         //write changes
        //         curr_node->temp_arr[k] = new_val;
        //         j+=1;
        //         k++;
        //     }
        // }

        // curr_node->temp_arr_snapshot = sort_iteration;
        // int temp_arr_size = (curr_node->right-curr_node->left)+1;

        // // std::cout<<"done sorting "<<k<<"\n";
        // // for(int i=0; i<temp_arr_size; i++){
        // //     std::cout<<curr_node->temp_arr[i]<<", ";
        // // }
        // // std::cout<<"temp arr\n";

        // for(int i=0; i<temp_arr_size && curr_node->sort_snapshot<sort_iteration;i++){
        //     (*sort_vec)[curr_node->left+i] = curr_node->temp_arr[i]; 
        // }

        // curr_node->sort_snapshot = this->sort_iteration;
        // curr_idx = (curr_idx>>1);

        // // taking control of the node
        // // LOCK VERSION
        curr_node->mtx.lock();
        
        if(curr_node->sort_snapshot == this->sort_iteration){
            curr_idx = (curr_idx>>1); // go up
            curr_node->mtx.unlock();
            continue;
        }

        // now doing merge sort
        uint8_t BUFFER = 8;
        // std::vector<int> temp_arr;
        std::vector<int> temp_arr((curr_node->right-curr_node->left)+BUFFER);
        auto& sort_vec_ref = *sort_vec;

        int mid;
        // since we don't have an algorithm on a leaf we can just take the true mid
        if(curr_node->is_leaf){
            mid = (curr_node->left + curr_node->right)>>1;
        }else{
            // we are in an internal node
            // we pull left node's max value since that is our mid
            mid = this->nodes[(curr_idx<<1)].right;
        }

        // applying typical merge sort
        int i = curr_node->left;
        int j = mid+1;
        int k = 0;

        while(i<=mid && j<=curr_node->right){
            if(sort_vec_ref[i] <= sort_vec_ref[j]){
                // temp_arr.push_back(sort_vec->at(i));
                temp_arr[k] = sort_vec_ref[i];
                i += 1;
                k++;
            }else{
                // temp_arr.push_back(sort_vec->at(j));
                temp_arr[k] = sort_vec_ref[j];
                j+=1;
                k++;
            }
        }

        while(i<=mid){
            // temp_arr.push_back(sort_vec->at(i));
            temp_arr[k] = sort_vec_ref[i];
            i+=1;
            k++;
        }

        while(j<=curr_node->right){
            // temp_arr.push_back(sort_vec->at(j));
            temp_arr[k] = sort_vec_ref[j];
            j+=1;
            k++;
        }


        for(int i=0; i<temp_arr.size();i++){
            (*sort_vec)[curr_node->left+i] = temp_arr[i]; 
        }

        curr_node->sort_snapshot = this->sort_iteration;
        curr_node->mtx.unlock();
        curr_idx = (curr_idx>>1);
    }

    thread_sort_iter = this->sort_iteration; // upgrade sort iter
}
};


// public functions

SortTree::SortTree(int max_array_size, int num_threads) 
: max_arr_size(max_array_size), sort_iteration(0), num_threads(num_threads)
{
    // validate input
    assert(num_threads > 0 && max_array_size > 0);

    // init leaves and size
    this->calculate_leaves_and_size(max_arr_size);

    // init our nodes vector
    this->nodes = std::vector<Node>(this->num_nodes+1); // +1 for one index (null -> 0)

    this->fill_leaves();
    this->merge_leaves_helper();

    // int track = 0;
    // for(const Node& n : this->nodes){
    //     printf("id: %d | left: %d | right: %d | is_leaf: %d\n",
    //     track,n.left,n.right,n.is_leaf);
    //     track++;
    // }
    int iter = 0;
    int inc = 1;
    if(num_threads>=num_leaves){
        iter = num_leaves; // num_leaves is our min
    }else{
        // num_leaves > num_threads
        iter = num_threads;
        inc = num_leaves/num_threads; // int division floor()
    }

    printf("spawning %d threads\n",iter);
    int track = 0;
    for(int i=0; i<iter; i++){
        int start_leaf = (num_nodes-num_leaves)+track+1;
        // std::cout<<"spawning thread "<<start_leaf<< " leaves "<<num_leaves<<std::endl;
        track += inc;
        this->threads.emplace_back(
            std::thread(&SortTree::per_thread_sort, this, start_leaf));
    }
    // for(int id=0; id<num_threads; id++){
    //     int start_leaf = (num_nodes-num_leaves)+id+1;
    //     this->threads.emplace_back(
    //         std::thread(&SortTree::per_thread_sort, this, start_leaf));
    // }
};

void SortTree::sort(std::vector<int>* vec){
    this->sort_vec = vec;
    this->sort_iteration++;
    this->cv.notify_all();

    // wait until sorted
    while(nodes[1].sort_snapshot != sort_iteration){}
};

/*
bool process
while(true){
    if(try_lock())
        process = true
        break
    
    if(node.process_snapshot<curr_snapshot) // spurious fail
        retry
    
    process = false // someone has the node skip over
}

// lockfree
shared s_temp_arr

while(i < mid){

}
*/