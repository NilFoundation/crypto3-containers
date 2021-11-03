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

#ifndef CRYPTO3_MERKLE_HPP
#define CRYPTO3_MERKLE_HPP

#include <vector>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_as_tree.hpp>

#include <nil/crypto3/detail/static_digest.hpp>
#include <nil/crypto3/hash/algorithm/hash.hpp>
#include <nil/merkle/utilities.hpp>
#include <nil/merkle/property.hpp>

template<class Graph>
void print(Graph &g) {
    typename Graph::vertex_iterator i, end;
    typename Graph::out_edge_iterator ei, edge_end;
    typename Graph::in_edge_iterator ein, edgein_end;
    for (boost::tie(i, end) = vertices(g); i != end; ++i) {
        boost::tie(ein, edgein_end) = in_edges(*i, g);
        if (ein == edgein_end) {
            std::cout << *i << " --- leaf";
        } else {
            std::cout << *i << " <-- ";
        }
        for (; ein != edgein_end; ++ein)
            std::cout << source(*ein, g) << "  ";
        std::cout << std::endl;
    }
}

namespace nil {
    namespace crypto3 {
        namespace merkletree {
            // Merkle Tree.
            //
            // All leafs and nodes are stored in a BGL graph structure.
            //
            // A merkle tree is a tree in which every non-leaf node is the hash of its
            // child nodes. A diagram for MerkleTree arity = 2:
            //
            //         root = h1234 = h(h12 + h34)
            //        /                           \
            //  h12 = h(h1 + h2)            h34 = h(h3 + h4)
            //   /            \              /            \
            // h1 = h(tx1)  h2 = h(tx2)    h3 = h(tx3)  h4 = h(tx4)
            // ```
            //
            // In graph representation:
            //
            // ```text
            //    root -> h12, h34
            //    h12  -> h1, h2
            //    h34  -> h3, h4
            // ```
            //
            // Merkle root is always the top element.

            template<typename Hash>
            struct MerkleTree_basic_policy {
                typedef typename Hash::digest_type hash_result_type;
                constexpr static const std::size_t hash_digest_size =
                    Hash::digest_bits / 8 + (Hash::digest_bits % 8 ? 1 : 0);
            };

            template<typename Hash, size_t Arity = 2>
            struct MerkleTree {

                typedef typename MerkleTree_basic_policy<Hash>::hash_result_type element;
                constexpr static const std::size_t element_size = MerkleTree_basic_policy<Hash>::hash_digest_size;

                typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS,
                                              boost::property<vertex_hash_t, element>>
                    graph_t;
                typedef typename boost::property_map<graph_t, vertex_hash_t>::type vertex_name_map_t;

                template<typename Hashable, size_t Size>
                MerkleTree(std::vector<std::array<Hashable, Size>> data) {
                    BOOST_ASSERT_MSG(data.size() % Arity == 0, "Wrong leafs number");

                    leafs = data.size();
                    len = utilities::get_merkle_tree_len(leafs, Arity);
                    row_count = utilities::get_merkle_tree_row_count(leafs, Arity);
                    for (size_t i = 0; i < len; ++i) {
                        boost::add_vertex(Tree);
                    }
                    size_t prev_layer_element = 0;
                    size_t start_layer_element = 0;
                    size_t layer_elements = leafs;
                    for (size_t row_number = 0; row_number < row_count; ++row_number) {
                        for (size_t current_element = start_layer_element;
                             current_element < start_layer_element + layer_elements; ++current_element) {
                            if (row_number == 0) {
                                hash_map[current_element] = crypto3::hash<Hash>(data[current_element]);
                            } else {
                                std::array<uint8_t, element_size * Arity> new_input;
                                for (size_t i = 0; i < Arity; ++i) {
                                    size_t children_index =
                                        (current_element - start_layer_element) * Arity + prev_layer_element + i;
                                    std::copy(hash_map[children_index].begin(), hash_map[children_index].end(),
                                              new_input.begin() + i * element_size);
                                    add_edge(children_index, current_element, Tree);
                                }
                                hash_map[current_element] = crypto3::hash<Hash>(new_input);
                            }
                        }
                        prev_layer_element = start_layer_element;
                        start_layer_element += layer_elements;
                        layer_elements /= Arity;
                    }
                }

                std::array<size_t, Arity> children(size_t leaf_index) {
                    std::array<size_t, Arity> res;
                    typename boost::graph_traits<graph_t>::in_edge_iterator ein, edgein_end;
                    size_t i = 0;
                    for (boost::tie(ein, edgein_end) = in_edges(leaf_index, Tree); ein != edgein_end; ++ein, ++i) {
                        res[i] = source(*ein, Tree);
                    }
                    return res;
                }

                size_t parent(size_t leaf_index) {
                    typename boost::graph_traits<graph_t>::out_edge_iterator ei, edge_end;
                    boost::tie(ei, edge_end) = out_edges(leaf_index, Tree);
                    return target(*ei, Tree);
                }

                element root() {
                    return hash_map[len - 1];
                }

                std::vector<element> hash_path(size_t leaf_index) {
                    std::vector<element> res;
                    res.push_back(hash_map[leaf_index]);
                    typename boost::graph_traits<graph_t>::adjacency_iterator ai, a_end;
                    boost::tie(ai, a_end) = adjacent_vertices(leaf_index, Tree);
                    while (ai != a_end) {    // while not the root
                        res.push_back(get(hash_map, *ai));
                        boost::tie(ai, a_end) = adjacent_vertices(*ai, Tree);
                    }
                    return res;
                }

                element &operator[](std::size_t idx) {
                    return hash_map[idx];
                }

                friend std::ostream &operator<<(std::ostream &o, MerkleTree const &a) {
                    typename boost::graph_traits<graph_t>::vertex_iterator i, end;
                    typename boost::graph_traits<graph_t>::in_edge_iterator ein, edgein_end;
                    for (boost::tie(i, end) = vertices(a.Tree); i != end; ++i) {
                        boost::tie(ein, edgein_end) = in_edges(*i, a.Tree);
                        if (ein == edgein_end) {
                            std::cout << "(" << *i << ", " << get(a.hash_map, *i) << ") --- leaf";
                        } else {
                            std::cout << "(" << *i << ", " << get(a.hash_map, *i) << ") <-- ";
                        }
                        for (; ein != edgein_end; ++ein) {
                            size_t idx_vertex = source(*ein, a.Tree);
                            std::cout << "(" << idx_vertex << ", " << get(a.hash_map, idx_vertex) << ")  ";
                        }
                        std::cout << std::endl;
                    }
                    return o;
                }

                size_t get_row_count() {
                    return row_count;
                }

                size_t get_len() {
                    return len;
                }

                size_t get_leafs() {
                    return leafs;
                }

            private:
                graph_t Tree;
                vertex_name_map_t hash_map = get(vertex_hash, Tree);

                size_t leafs;
                size_t len;
                // Note: The former 'upstream' merkle_light project uses 'height'
                // (with regards to the tree property) incorrectly, so we've
                // renamed it since it's actually a 'row_count'.  For example, a
                // tree with 2 leaf nodes and a single root node has a height of
                // 1, but a row_count of 2.
                //
                // Internally, this code considers only the row_count.
                size_t row_count;
            };
        }    // namespace merkletree
    }        // namespace crypto3
}    // namespace nil

#endif    // CRYPTO3_MERKLE_HPP