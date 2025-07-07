import threading
import sys

INPUT_SIZE = 4 # input size
INPUT_ARR = [INPUT_SIZE-i for i in range(INPUT_SIZE)] # generating unsorted array
MAX_THREADS = 1

STATE_SORT_NODE = 1
STATE_GO_LEFT = 2
STATE_GO_RIGHT = 3
STATE_GO_UP = 4

def calculate_leaves_and_size(arr_size):
    """
    returns (leaves,size)
    """
    leaves = (arr_size>>1)+(arr_size&1) # divide by 2 and ceil
    size = (leaves<<1) - 1 
    return (leaves,size)


class Node:
    def __init__(self,left,right):
        self.left = left
        self.right = right
        self.is_leaf = right-left <= 1
        self.mtx = threading.Lock()
        self.sort_iter = -1

class SortTree:
    def __init__(self,max_size):
        # print("building sort tree")
        # self.leaf_arr = []
        self.sort_iter = 0 #identifer for current sort operation
        self.max_arr_size = max_size # max size of our array can be
        # print(f"max_arr_size: {max_size}")
        
        # buidling our sort tree 
        self.leaves, self.nodes = calculate_leaves_and_size(self.max_arr_size)

        # print(f"leafs: {self.leafs} | nodes: {self.nodes}")
        # print(f"Leaves: {self.leafs} Nodes: {self.nodes}")
        # self.node_arr = [Node(-1,-1),Node(0,self.max_arr_size-1)] # 0 - NULL node, 1 - Root
        self.private_fill_leaves()
        self.private_merge_leaves()

    def private_fill_leaves(self):
        # helper to fill our leaves array
        def fill_leaves_helper(st_adjst, start_range, end_range,has_last_leaf=True):
            for i in range(start_range,end_range):
                leaf_idx = (self.nodes - self.leaves)+i+1
                a,b = st_adjst, st_adjst+1
            
                # last leaf and we have an odd number of elements
                # we must adjust for our leaf size
                if has_last_leaf and i==end_range-1 and self.max_arr_size & 1 == 1:
                    b -= 1
                assert a<=b and b-a<=1

                self.node_arr[leaf_idx] = Node(a,b)
                st_adjst += 2

        self.node_arr = [Node(-1,-1) for _ in range(self.nodes+1)] # filling with NULL nodes
        self.node_arr[1] = Node(0,self.max_arr_size-1) # root (1 idx)


        #checking for underflow
        bottom_lvl_size = 1<<(self.nodes.bit_length()-1)
        underflow = bottom_lvl_size - self.leaves
        st_end_leaf = (self.max_arr_size-1) - ((self.max_arr_size-1)&1)

        # print("undfl, botlvlsz, nodes, bitlength",underflow,bottom_lvl_size,self.nodes,self.nodes.bit_length(),self.leaves)
        if underflow <= 0:
            # print("doing regular fill and thats it")
            # do regular fill
            fill_leaves_helper(
                st_adjst=0,
                start_range=0,
                end_range=self.leaves)
            return

        if underflow==1:
            st_adjst = st_end_leaf
        else:
            st_adjst = st_end_leaf - ((underflow-1)<<1)

        # print(f"[0,{underflow}],[{underflow},{self.leaves}],adjst: {st_adjst} | st_endleaf: {st_end_leaf}")
        # print(self.nodes-self.leaves+1)
        #build underflow leaves 
        fill_leaves_helper(
            st_adjst,
            start_range=0,
            end_range=underflow)

        # fill the rest of the leaves
        fill_leaves_helper(
            st_adjst=0,
            start_range=underflow,
            end_range=self.leaves,
            has_last_leaf=False)

    def private_merge_leaves(self):
        for idx in range(self.nodes-self.leaves,0,-1):
            left = self.node_arr[(idx<<1)]
            right = self.node_arr[(idx<<1)+1]

            self.node_arr[idx] = Node(left.left,right.right)        

    def sort(self, arr):
        self.arr = arr # grab reference to the array
        self.sort_iter += 1 # setting new sort window

        # grab from pool and run threads
        threads = []
        for i in range(min(MAX_THREADS,self.leaves)):
            start_leaf = (self.nodes-self.leaves)+i+1
            threads.append(
                threading.Thread(target=self.private_sort,args=(start_leaf,))
            )
            threads[-1].start()

        # join on threads
        for t in threads: 
            t.join()
 
    def private_get_state(self,idx):
        """
        takes in current node and outputs action state which should be done
        """
        curr_iter = self.sort_iter

        # is leaf
        if self.node_arr[idx].is_leaf: 
            return STATE_SORT_NODE 
        
        # left = self.node_arr[2*idx]
        # right = self.node_arr[2*idx+1]

        if self.node_arr[idx<<1].sort_iter != curr_iter:
            return STATE_GO_LEFT
        if self.node_arr[(idx<<1)+1].sort_iter != curr_iter:
            return STATE_GO_RIGHT

        return STATE_SORT_NODE 

    # concurrent per thread sorting algoirthm
    def private_sort(self, start_idx):
        # print("spawning thread on",start_idx)
        curr_idx = start_idx
        while self.node_arr[1].sort_iter != self.sort_iter:
            curr_node = self.node_arr[curr_idx]
            # print("sorting",self.arr[curr_node.left:curr_node.right+1])
            state = self.private_get_state(curr_idx)
            if state == STATE_GO_LEFT:
                # print("going left")
                curr_idx = curr_idx << 1
                continue

            if state == STATE_GO_RIGHT:
                # print("going right")
                curr_idx = (curr_idx<<1)+1
                continue
            
            # print("attempting actual sort")
            # current node got sorted
            if curr_node.sort_iter == self.sort_iter:
                # print("already sorted going up")
                curr_idx = (curr_idx>>1)
                continue

            curr_node.mtx.acquire()

            # current node got sorted
            if curr_node.sort_iter == self.sort_iter:
                # print("locked: already sroted going up")
                curr_idx = (curr_idx>>1)
                curr_node.mtx.release()
                continue

            # print("now sorting via merge")
            # if curr_idx == 1:
                # print("now sorting root")
                # print(self.arr,curr_node.left,curr_node.right)
            # attempt to sort
            tmp_arr = []
            arr = self.arr

            # since we don't have an algorithm we can do whatever
            if curr_node.is_leaf: 
                mid = (curr_node.left + curr_node.right)>>1
            else: # we are an internal node
                # pull left node's max value since that is our mid
                mid = self.node_arr[(curr_idx<<1)].right 

            # else: # we must follow the same algoirthm as our desent
            #     mid = self.private_calculate_mid_point(curr_node.left,curr_node.right)
