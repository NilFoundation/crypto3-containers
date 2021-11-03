//---------------------------------------------------------------------------//
// Copyright (c) 2018-2020 Mikhail Komarov <nemo@nil.foundation>
// Copyright (c) 2021 Aleksei Moskvin <alalmoskvin@gmail.com>
//
// MIT License
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//---------------------------------------------------------------------------//

#define BOOST_TEST_MODULE containter_merkletree_test

#include <nil/crypto3/hash/sha2.hpp>
#include <nil/crypto3/hash/md5.hpp>
#include <nil/crypto3/hash/blake2b.hpp>

#include <nil/merkle/merkle.hpp>
#include <nil/merkle/proof.hpp>

#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/data/monomorphic.hpp>

#include <cstdio>

using namespace nil::crypto3;
using namespace nil::crypto3::detail;
using namespace nil::crypto3::merkletree;

BOOST_AUTO_TEST_SUITE(containers_merkltree_test)

template <typename Hash, size_t Arity, typename Element>
void testing_validate_template(std::vector<Element> data) {
    std::array<uint8_t, 7> data_not_in_tree = {'\x6d', '\x65', '\x73', '\x73', '\x61', '\x67', '\x65'};
    MerkleTree<Hash, Arity> tree(data);
    MerkleProof<Hash, Arity> proof(tree, 0);
    bool good_validate = proof.validate(data[0]);
    bool wrong_leaf_validate = proof.validate(data[1]);
    bool wrong_data_validate = proof.validate(data_not_in_tree);
    BOOST_CHECK(true == good_validate);
    BOOST_CHECK(false == wrong_leaf_validate);
    BOOST_CHECK(false == wrong_data_validate);
}

template <typename Hash, size_t Arity, typename Element>
void testing_hash_template(std::vector<Element> data, std::string result) {
    MerkleTree<Hash, Arity> tree(data);
    std::cout << tree.root() << std::endl;
    BOOST_CHECK(result == std::to_string(tree.root()).data());
}

BOOST_AUTO_TEST_CASE(merkletree_validate_test_1) {
    std::vector<std::array<char, 1> > v = {{'0'}, {'1'}, {'2'}, {'3'}, {'4'}, {'5'}, {'6'}, {'7'}};
    testing_validate_template<hashes::sha2<256>, 2>(v);
    testing_validate_template<hashes::md5, 2>(v);
    testing_validate_template<hashes::blake2b<224>, 2>(v);
}

BOOST_AUTO_TEST_CASE(merkletree_validate_test_2) {
    std::vector<std::array<char, 1> > v = {{'0'}, {'1'}, {'2'}, {'3'}, {'4'}, {'5'}, {'6'}, {'7'}, {'8'}};
    testing_validate_template<hashes::sha2<256>, 3>(v);
    testing_validate_template<hashes::md5, 3>(v);
    testing_validate_template<hashes::blake2b<224>, 3>(v);
}

BOOST_AUTO_TEST_CASE(merkletree_hash_test_1) {
    std::vector<std::array<char, 1> > v = {{'0'}, {'1'}, {'2'}, {'3'}, {'4'}, {'5'}, {'6'}, {'7'}};
    testing_hash_template<hashes::sha2<256>, 2>(v, "3b828c4f4b48c5d4cb5562a474ec9e2fd8d5546fae40e90732ef635892e42720");
    testing_hash_template<hashes::md5, 2>(v, "11ee8b50825ce6f816a1ae06d4aa0045");
    testing_hash_template<hashes::blake2b<224>, 2>(v, "0ed2a2145cae554ca57f08420d6cb58629ca1e89dc92f819c6c1d13d");
}

BOOST_AUTO_TEST_CASE(merkletree_hash_test_2) {
    std::vector<std::array<char, 1> > v = {{'0'}, {'1'}, {'2'}, {'3'}, {'4'}, {'5'}, {'6'}, {'7'}, {'8'}};
    testing_hash_template<hashes::sha2<256>, 3>(v, "6831d4d32538bedaa7a51970ac10474d5884701c840781f0a434e5b6868d4b73");
    testing_hash_template<hashes::md5, 3>(v, "0733c4cd580b1523cfbb9751f42e9420");
    testing_hash_template<hashes::blake2b<224>, 3>(v, "d9d0ff26d10aaac2882c08eb2b55e78690c949d1a73b1cfc0eb322ee");
}

BOOST_AUTO_TEST_SUITE_END()