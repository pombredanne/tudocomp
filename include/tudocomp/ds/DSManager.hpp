#pragma once

#include <map>
#include <memory>
#include <tuple>

#include <tudocomp/def.hpp>
#include <tudocomp/util/cpp14/integer_sequence.hpp>

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/Env.hpp>
#include <tudocomp/util/View.hpp>

#include <tudocomp/ds/CompressMode.hpp>
#include <tudocomp/ds/DSDef.hpp>
#include <tudocomp/ds/DSProvider.hpp>
#include <tudocomp/ds/DSDependencyGraph.hpp>

#include <tudocomp/CreateAlgorithm.hpp>

namespace tdc {

static_assert(
    std::is_same<View::value_type, uliteral_t>::value,
    "View::value_type and uliteral_t must be the same");

/// Manages data structures and construction algorithms.
template<typename... provider_ts>
class DSManager : public Algorithm {
private:
    using this_t = DSManager<provider_ts...>;

    using provider_tuple_t = std::tuple<std::shared_ptr<provider_ts>...>;
    provider_tuple_t m_providers;

    View m_input; // TODO: when the changes to the I/O are done, this can be an Input?
    CompressMode m_cm;

    std::map<dsid_t, std::shared_ptr<DSProvider>> m_provider_map;

    template<size_t... Is>
    inline void register_providers(
        provider_tuple_t& tuple,
        std::index_sequence<Is...>) {
        
        register_provider(std::get<Is>(tuple)...);
    }

    template<typename head_t, typename... tail_t>
    inline void register_provider(head_t& head, tail_t&... tail) {
        // head_t is std::shared_ptr<provider_t>
        using provider_t = typename head_t::element_type;
        DLOG(INFO) << "register_provider: " << provider_t::meta().name();

        //TODO: head = std::make_shared<provider_t>(env().env_for_option("providers", i));
        head = std::make_shared<provider_t>(create_env(provider_t::meta()));

        // recurse
        register_provider(tail...);
    }

    inline void register_provider() {
        // done
    }

public:
    inline static Meta meta() {
        Meta m("ds", "ds");
        m.option("compress").dynamic("delayed");
        //TODO: m.option("providers").templated_array("provider")
        return m;
    }

    inline DSManager(Env&& env, const View& input)
        : Algorithm(std::move(env)), m_input(input) {

        if(!m_input.ends_with(uint8_t(0))){
             throw std::logic_error(
                 "Input has no sentinel! Please make sure you declare "
                 "the compressor calling this with "
                 "`m.needs_sentinel_terminator()` in its `meta()` function."
            );
        }

        register_providers(
            m_providers,
            std::make_index_sequence<std::tuple_size<provider_tuple_t>::value>());

        auto& cm_str = this->env().option("compress").as_string();
        if(cm_str == "delayed") {
            m_cm = CompressMode::delayed;
        } else if(cm_str == "compressed") {
            m_cm = CompressMode::compressed;
        } else {
            m_cm = CompressMode::plain;
        }
    }

private:
    template<size_t... Is>
    inline DSProvider* find_provider(
        const dsid_t dsid,
        const provider_tuple_t& tuple,
        std::index_sequence<Is...>) {
        
        // start a variadic linear search ...
        return find_provider(dsid, std::get<Is>(tuple)...);
    }

    template<typename head_t, typename... tail_t>
    inline DSProvider* find_provider(const dsid_t dsid, head_t head, tail_t... tail) {
        if(head->does_provide(dsid)) {
            // found
            return static_cast<DSProvider*>(head.get());
        } else {
            // recurse
            return find_provider(dsid, tail...);
        }
    }

    inline DSProvider* find_provider(const dsid_t dsid) {
        // none found
        throw std::logic_error(
            std::string("No provider available for text ds ") +
            ds::name_for(dsid));
    }

public:
    inline DSProvider& get_provider(dsid_t dsid) {
        DSProvider* p = find_provider(dsid, m_providers,
            std::make_index_sequence<std::tuple_size<provider_tuple_t>::value>());

        return *p;  
    }

    inline void construct(const dsid_list_t& requested_ds) {
        DLOG(INFO) << "create dependency graph";
        
        DSDependencyGraph<this_t> g(*this);
        for(auto id : requested_ds) {
            g.request(id);
        }

        g.construct_requested();
    }

    const View& input = m_input;
};

} //ns