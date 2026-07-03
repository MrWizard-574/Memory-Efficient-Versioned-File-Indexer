#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <cctype>
#include <memory>
#include <stdexcept>
#include <chrono>

using namespace std;

// User-defined template 
template <typename T>
void toLowerCase(T& str) {
    for (auto& c : str) {
        c = tolower(c);
    }
}

// buffer reader class
class FileBufferReader {
    ifstream file;
    size_t bufferSize;
    vector<char> buffer;
public:
    FileBufferReader(const string& path, size_t sizeKB) : bufferSize(sizeKB * 1024), buffer(sizeKB * 1024) {
        if (sizeKB < 256 || sizeKB > 1024) { 
            throw std::invalid_argument("Buffer size must be between 256 KB and 1024 KB.");
        }
        file.open(path, std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file: " + path);
        }
    }
    
    size_t readChunk(char*& outBuffer) {
        file.read(buffer.data(), bufferSize);
        outBuffer = buffer.data();
        return file.gcount();
    }
    
    bool eof() const { return file.eof(); }
};

// Tokenizer class
class Tokenizer {
    string leftover;
public:
    void tokenizeChunk(const char* buffer, size_t size, unordered_map<string, int>& index) {
        string currentWord = leftover;
        leftover.clear();

        for (size_t i = 0; i < size; ++i) {
            char c = buffer[i];
            if (isalnum(c)) { 
                currentWord += c;
            } else {
                if (!currentWord.empty()) {
                    toLowerCase(currentWord); 
                    index[currentWord]++;
                    currentWord.clear();
                }
            }
        }
        leftover = currentWord;
    }

    void finalize(unordered_map<string,int>& index) {
        if (!leftover.empty()) {
            toLowerCase(leftover);
            index[leftover]++;
            leftover.clear();
        }
    }
};

// Version index class
class VersionIndex {
    string versionName;
    unordered_map<string,int> index; 
public:
    VersionIndex() {}
    VersionIndex(const string &name) : versionName(name) {}

    void buildIndex(const string &filePath, size_t bufferSizeKB) {
        FileBufferReader reader(filePath, bufferSizeKB);
        Tokenizer tokenizer;
        char* buffer;
        
        // file processed incrementally
        while (!reader.eof()) {
            auto bytesRead = reader.readChunk(buffer);
            if (bytesRead > 0) {
                tokenizer.tokenizeChunk(buffer, bytesRead, index);
            }
        }
        tokenizer.finalize(index);
    }

    // Function overloading
    int getFrequency(const string& word) const {
        auto it = index.find(word);
        return (it != index.end()) ? it->second : 0;
    }
    int getFrequency(const string& word, bool printWarning) const {
        int freq = getFrequency(word);
        if (freq == 0 && printWarning) cout << "Warning: Word not found.\n";
        return freq;
    }

    const string& getVersionName() const { return versionName; }
    const unordered_map<string, int>& getMap() const { return index; }
};

// Abstract base class
class QueryProcessor {
protected:
    size_t bufferKB;
public:
    QueryProcessor(size_t bufKB) : bufferKB(bufKB) {}
    virtual ~QueryProcessor() {}
    
    // Pure virtual function
    virtual void execute() = 0; 
};

// Derived class of QueryProcessor
class WordQueryProcessor : public QueryProcessor {
    VersionIndex index;
    string queryWord;
public:
    WordQueryProcessor(const string& file, const string& version, int bufKB, const string& word)
        : QueryProcessor(bufKB), index(version), queryWord(word) {
        toLowerCase(queryWord);
        index.buildIndex(file, bufKB);
    }
    void execute() override {
        cout << "Version: " << index.getVersionName() << "\n";
        cout << "Query Result (" << queryWord << "): " << index.getFrequency(queryWord) << "\n";
    }
};

// Derived class of QueryProcessor
class TopKQueryProcessor : public QueryProcessor{
    VersionIndex index;
    int k;
public:
    TopKQueryProcessor(const string& file, const string& version, int bufKB, int topK)
        : QueryProcessor(bufKB), index(version), k(topK) {
        index.buildIndex(file, bufKB);
    }
    void execute() override {
        cout << "Version: " << index.getVersionName() << "\n";
        vector<pair<string, int>> sortedWords(index.getMap().begin(), index.getMap().end());
        
        sort(sortedWords.begin(), sortedWords.end(), [](const auto& a, const auto& b) {
            return a.second > b.second; 
        });
        
        cout << "Query Result (Top " << k << "):\n";
        for (int i = 0; i < k && i < sortedWords.size(); ++i) {
            cout << sortedWords[i].first << " -> " << sortedWords[i].second << "\n";
        }
    }
};

// Derived class of QueryProcessor
class DiffQueryProcessor : public QueryProcessor {
    VersionIndex index1;
    VersionIndex index2;
    string queryWord;
public:
    DiffQueryProcessor(const string& f1, const string& v1, const string& f2, const string& v2, int bufKB, const string& word)
        : QueryProcessor(bufKB), index1(v1), index2(v2), queryWord(word) {
        toLowerCase(queryWord);
        index1.buildIndex(f1, bufKB); 
        index2.buildIndex(f2, bufKB);
    }
    void execute() override {
        std::cout << "Versions: " << index1.getVersionName() << " and " << index2.getVersionName() << "\n";
        int f1 = index1.getFrequency(queryWord);
        int f2 = index2.getFrequency(queryWord);
        cout << "Query Result (Diff for '" << queryWord << "'): " << f2 - f1 << "\n";
    }
};

// Main function
int main(int argc, char* argv[]) {
    auto start = chrono::high_resolution_clock::now();

    string file1, file2, ver1, ver2, queryType, queryWord;
    int bufferSize = 0;
    int topK = 0;

    try { 
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--file" && i + 1 < argc) file1 = argv[++i];
            else if (arg == "--file1" && i + 1 < argc) file1 = argv[++i];
            else if (arg == "--file2" && i + 1 < argc) file2 = argv[++i];
            else if (arg == "--version" && i + 1 < argc) ver1 = argv[++i];
            else if (arg == "--version1" && i + 1 < argc) ver1 = argv[++i];
            else if (arg == "--version2" && i + 1 < argc) ver2 = argv[++i];
            else if (arg == "--buffer" && i + 1 < argc) bufferSize = stoi(argv[++i]);
            else if (arg == "--query" && i + 1 < argc) queryType = argv[++i];
            else if (arg == "--word" && i + 1 < argc) queryWord = argv[++i];
            else if (arg == "--top" && i + 1 < argc) topK = std::stoi(argv[++i]);
        }

        if (bufferSize == 0) throw invalid_argument("Buffer size not provided or invalid.");

        unique_ptr<QueryProcessor> processor;

        if (queryType == "word") {
            processor = make_unique<WordQueryProcessor>(file1, ver1, bufferSize, queryWord);
        } else if (queryType == "top") {
            processor = make_unique<TopKQueryProcessor>(file1, ver1, bufferSize, topK);
        } else if (queryType == "diff") {
            processor = make_unique<DiffQueryProcessor>(file1, ver1, file2, ver2, bufferSize, queryWord);
        } else {
            throw invalid_argument("Invalid or missing query type.");
        }
 
        processor->execute();

        cout << "Allocated Buffer Size: " << bufferSize << " KB\n";
    } catch (const exception &e) {
        std::cerr << "Error: " << e.what() << "\n";
    }

    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double> elapsed = end - start;
    cout << "Total Execution Time: " << elapsed.count() << " seconds\n";

    return 0;
}