#
            i = curr_node.left
            j = mid+1
            # print(i,mid,j)

            # print("curr node", curr_node.left, curr_node.right)
            while i<=mid and j<=curr_node.right:
                # print(arr[i],arr[j])
                if arr[i] <= arr[j]:
                    tmp_arr.append(arr[i])
                    i += 1
                else:
                    tmp_arr.append(arr[j])
                    j += 1

            while i<=mid:
                tmp_arr.append(arr[i])
                i+=1

            while j<=curr_node.right:
                tmp_arr.append(arr[j])
                j+=1

            for idx, val in enumerate(tmp_arr):
                arr[curr_node.left+idx] = val
            
            # print("tmp array",tmp_arr)
            curr_node.sort_iter = self.sort_iter # upgrade sort iter
            curr_node.mtx.release() # release mutex
            curr_idx = (curr_idx>>1) # go up to parent


def is_sorted():
    for i in range(1,INPUT_SIZE):
        if INPUT_ARR[i-1] > INPUT_ARR[i]:
            # print("not sorted")
            # print(INPUT_ARR[i-1],INPUT_ARR[i],i)
            return False
    return True

if len(sys.argv) != 3:
    c,v = 0,1
    for i in range(4,10**5):
        INPUT_SIZE = i# input size
        INPUT_ARR = [INPUT_SIZE-i for i in range(INPUT_SIZE)] # generating unsorted array
        x = SortTree(INPUT_SIZE)
        assert is_sorted()==False, i
        x.sort(INPUT_ARR)
        # print(INPUT_ARR)
        assert is_sorted()==True, i
        if c==1000:
            print("passed:",c*v)
            c = 0
            v += 1
        c+=1
    # print("passed,",i)
    print("passed tests")
    exit()

MAX_THREADS = int(sys.argv[1])
INPUT_SIZE = int(sys.argv[2])
INPUT_ARR = [INPUT_SIZE-i for i in range(INPUT_SIZE)] # generating unsorted array

print(f"running sort-tree with {MAX_THREADS} threads and {INPUT_SIZE} elements")
x = SortTree(INPUT_SIZE)
# for i,n in enumerate(x.node_arr):
#     print(i,n.left,n.right,n.is_leaf)

assert is_sorted() == False
x.sort(INPUT_ARR)
assert is_sorted() == True
print("passed!")

INPUT_ARR[3] = INPUT_ARR[0]
print(INPUT_ARR)
assert is_sorted() == False
x.sort(INPUT_ARR)
assert is_sorted() == True
print("passed twice")
print(INPUT_ARR)
#
