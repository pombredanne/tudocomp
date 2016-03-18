#ifndef LZ78RULE_RULE_H
#define LZ78RULE_RULE_H

#include <tudocomp/Compressor.hpp>

#include <tudocomp/lz78/trie.h>
#include <tudocomp/lz78/factors.h>
#include <tudocomp/lz78/coder.h>

namespace lz78 {

using namespace tudocomp;

inline Entries compress_impl(const Env& env, Input& input) {
    auto guard = input.as_stream();
    PrefixBuffer buf(*guard);

    Trie trie(Trie::Lz78);
    Entries entries;

    while (!buf.is_empty()) {
        Result phrase_and_size = trie.find_or_insert(buf);

        entries.push_back(phrase_and_size.entry);
    }

    trie.print(0);

    return entries;
}

class Lz78RuleCoder;
class Lz78RuleCompressor;

const std::string THRESHOLD_OPTION = "lz78rule.threshold";
const std::string THRESHOLD_LOG = "lz78rule.threshold";
const std::string RULESET_SIZE_LOG = "lz78rule.rule_count";

struct Lz78Rule: public Compressor {
    Lz78RuleCoder* m_encoder;

    inline Lz78Rule(Env& env,
                    Lz78RuleCoder* encoder):
        Compressor(env),
        m_encoder(encoder) {};

    inline virtual void compress(Input& input, Output& output) override final;

    inline virtual void decompress(Input& inp, Output& out) override final;
};

inline void Lz78Rule::compress(Input& input, Output& out) {
    auto entries = compress_impl(*m_env, input);
    m_env->log_stat(RULESET_SIZE_LOG, entries.size());
    m_encoder->code(std::move(entries), out);
}

inline void Lz78Rule::decompress(Input& inp, Output& out) {
    m_encoder->decode(inp, out);
}

}

#endif