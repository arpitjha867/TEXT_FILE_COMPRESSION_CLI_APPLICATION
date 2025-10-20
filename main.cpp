#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <queue>
#include <vector>
#include <cmath> // For decompression ratio calculation

using namespace std;

// --- GLOBAL DATA STRUCTURES ---
unordered_map<char, int> characterFrequency;
unordered_map<char, string> charCode;
string currentCode;
string filePath;
string compressFilePath;
float TotalCharCnt = 0;
float compressedByteCnt = 1; // Start at 1 to account for the padding byte in header


// --- HUFFMAN NODE STRUCTURE ---
struct Node {
    char character;
    int frequency;
    Node* left;
    Node* right;
};

// --- MIN-HEAP COMPARATOR ---
struct Compare {
    bool operator()(Node* n1, Node* n2) {
        return n1->frequency > n2->frequency;
    }
};

priority_queue<Node*, vector<Node*>, Compare> minHeap;

// --- HELPER FUNCTIONS ---

void print_alternate_banner() {
    cout << "\n";
    cout << "+-------------------------------------------------+\n";
    cout << "|             @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@     |\n";
    cout << "|             @     T E X T   C O D E C     @     |\n";
    cout << "|             @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@     |\n";
    cout << "|              Huffman Encoding/Decoding          |\n";
    cout << "+-------------------------------------------------+\n";
    cout << "\n";
}

// Function to read the input file and build the frequency map/minHeap
void read_file() {
	cout<<"Please enter the file path with respect to current directory : ";
	cin >> filePath;
	cout << endl;
    cout << "\n--- Analyzing " << filePath << " ---\n";
    ifstream file(filePath, ios::binary);
    if (!file) {
        cerr << "ERROR! Could not open input file "<<filePath << endl;
        return;
    }
    char ch;
    while (file.get(ch)) {
        if (ch != '\r') characterFrequency[ch]++;
    }
    
    for (const auto& pair : characterFrequency) {
        cout << "KEY: '" << pair.first << "' | FREQUENCY: " << pair.second << endl;
        TotalCharCnt += pair.second;
        Node* newNode = new Node();
        newNode->character = pair.first;
        newNode->frequency = pair.second;
        newNode->left = nullptr;
        newNode->right = nullptr;
        minHeap.push(newNode);
    }
    cout << "TOTAL CHARACTERS: " << TotalCharCnt << endl;
    file.close();
}

// Function to build the Huffman Tree from the min-heap
Node* build_huffman_tree() {
    while (minHeap.size() > 1) {
        Node* node1 = minHeap.top(); minHeap.pop();
        Node* node2 = minHeap.top(); minHeap.pop();

        Node* newInternalNode = new Node();
        newInternalNode->character = '\0';
        newInternalNode->frequency = node1->frequency + node2->frequency;
        newInternalNode->left = node1;
        newInternalNode->right = node2;
        minHeap.push(newInternalNode);
    }
    if (minHeap.empty()) return nullptr;
    return minHeap.top();
}

// Function to generate Huffman codes by traversing the tree
void generate_code(Node* currNode, string currCode){
    if(currNode == NULL) return;
    if(currNode->left == NULL && currNode-> right == NULL){
        charCode[currNode->character] = currCode;
        cout << "Code for '" << currNode->character << "': " << currCode << endl;
    }
    
    generate_code(currNode->left, currCode + "0");
    generate_code(currNode->right, currCode + "1");
}

// --- COMPRESSION HELPERS ---

// Helper function to write a single bit (0 or 1) to the stream
void writeBit(std::ofstream& outFile, unsigned char& buffer, int& bitCount, char bit) {
    if (bit == '1') {
        buffer |= (1 << (7 - bitCount)); // Set the bit at the correct position (MSB first)
    }
    
    bitCount++;

    if (bitCount == 8) {
        outFile.write(reinterpret_cast<const char*>(&buffer), 1);
        buffer = 0;
        bitCount = 0;
        compressedByteCnt += 1; // Update byte count
    }
}

// Function to write the Huffman Tree structure to the header using a bit stream
void WriteTreeStructure(Node* node, std::ofstream& outFile, unsigned char& buffer, int& bitCount) {
    if (node->left == nullptr && node->right == nullptr) {
        writeBit(outFile, buffer, bitCount, '1'); // Leaf Node: '1'
        
        // Write the 8-bit character data
        for (int i = 7; i >= 0; i--) {
            char bit = (node->character & (1 << i)) ? '1' : '0';
            writeBit(outFile, buffer, bitCount, bit);
        }
    } else {
        writeBit(outFile, buffer, bitCount, '0'); // Internal Node: '0'
        
        WriteTreeStructure(node->left, outFile, buffer, bitCount);
        WriteTreeStructure(node->right, outFile, buffer, bitCount);
    }
}

