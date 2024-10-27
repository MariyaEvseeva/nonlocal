#ifndef NONLOCAL_MESH_SU2_PARSER_HPP
#define NONLOCAL_MESH_SU2_PARSER_HPP

#include "mesh_container_2d.hpp"
#include "vtk_elements_set.hpp"

namespace nonlocal::mesh {

template<class T, class I>
class mesh_parser<T, I, mesh_format::SU2> final {
    mesh_container_2d<T, I>& _mesh;

    template<size_t... K, class Stream>
    std::vector<I> read_element(Stream& mesh_file);
    template<class Stream>
    std::vector<I> read_element(Stream& mesh_file, const size_t type);
    template<class Stream>
    auto read_elements_2d(Stream& mesh_file);
    template<class Stream>
    auto read_nodes(Stream& mesh_file);
    template<class Stream>
    auto read_elements_groups(Stream& mesh_file);

public:
    explicit mesh_parser(mesh_container_2d<T, I>& mesh) noexcept;

    template<class Stream>
    void parse(Stream& mesh_file);
};

template<class T, class I>
mesh_parser<T, I, mesh_format::SU2>::mesh_parser(mesh_container_2d<T, I>& mesh) noexcept
    : _mesh{mesh} {}

template<class T, class I>
template<size_t... K, class Stream>
std::vector<I> mesh_parser<T, I, mesh_format::SU2>::read_element(Stream& mesh_file) {
    std::vector<I> element(sizeof...(K));
    (mesh_file >> ... >> element[K]);
    return element;
}

template<class T, class I>
template<class Stream>
std::vector<I> mesh_parser<T, I, mesh_format::SU2>::read_element(Stream& mesh_file, const size_t type) {
    switch(vtk_element_number(type)) {
        case vtk_element_number::LINEAR:
            return read_element<0, 1>(mesh_file);

        case vtk_element_number::QUADRATIC:
            return read_element<0, 2, 1>(mesh_file);

        case vtk_element_number::TRIANGLE:
            return read_element<0, 1, 2>(mesh_file);

        case vtk_element_number::QUADRATIC_TRIANGLE:
            return read_element<0, 1, 2, 3, 4, 5>(mesh_file);

        case vtk_element_number::BILINEAR:
            return read_element<0, 1, 2, 3>(mesh_file);

        case vtk_element_number::QUADRATIC_SERENDIPITY:
            return read_element<0, 2, 4, 6, 1, 3, 5, 7>(mesh_file);

        // TODO: fix parse quadratic lagrange elements
        //case vtk_element_number::QUADRATIC_LAGRANGE:
        //   return read_element<0, 2, 4, 6, 1, 3, 5, 7, 8>(mesh_file);

        default:
            throw std::domain_error{"Unknown element type."};
    }
}

template<class T, class I>
template<class Stream>
auto mesh_parser<T, I, mesh_format::SU2>::read_elements_2d(Stream& mesh_file) {
    std::string pass;
    size_t elements_count = 0;
    mesh_file >> pass >> pass >> pass >> elements_count;
    std::vector<std::vector<I>> elements_2d(elements_count);
    std::vector<uint8_t> elements_types_2d(elements_count);
    for(const size_t e : std::ranges::iota_view{0u, elements_count}) {
        size_t type = 0;
        mesh_file >> type;
        elements_types_2d[e] = type;
        elements_2d[e] = read_element(mesh_file, type);
        mesh_file >> pass;
    }
    return std::make_tuple(std::move(elements_2d), std::move(elements_types_2d));
}

template<class T, class I>
template<class Stream>
auto mesh_parser<T, I, mesh_format::SU2>::read_nodes(Stream& mesh_file) {
    size_t nodes_count = 0;
    std::string pass;
    mesh_file >> pass >> nodes_count;
    std::vector<std::array<T, 2>> nodes(nodes_count);
    for(auto& [x, y] : nodes)
        mesh_file >> x >> y >> pass;
    return nodes;
}

template<class T, class I>
template<class Stream>
auto mesh_parser<T, I, mesh_format::SU2>::read_elements_groups(Stream& mesh_file) {
    std::string pass;
    size_t groups_count = 0;
    mesh_file >> pass >> groups_count;
    std::unordered_set<std::string> groups_names;
    std::unordered_map<std::string, std::vector<std::vector<I>>> elements_in_groups(groups_count);
    std::unordered_map<std::string, std::vector<uint8_t>> types_in_groups(groups_count);
    for(const size_t group : std::ranges::iota_view{0u, groups_count}) {
        std::string group_name;
        size_t elements_count = 0;
        mesh_file >> pass >> group_name >> pass >> elements_count;

        auto& elements = elements_in_groups[group_name] = {};
        auto& types = types_in_groups[group_name] = {};
        groups_names.emplace(std::move(group_name));
        types.resize(elements_count);
        elements.resize(elements_count);

        for(const size_t e : std::ranges::iota_view{0u, elements_count}) {
            size_t type = 0;
            mesh_file >> type;
            types[e] = type;
            elements[e] = read_element(mesh_file, type);
        }
    }
    return std::make_tuple(std::move(groups_names), std::move(elements_in_groups), std::move(types_in_groups));
}

template<class T, class I>
template<class Stream>
void mesh_parser<T, I, mesh_format::SU2>::parse(Stream& mesh_file) {
    _mesh._elements_set = std::make_unique<vtk_elements_set<T>>();

    auto [elements_2d, elements_types_2d] = read_elements_2d(mesh_file);
    _mesh._elements_2d_count = elements_2d.size();
    _mesh._nodes = read_nodes(mesh_file);
    auto [groups_names, elements_in_groups, elements_types] = read_elements_groups(mesh_file);

    size_t elements_2d_shift = 0;
    size_t elements_shift = _mesh.elements_2d_count();
    for(const auto& [group, types] : elements_types) {
        if (group == "DEFAULT")
            throw std::domain_error{"The group name cannot be called \"DEFAULT\", as it is a reserved group name."};
        if (_mesh.get_elements_set().is_element_1d(types.front())) {
            _mesh._elements_groups[group] = std::ranges::iota_view{elements_shift, elements_shift + types.size()};
            elements_shift += types.size();
            _mesh._groups_1d.insert(group);
        } else {
            _mesh._elements_groups[group] = std::ranges::iota_view{elements_2d_shift, elements_2d_shift + types.size()};
            elements_2d_shift += types.size();
            _mesh._groups_2d.insert(group);
        }
    }
    if (elements_2d_shift > elements_2d.size())
        throw std::domain_error{"Problem with parsing groups: some groups of 2D elements overlap each other."};

    _mesh._elements.resize(elements_shift);
    _mesh._elements_types.resize(elements_shift);
    elements_2d_shift = 0;
    elements_shift = _mesh.elements_2d_count();
    for(const auto& [group, range] : _mesh._elements_groups) {
        auto& types = elements_types[group];
        auto& elements = elements_in_groups[group];
        if (_mesh.get_elements_set().is_element_1d(types.front())) {
            for(const size_t e : std::ranges::iota_view{0u, range.size()}) {
                _mesh._elements[range.front() + e] = std::move(elements[e]);
                _mesh._elements_types[range.front() + e] = uint8_t(_mesh.get_elements_set().model_to_local_1d(types[e]));
            }
            elements_shift += range.size();
        } else {
            for(const size_t e : std::ranges::iota_view{0u, range.size()}) {
                _mesh._elements[range.front() + e] = std::move(elements[e]);
                _mesh._elements_types[range.front() + e] = uint8_t(_mesh.get_elements_set().model_to_local_2d(types[e]));
            }
            elements_2d_shift += range.size();
        }
    }
   
    if (elements_2d_shift < elements_2d.size()) {
        _mesh._groups_2d.insert("DEFAULT");
        _mesh._elements_groups["DEFAULT"] = std::ranges::iota_view{elements_2d_shift, elements_2d.size()};
        for(const size_t e : std::ranges::iota_view{0u, _mesh.elements_2d_count()})
            if (!elements_2d[e].empty()) {
                _mesh._elements[elements_2d_shift] = std::move(elements_2d[e]);
                _mesh._elements_types[elements_2d_shift] = uint8_t(_mesh.get_elements_set().model_to_local_2d(elements_types_2d[e]));
                ++elements_2d_shift;
            }
    }
}

}

#endif