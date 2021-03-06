#pragma once

#include <tudocomp/util/View.hpp>
#include <tudocomp/compressors/esp/SLP.hpp>
#include <tudocomp/compressors/esp/Rounds.hpp>
#include <tudocomp/compressors/esp/utils.hpp>
#include <tudocomp/compressors/esp/DebugContext.hpp>

namespace tdc {namespace esp {
    template<typename ipd_t>
    class EspContext {
        const Env* m_env;
    public:
        using esp_view_t = ConstGenericView<size_t>;

        template<typename F>
        auto with_env(F f) -> decltype(f(*m_env)) {
            if (m_env != nullptr) {
                return f(*m_env);
            }
            return decltype(f(*m_env))();
        }

        DebugContext debug;
        IPDStats ipd_stats;

        EspContext(const Env* e, bool silent):
            m_env(e),
            debug(std::cout, !silent, false)
        {}

        bool behavior_metablocks_maximimze_repeating = true;
        bool behavior_landmarks_tie_to_right = true;
        bool behavior_iter_log_mode = false; // UNUSED

        template<typename T>
        SLP generate_grammar(T&& s);
    };
}}
