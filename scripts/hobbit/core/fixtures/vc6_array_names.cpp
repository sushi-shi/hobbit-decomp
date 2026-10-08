struct item { int a; };
int global_array[2] = {1, 2};
const item const_global_array[2] = {{3}, {4}};
struct arrays {
    static int public_array[2];
    static const item public_const_array[2];
protected:
    static int protected_array[2];
private:
    static const item private_array[2];
};
int arrays::public_array[2] = {5, 6};
const item arrays::public_const_array[2] = {{7}, {8}};
int arrays::protected_array[2] = {9, 10};
const item arrays::private_array[2] = {{11}, {12}};
const item* keep_const_array() {return const_global_array;}
