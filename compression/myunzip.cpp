#include <stdio.h>
#include <stdint.h>

class Node {
public:
    bool isLeaf; 
    uint8_t byte;
    Node* left;
    Node* right;

    Node(uint8_t byte) {
        isLeaf = true;
        this->byte = byte;
        left = nullptr;
        right = nullptr;
    }

    Node() {
        isLeaf = false;
        this->left = nullptr;
        this->right = nullptr;
    }

    ~Node() {
        if (left) {
            delete left;
        }
        if (right) {
            delete right;
        }
    }
};

class ByteBufferReader {
public:
    uint8_t byte;
    uint8_t index;
    FILE* file;

    ByteBufferReader(FILE* file) {
        index = 0;
        this->file = file;
    }

    bool readBit() {
        if (!index) {
            byte = fgetc(file);
        }
        bool result = byte & (1 << index);
        index = (index + 1) % 8;
        return result;
    }

    void next() {
        index = 0;
    }
};

int main(int argc, char* argv[]) {
    if (argc < 3) {
        return 1;
    }

    FILE* input_file = fopen(argv[1], "r");
    int num_entries = fgetc(input_file) + 1;
    int num_bytes = 0;
    for (int i=0; i<sizeof(num_bytes); i++) {
        num_bytes += fgetc(input_file) << (8 * i);
    }

    Node* heap = new Node();
    ByteBufferReader* reader = new ByteBufferReader(input_file);
    for (int i=0; i<num_entries; i++) {
        Node* current = heap;
        uint8_t byte = fgetc(input_file);
        uint8_t num_bits = fgetc(input_file);
        for (int j=0; j<num_bits-1; j++) {
            if (reader->readBit()) {
                if (!current->right) {
                    current->right = new Node();
                }
                current = current->right;
            } else {
                if (!current->left) {
                    current->left = new Node();
                }
                current = current->left;
            }
        }
        if (reader->readBit()) {
            current->right = new Node(byte);
        } else {
            current->left = new Node(byte);
        }
        reader->next();
    }

    FILE* output_file = fopen(argv[2], "w");
    for (int i=0; i<num_bytes; i++) {
        Node* current = heap;
        while (!current->isLeaf) {
            if(reader->readBit()) {
                current = current->right;
            } else {
                current = current->left;
            }
        }
        fputc(current->byte, output_file);
    }

    fclose(input_file);
    fclose(output_file);
    delete heap;
    delete reader;
}
