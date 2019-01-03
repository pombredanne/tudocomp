#pragma once

#include <tudocomp/util.hpp>
#include <tudocomp/ds/bwt.hpp>
#include <tudocomp/ds/TextDS.hpp>
#include <tudocomp/Compressor.hpp>
#include <tudocomp/decompressors/BWTDecompressor.hpp>

#include <tudocomp_stat/StatPhase.hpp>

namespace tdc {

template<typename text_t = TextDS<>>
class BWTCompressor : public Compressor {
public:
    inline static Meta meta() {
        Meta m(Compressor::type_desc(), "bwt",
            "Computes the Burrows-Wheeler transform of the input text.");
        m.param("textds", "The text data structure provider.")
            .strategy<text_t>(TypeDesc("textds"), Meta::Default<TextDS<>>());
        return m;
    }

    using Compressor::Compressor;

    inline virtual void compress(Input& input, Output& output) override {
        auto ostream = output.as_stream();
        auto in = input.as_view();

        text_t t(config().sub_config("textds"), in, text_t::SA);
		DVLOG(2) << vec_to_debug_string(t);
		const len_t input_size = t.size();

        StatPhase::wrap("Construct Text DS", [&]{
            t.require(text_t::SA);
            DVLOG(2) << vec_to_debug_string(t.require_sa());
        });

        const auto& sa = t.require_sa();
        for(size_t i = 0; i < input_size; ++i) {
            ostream << bwt::bwt(t,sa,i);
        }
    }

    inline std::unique_ptr<Decompressor> decompressor() const override {
        return Algorithm::unique_instance<BWTDecompressor>();
    }
};

}//ns

