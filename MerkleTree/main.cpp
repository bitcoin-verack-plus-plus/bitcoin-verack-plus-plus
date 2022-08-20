#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include <regex>
#include "handshake_proof_merklecpp.h"
#include "openssl/sha.h"

// Computes the SHA-256 hash of a string
std::string sha256(const std::string str)
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, str.c_str(), str.size());
    SHA256_Final(hash, &sha256);
    std::stringstream ss;
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

// Recursively list the files in a directory
std::vector<std::string> getFiles(std::string directory, std::string regexStr, std::vector<std::string> directoriesToIgnore) {

	std::vector<std::string> files;
	for(std::filesystem::recursive_directory_iterator i(directory), end; i != end; ++i) {
		if(!is_directory(i->path())) {
			std::string str = i->path();
			if(std::regex_match(str, std::regex(regexStr))) {

				std::string parent = str.substr(0, str.find_last_of("/\\"));
				for(int i = 0; i < directoriesToIgnore.size(); i++) {
					if(std::filesystem::equivalent(parent, directoriesToIgnore.at(i))) {
						//std::cout << "Ignoring \"" << str << "\"" << std::endl;
						continue;
					}
				}
				std::cout << "Including \"" << str << "\"" << std::endl;

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

int main() {
	std::string directory = "../src";
	//std::string current_path = std::filesystem::current_path();
	std::vector<std::string> directoriesToIgnore = {
		directory + "/minisketch",
		directory + "/obj",
		directory + "/qt",
		directory + "/qt/android",
		directory + "/qt/forms",
		directory + "/qt/locale",
		directory + "/qt/res",
		directory + "/qt/test",
	};

	// Get the list of code file names
	std::vector<std::string> files = getFiles("../src", ".*(\\.cpp|\\.c|\\.h|\\.cc|\\.py|\\.sh)", directoriesToIgnore);
	std::vector<std::string> hashes (files.size());
	// Compute the hash of the files
	for(int i = 0; i < files.size(); i++) {
		hashes.at(i) = sha256(getContents(files[i]));
	}
	// Set the initial ID
	hashes.insert(hashes.begin(), "0000000000000000000000000000000000000000000000000000000000000000");
	// Adjust the size to make it a full binary tree
	int targetSize = nextPowerOfTwo(hashes.size()), i = 0;
	while(hashes.size() < targetSize) {
		hashes.push_back(hashes.at(i));
		i++;
	}

	// Cybersecurity Lab: Testing a mini tree
	// std::vector<std::string> hashes (16);
	// for(int i = 0; i < 16; i++) {
	// 	hashes.at(i) = sha256(to_string(i + 1));
	// }




	// Convert the hashes to Merkle node objects
	std::vector<merkle::Tree::Hash> leaves (hashes.size());
	for(int i = 0; i < hashes.size(); i++) {
		merkle::Tree::Hash hash(hashes.at(i));
		leaves.at(i) = hash;
	}

	// Create the tree
	merkle::Tree tree;
	tree.insert(leaves);

	// Update the ID
	updateHashAtIndex(tree, 0, "0000000000000000000000000000000000000000000000000000000000000000");

	if(tree.root().to_string() == "721b70434f32fd73c8cd5045ed66aca8fb43fb94ca82e077603a73e9808bd15b") {
		std::cout << "Correct version" << std::endl;
	} else {
		std::cout << "Incorrect version: " << tree.root().to_string() << std::endl;
	}


	// auto root = tree.root();
	// auto path = tree.path(0);
	// assert(path->verify(root));
	return 0;
}