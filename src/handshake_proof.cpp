// This code is created by the Cybersecurity Lab, University of Colorado, Colorado Springs, 2022

#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include <regex>
#include <handshake_proof_merklecpp.h>
//#include "openssl/sha.h"
using namespace std;
#include <inttypes.h>
#include <logging.h>
#include <hash.h>
//#include <span.h>
//#include <node/context.h>
//#include <sync.h>
//#include <net.h>

#ifndef BITCOIN_HANDSHAKE_PROOF_CPP
#define BITCOIN_HANDSHAKE_PROOF_CPP


class HandshakeProof {
    private:
        bool initialized = false;
        merkle::Tree tree;
        string merkle_hash = "";

        // Computes the SHA-256 hash of a string
        std::string sha256(const std::string str)
        {
            // unsigned char hash[SHA256_DIGEST_LENGTH];
            // SHA256_CTX sha256;
            // SHA256_Init(&sha256);
            // SHA256_Update(&sha256, str.c_str(), str.size());
            // SHA256_Final(hash, &sha256);
            // std::stringstream ss;
            // for(int i = 0; i < SHA256_DIGEST_LENGTH; i++)
            // {
            //     ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
            // }
            // return ss.str();

            uint256 hash;
            CHash256().Write(MakeUCharSpan(str)).Finalize(hash);
            return hash.ToString();
        }

        // Recursively list the files in a directory
        std::vector<std::string> getFiles(std::string directory, std::string regexStr, bool ignore_current_directory = true) {
            std::string current_path = std::filesystem::current_path();

            std::vector<std::string> files;
            for(std::filesystem::recursive_directory_iterator i(".."), end; i != end; ++i) {
                if(!is_directory(i->path())) {
                    std::string str = i->path();
                    if(std::regex_match(str, std::regex(regexStr))) {

                        if(ignore_current_directory) {
                            std::string parent = str.substr(0, str.find_last_of("/\\"));
                            if(std::filesystem::equivalent(parent, current_path)) {
                                continue;
                            }
                        }

                        files.push_back(str);
                    }
                }
            }
            return files;
        }

        // Read the contents of a file at a given file path
        std::string getContents(std::string filePath) {
            std::string contents = "";
            std::ifstream f(filePath);
            while(f) {
                std::string line;
                getline(f, line);
                contents += line + '\n';
            }
            return contents;
        }

        // Given a number (e.g. 10) compute the next power of two (e.g. 16)
        int nextPowerOfTwo(int num) {
            double n = log2(num);
            return (int)pow(2, ceil(n));
        }

        // Update the hash at an index within the tree
        void updateHashAtIndex(merkle::Tree &tree, int index, std::string hash_string) {
            merkle::TreeT<32, merkle::sha256_compress>::Node* ID = tree.walk_to(index, true, [](merkle::TreeT<32, merkle::sha256_compress>::Node* n, bool go_right) {
                n->dirty = true;
                return true;
            });
            merkle::Tree::Hash newHash(hash_string);
            ID->hash = newHash;
            tree.compute_root();
        }

    public:

        string supportedVersion = "";

        //RecursiveMutex cs_handshakeProof;

        // HandshakeProof(int temp) {
        //     LogPrint(BCLog::HANDSHAKE_PROOF, "\nINITIALIZED THE HANDSHAKE PROVER");
        // }

        string generateProof(std::string ID) {
            if(!initialized) initialize();
            // Update the ID
            LogPrint(BCLog::HANDSHAKE_PROOF, "\nGENERATING PROOF FOR %s", ID);
            updateHashAtIndex(tree, 0, sha256(ID));
            return tree.root().to_string();
        }

        bool verifyProof(std::string hash, std::string ID) {
            LogPrint(BCLog::HANDSHAKE_PROOF, "\nVERIFYING PROOF FOR %s", ID);
            return generateProof(ID) == hash;
            
            // if(tree.root().to_string() == "fbdef921f44b67c5104404a2b1d496cedc5001a7bd321fb31bda1a7ac4cef571") {
            //     cout << "Correct version" << endl;
            // } else {
            //     cout << "Incorrect version: " << tree.root().to_string() << endl;
            // }
        }

        void initialize() {
            if(initialized) return;

            // Get the list of code file names
            std::vector<std::string> files = getFiles("..", ".*(\\.cpp|\\.c|\\.h|\\.cc|\\.py|\\.sh)", true);
            std::vector<std::string> hashes ((int)files.size());
            // Compute the hash of the files
            for(int i = 0; i < (int)files.size(); i++) {
                hashes.at(i) = sha256(getContents(files[i]));
            }
            // Set the initial ID
            hashes.insert(hashes.begin(), "0000000000000000000000000000000000000000000000000000000000000000");
            // Adjust the size to make it a full binary tree
            int targetSize = nextPowerOfTwo((int)hashes.size()), i = 0;
            while((int)hashes.size() < targetSize) {
                hashes.push_back(hashes.at(i));
                i++;
            }

            // Cybersecurity Lab: Testing a mini tree
            // std::vector<std::string> hashes (16);
            // for(int i = 0; i < 16; i++) {
            //  hashes.at(i) = sha256(to_string(i + 1));
            // }

            // Convert the hashes to Merkle node objects
            std::vector<merkle::Tree::Hash> leaves ((int)hashes.size());
            for(int i = 0; i < (int)hashes.size(); i++) {
                merkle::Tree::Hash hash(hashes.at(i));
                leaves.at(i) = hash;
            }

            // Create the tree
            tree.insert(leaves);

            // Update the ID
            updateHashAtIndex(tree, 0, "0000000000000000000000000000000000000000000000000000000000000000");

            merkle_hash = tree.root().to_string();

            // auto root = tree.root();
            // auto path = tree.path(0);
            // assert(path->verify(root));
            initialized = true;
        }
};


#endif // BITCOIN_HANDSHAKE_PROOF_CPP