#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define BYTE_SIZE (256)

// Warning I've never wrote more schizo-code than this in my life

class Node {
public:
    bool isLeaf; 
    uint8_t byte;
    int times;
    Node* left;
    Node* right;

    Node(uint8_t byte, int times) {
        isLeaf = true;
        this->byte = byte;
        this->times = times;
        left = nullptr;
        right = nullptr;
    }

    Node(Node* left, Node* right) {
        isLeaf = false;
        times = left->times + right->times;
        this->left = left;
        this->right = right;
    }

    ~Node() {
        if (left) {
            delete left;
        }
        if (right) {
            delete right;
        }
    }

    void print() {
        if (isLeaf) {
            printf("Leaf c=%c/%u t=%d\n", byte, byte, times);
        } else {
            printf("Branch p=%p t=%d Left:\n", this, times);
            left->print();
            printf("Branch p=%p t=%d Right:\n", this, times);
            right->print();
        }
    }
};

class MinQueue {
public:
    Node* array[BYTE_SIZE];
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
            if (node->times >= array[i]->times) {
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

class CodeVector {
public:
    bool array[BYTE_SIZE];
    int size;

    CodeVector() {
        size = 0;
    }

    void push(bool isRight) {
        array[size++] = isRight;
    }

    bool pop() {
        return array[--size];
    }

    char* getCode() {
        char* result = (char*)malloc(size + 1);
        for (int i=0; i<size; i++) {
            if (array[i]) {
                result[i] = '1';
            } else {
                result[i] = '0';
            }
        }
        result[size] = '\0';
        
        return result;
    }
};

void fputMapEntry(FILE* file, uint8_t byte, char* code) {
    fputc(byte, file); // Write byte code is for

    int length = strlen(code);
    fputc(length, file); // Write number of bits in code
    
    while (*code) {
        uint8_t byte = 0;
        for (int i=0; i<8 && *code; i++) {
            byte |= (*code - '0') << i;
            code++;
        }
        fputc(byte, file);
    }
}

void initCodeMapFromNodeRecursive(Node* node, CodeVector* path, char* map[]) {
    if (node->isLeaf) {
        map[node->byte] = path->getCode();
    } else {
        path->push(false);
        initCodeMapFromNodeRecursive(node->left, path, map);
        path->push(true);
        initCodeMapFromNodeRecursive(node->right, path, map);
    }
    path->pop();
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        return 1;
    }

    int map[BYTE_SIZE];
    for (int i=0; i<BYTE_SIZE; i++) {
        map[i] = 0;
    }
    FILE* file = fopen(argv[1], "r");
    for (int byte = fgetc(file); byte != EOF; byte = fgetc(file)) {
        map[byte]++;
    }
    fclose(file);

    MinQueue* queue = new MinQueue();
    int num_entries = 0;
    for (int i=0; i<BYTE_SIZE; i++) {
        if (map[i]) {
            queue->push(new Node(i, map[i]));
            num_entries++;
        }
    }

    while (queue->size > 1) {
        queue->push(new Node(queue->pop(), queue->pop()));
    }
    Node* heap = queue->pop();
    delete queue;
    
    

    file = fopen(argv[2], "w");
    fputc(num_entries - 1, file); // Write map_size

    for (int i=0; i<4; i++) {
        fputc(0, file); // Make space for data_size;
    }

    CodeVector* path = new CodeVector();
    char* codeMap[BYTE_SIZE];
    for (int i=0; i<BYTE_SIZE; i++) {
        codeMap[i] = 0;
    }
    initCodeMapFromNodeRecursive(heap, path, codeMap);
    for (int i=0; i<BYTE_SIZE; i++) {
        if (codeMap[i]) {
            fputMapEntry(file, i, codeMap[i]);
            printf("%c/%d: %s\n", i, i, codeMap[i]);
        }
    }
    fclose(file);

    for (int i=0; i<BYTE_SIZE; i++) {
        if (codeMap[i]) {
            free(codeMap[i]);
        }
    }
    delete path;
    delete heap;
    printf("EXit!\n");
}