// --- DECOMPRESSION HELPERS ---

// Helper function to read a single bit from the stream
char readBit(std::ifstream& inFile, unsigned char& buffer, int& bitCount, bool& endOfFile) {
    if (bitCount == 0) {
        if (!inFile.read(reinterpret_cast<char*>(&buffer), 1)) {
            endOfFile = true;
            return '\0'; // Indicate failure
        }
        bitCount = 8;
    }
    
    // Check the bit at the current position (MSB first)
    char bit = (buffer & (1 << (bitCount - 1))) ? '1' : '0';
    bitCount--;
    return bit;
}

// Function to read the Huffman Tree structure from the header
Node* ReadTreeStructure(std::ifstream& inFile, unsigned char& buffer, int& bitCount, bool& endOfFile) {
    char indicator = readBit(inFile, buffer, bitCount, endOfFile);
    if (endOfFile) return nullptr;

    if (indicator == '1') {
        // Leaf Node
        Node* newNode = new Node();
        newNode->left = nullptr;
        newNode->right = nullptr;
        newNode->character = 0;

        // Read 8-bit character data
        for (int i = 7; i >= 0; i--) {
            char charBit = readBit(inFile, buffer, bitCount, endOfFile);
            if (endOfFile) { delete newNode; return nullptr; }
            if (charBit == '1') {
                newNode->character |= (1 << i);
            }
        }
        return newNode;
    } else {
        // Internal Node
        Node* newNode = new Node();
        newNode->character = '\0';
        newNode->left = ReadTreeStructure(inFile, buffer, bitCount, endOfFile);
        newNode->right = ReadTreeStructure(inFile, buffer, bitCount, endOfFile);
        
        if (newNode->left == nullptr || newNode->right == nullptr) {
            // Error or unexpected EOF during tree rebuild
            delete newNode;
            return nullptr;
        }
        return newNode;
    }
}

// --- MAIN FUNCTIONS ---

// Function to compress the file
void compress_file() {
    Node* root = minHeap.top();
    unsigned char current_byte = 0;
    int bit_count = 0;
    compressedByteCnt = 1; // Reset for calculation

    std::ofstream outFile("compressed.huff", std::ios::binary); 
    if (!outFile.is_open()) {
        std::cerr << "Error: Could not open output file compressed.huff" << std::endl;
        return;
    }

    // 1. Write Placeholder for Final Padding Bit Count
    long padding_pos = outFile.tellp();
    char final_valid_bits_placeholder = 0;
    outFile.write(&final_valid_bits_placeholder, sizeof(char));

    // 2. Write Optimized Header (The Tree Structure)
    WriteTreeStructure(root, outFile, current_byte, bit_count);

    // 3. Flush any pending bits from header writing
    if (bit_count > 0) {
        outFile.write(reinterpret_cast<const char*>(&current_byte), 1);
        current_byte = 0;
        bit_count = 0;
        compressedByteCnt += 1;
    }
    
    std::ifstream inFile(filePath, std::ios::binary);
    if (!inFile.is_open()) {
        std::cerr << "Error: Could not open input file "<<filePath << std::endl;
        outFile.close();
        return;
    }

    // 4. Write Encoded Data (Bit Packing)
    char c;
    while (inFile.get(c)) {
        if (charCode.count(c)) {
            const std::string& codeString = charCode.at(c);
            for (char bit : codeString) {
                writeBit(outFile, current_byte, bit_count, bit);
            }
        }
    }

    // 5. Final Write and Padding Metadata Update
    char final_valid_bits;
    if (bit_count > 0) {
        outFile.write(reinterpret_cast<const char*>(&current_byte), 1);
        final_valid_bits = static_cast<char>(bit_count);
        compressedByteCnt += 1;
    } else {
        final_valid_bits = 8;
    }

    outFile.seekp(padding_pos);
    outFile.write(&final_valid_bits, sizeof(char));

    inFile.close();
    outFile.close();
    
    cout << "\nCompression complete! Output saved to compressed.huff (Optimized Header).\n";
    cout << "Original Size: " << TotalCharCnt << " Bytes\n";
    cout << "Compressed Size (Data + Header): " << compressedByteCnt << " Bytes\n";
    if (TotalCharCnt > 0) {
        float ratio = compressedByteCnt / TotalCharCnt;
        cout << "COMPRESSION RATIO (Compressed/Original): " << ratio << endl;
        cout << "SAVINGS: " << (1.0 - ratio) * 100 << "%\n";
    }
}

