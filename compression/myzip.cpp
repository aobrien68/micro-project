#include <stdio.h>

class Node {
public:
    int data;
    Node* left;
    Node* right;

    Node(int data) {
        this->data = data;
        left = nullptr;
        right = nullptr;
    }

    Node(Node* left, Node* right) {
        data = left->data + right->data;
        this->left = left;
        this->right = right;
    }
};

class MinQueue {
public:
    Node* array[256];
    int size;

    MinQueue() {
        size = 0;
    }

    void insert(Node* node, int index) {
        for (int i=size; i>index; i--) {
            array[i] = array[i - 1];
        }
        array[index] = node;
        size++;
    }

    void push(Node* node){
        for (int i=0; i<size; i++) {
            if (node->data >= array[i]->data) {
                insert(node, i);
                return;
            }
        }
        array[size++] = node;
    }

    Node* pop() {
        return array[--size];
    }
};

int main(int argc, char* argv[]) {
    // if (argc < 2) {
    //     return 1;
    // }

    // int map[256];
    // for (int i=0; i<256; i++) {
    //     map[i] = 0;
    // }
    // FILE* file = fopen(argv[1], "r");
    // for (int byte = fgetc(file); byte != EOF; byte = fgetc(file)) {
    //     map[byte]++;
    // }
    // fclose(file);

    Node* n1 = new Node(8);
    Node* n2 = new Node(17);
    Node* n3 = new Node(14);
    Node* n4 = new Node(n1, n3);
    MinQueue* queue = new MinQueue();

    queue->push(n3);
    queue->push(n1);
    queue->push(n4);
    queue->push(n2);

    for (int i=0; i<4; i++) {
        printf("%d\n", queue->pop()->data);
    }

    delete n1;
    delete n2;
    delete n3;
    delete n4;
    delete queue;
}