// Function to decompress the file
void decompress_file() {
	cout<<"Please enter the file path with respect to current directory : ";
	cin >> compressFilePath;
	cout << endl;
    cout << "\n--- Analyzing " << compressFilePath << " ---\n";
    std::ifstream inFile(compressFilePath, std::ios::binary);
    std::ofstream outFile("decompressed.txt");
    
    if (!inFile.is_open() || !outFile.is_open()) {
        std::cerr << "Error: Could not open " <<compressFilePath <<" or decompressed.txt" << std::endl;
        return;
    }

    unsigned char current_byte = 0;
    int bit_count = 0;
    bool endOfFile = false;
    long decoded_char_count = 0;

    // 1. Read Padding Bit Count from Header
    char final_valid_bits;
    if (!inFile.read(&final_valid_bits, sizeof(char))) {
        cerr << "Error reading padding count." << endl;
        return;
    }
    int padding_bits = final_valid_bits;
    cout << "\nPadding Bits in last byte: " << padding_bits << endl;

    // 2. Rebuild Huffman Tree
    Node* root = ReadTreeStructure(inFile, current_byte, bit_count, endOfFile);
    if (root == nullptr) {
        cerr << "Error: Failed to rebuild Huffman tree." << endl;
        return;
    }
    
    // Determine the total expected data bits to read (Total compressed size - header size)
    // The total characters we expect to decode is needed to know when to stop reading bits.
    // Since we don't store the character count in the header, we must rely on reaching
    // the end of the file data stream, taking padding into account.
    
    // Save current file pointer to know where encoded data starts
    long data_start_pos = inFile.tellg();
    
    // Go to the end of file to find total size
    inFile.seekg(0, ios::end);
    long total_size = inFile.tellg();
    
    // Calculate size of encoded data bytes
    long encoded_data_bytes = total_size - data_start_pos;
    
    // Return to the start of the encoded data
    inFile.seekg(data_start_pos);

    // 3. Read and Decode Encoded Data
    Node* currentNode = root;
    long total_bits_to_read = encoded_data_bytes * 8;
    
    // Total bits to process BEFORE padding is encountered
    long bits_processed = 0;
    
    // The number of bits in the last byte that are actually data (not padding)
    long data_bits_in_last_byte = (encoded_data_bytes > 0) ? padding_bits : 0;
    
    // Total actual data bits to read
    long total_data_bits = (encoded_data_bytes - 1) * 8 + data_bits_in_last_byte;

    cout << "Total data bytes to decode: " << encoded_data_bytes << endl;
    cout << "Total actual data bits to read: " << total_data_bits << endl;

    // Loop through all data bits
    while (bits_processed < total_data_bits) {
        char bit = readBit(inFile, current_byte, bit_count, endOfFile);
        if (endOfFile) break;

        bits_processed++;

        // Traverse the tree based on the bit
        if (bit == '0') {
            currentNode = currentNode->left;
        } else if (bit == '1') {
            currentNode = currentNode->right;
        }

        // Check if a leaf node (character) has been reached
        if (currentNode->left == nullptr && currentNode->right == nullptr) {
            outFile.put(currentNode->character);
            decoded_char_count++;
            currentNode = root; // Reset to root for the next character
        }
    }

    // Clean up
    inFile.close();
    outFile.close();
    cout << "Decompression complete! Output saved to decompressed.txt.\n";
    cout << "Total characters decoded: " << decoded_char_count << endl;
}


void cleanup_tree(Node* node) {
    if (node == nullptr) return;
    cleanup_tree(node->left);
    cleanup_tree(node->right);
    delete node;
}

// --- MAIN EXECUTION ---

int main() {
    print_alternate_banner();
    
    int choice;
    cout << "Select Operation:\n";
    cout << "1. Compress an TXT File to   -> compressed.huff\n";
    cout << "2. Decompress any HUFF file to -> decompressed.txt\n";
    cout << "Enter choice (1 or 2): ";
    cin >> choice;

    if (choice == 1) {
        // Compression Sequence
        read_file();
        if (TotalCharCnt == 0 && characterFrequency.empty()) {
            cout << "Input file is empty or missing. Aborting compression.\n";
            return 0;
        }
        
        Node* headOfTree = build_huffman_tree();
        if (headOfTree == nullptr) {
            cout << "Failed to build Huffman tree. Aborting.\n";
            return 0;
        }

        generate_code(headOfTree, currentCode);
        compress_file();
        
        cleanup_tree(headOfTree); // Clean up the tree after use
    } else if (choice == 2) {
        // Decompression Sequence
        decompress_file();
    } else {
        cout << "Invalid choice. Please enter 1 or 2.\n";
    }

    return 0;
}