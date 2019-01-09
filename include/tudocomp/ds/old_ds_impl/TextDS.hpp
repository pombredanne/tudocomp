#pragma once

#include <tudocomp/Error.hpp>
#include <tudocomp/Tags.hpp>

#include <tudocomp/ds/TextDSFlags.hpp>
#include <tudocomp/Algorithm.hpp>
#include <tudocomp/ds/IntVector.hpp>

#include <tudocomp/ds/CompressMode.hpp>

//Defaults
#include <tudocomp/ds/SADivSufSort.hpp>
#include <tudocomp/ds/PhiFromSA.hpp>
#include <tudocomp/ds/PLCPFromPhi.hpp>
#include <tudocomp/ds/LCPFromPLCP.hpp>
#include <tudocomp/ds/ISAFromSA.hpp>

namespace tdc {

static_assert(
    std::is_same<View::value_type, uliteral_t>::value,
    "View::value_type and uliteral_t must be the same");

/// Manages text related data structures.
template<
    typename sa_t = SADivSufSort,
    typename phi_t = PhiFromSA,
    typename plcp_t = PLCPFromPhi,
    typename lcp_t = LCPFromPLCP,
    typename isa_t = ISAFromSA
>
class TextDS : public Algorithm {
public:
    using dsflags_t = ds::dsflags_t;
    static const dsflags_t SA  = ds::SA;
    static const dsflags_t ISA = ds::ISA;
    static const dsflags_t LCP = ds::LCP;
    static const dsflags_t PHI = ds::PHI;
    static const dsflags_t PLCP = ds::PLCP;

    using value_type = uliteral_t;

    using sa_type = sa_t;
    using phi_type = phi_t;
    using plcp_type = plcp_t;
    using lcp_type = lcp_t;
    using isa_type = isa_t;

private:
    using this_t = TextDS<sa_t, phi_t, plcp_t, lcp_t, isa_t>;

    View m_text;

    std::unique_ptr<sa_t>  m_sa;
    std::unique_ptr<phi_t> m_phi;
    std::unique_ptr<plcp_t> m_plcp;
    std::unique_ptr<lcp_t> m_lcp;
    std::unique_ptr<isa_t> m_isa;

    dsflags_t m_ds_requested;
    CompressMode m_cm;

    template<typename ds_t>
    inline std::unique_ptr<ds_t> construct_ds(const std::string& option, CompressMode cm) {
        return std::make_unique<ds_t>(
                    config().sub_config(option),
                    *this,
                    cm_select(cm, m_cm));
    }

    template<typename ds_t>
    inline const ds_t& require_ds(
        std::unique_ptr<ds_t>& p, const std::string& option, CompressMode cm) {

        if(!p) p = construct_ds<ds_t>(option, cm_select(cm, m_cm));
        return *p;
    }

    template<typename ds_t>
    inline void discard_ds(std::unique_ptr<ds_t>& p, dsflags_t flag) {
        p.reset(nullptr);
    }

    template<typename ds_t>
    inline ds_t release_ds(std::unique_ptr<ds_t>& p, dsflags_t flag, string_ref err_msg) {
        DCHECK(p) << "TextDS did not contain a " << err_msg;
        return std::move(*p);
    }

    template<typename ds_t>
    inline typename ds_t::data_type inplace_ds(
        std::unique_ptr<ds_t>& p, dsflags_t flag, const std::string& option, CompressMode cm) {

        if(!p) p = construct_ds<ds_t>(option, cm_select(cm, m_cm));
        if(m_ds_requested & flag) {
            // data structure is requested, return a copy of the data
            return p->copy();
        } else {
            // relinquish data and discard ds
            auto data = p->relinquish();
            p.reset(nullptr);
            return std::move(data);
        }
    }

public:
    inline static Meta meta() {
        Meta m(TypeDesc("textds"), "textds", "Text data structure provider.");
        m.param("sa", "The suffix array implementation.")
            .strategy<sa_t>(TypeDesc("sa"), Meta::Default<SADivSufSort>());
        m.param("phi", "The Phi array implementation.")
            .strategy<phi_t>(TypeDesc("phi"), Meta::Default<PhiFromSA>());
        m.param("plcp", "The PLCP array implementation.")
            .strategy<plcp_t>(TypeDesc("plcp"), Meta::Default<PLCPFromPhi>());
        m.param("lcp", "The LCP array implementation.")
            .strategy<lcp_t>(TypeDesc("lcp"), Meta::Default<LCPFromPLCP>());
        m.param("isa", "The inverse suffix array implementation.")
            .strategy<isa_t>(TypeDesc("isa"), Meta::Default<ISAFromSA>());
        m.param("compress",
            "Compression Mode, one of:\n"
            "\"none\" - structures remain uncompressed (fastest)\n"
            "\"delayed\" - structures are bit-compressed after "
            "construction (balanced)\n"
            "\"compressed\" - structures are constructed in "
            "bit-compressed space (lowest memory consumption)"
        ).primitive("delayed");

        m.inherit_tags_from_all(
            tl::type_list<sa_t, phi_t, plcp_t, lcp_t, isa_t>());
        return m;
    }

    inline TextDS(Config&& cfg, const View& text)
        : Algorithm(std::move(cfg)),
          m_text(text), m_ds_requested(0) {

        if(meta().has_tag(tags::require_sentinel)){
            MissingSentinelError::check(m_text);
        }

        const auto& cm_str = this->config().param("compress").as_string();
        if(cm_str == "delayed") {
            m_cm = CompressMode::delayed;
        } else if(cm_str == "compressed") {
            m_cm = CompressMode::compressed;
        } else {
            m_cm = CompressMode::plain;
        }
    }

    inline TextDS(Config&& cfg, const View& text, dsflags_t flags,
        CompressMode cm = CompressMode::select)
        : TextDS(std::move(cfg), text) {

        require(flags, cm);
    }

    // require methods

    inline const sa_t& require_sa(CompressMode cm = CompressMode::select) {
        return require_ds(m_sa, "sa", cm);
    }
    inline const phi_t& require_phi(CompressMode cm = CompressMode::select) {
        return require_ds(m_phi, "phi", cm);
    }
    inline const plcp_t& require_plcp(CompressMode cm = CompressMode::select) {
        return require_ds(m_plcp, "plcp", cm);
    }
    inline const lcp_t& require_lcp(CompressMode cm = CompressMode::select) {
        return require_ds(m_lcp, "lcp", cm);
    }
    inline const isa_t& require_isa(CompressMode cm = CompressMode::select) {
        return require_ds(m_isa, "isa", cm);
    }

    // inplace methods

    inline typename sa_t::data_type inplace_sa(
        CompressMode cm = CompressMode::select) {

        return inplace_ds(m_sa, SA, "sa", cm);
    }
    inline typename phi_t::data_type inplace_phi(
        CompressMode cm = CompressMode::select) {

        return inplace_ds(m_phi, PHI, "phi", cm);
    }
    inline typename plcp_t::data_type inplace_plcp(
        CompressMode cm = CompressMode::select) {

        return inplace_ds(m_plcp, PLCP, "plcp", cm);
    }
    inline typename lcp_t::data_type inplace_lcp(
        CompressMode cm = CompressMode::select) {

        return inplace_ds(m_lcp, LCP, "lcp", cm);
    }
    inline typename isa_t::data_type inplace_isa(
        CompressMode cm = CompressMode::select) {

        return inplace_ds(m_isa, ISA, "isa", cm);
    }

    // release methods

    inline sa_t release_sa() {
        return release_ds(m_sa, SA, "SA");
    }
    inline phi_t release_phi() {
        return release_ds(m_phi, PHI, "PHI");
    }
    inline plcp_t release_plcp() {
        return release_ds(m_plcp, PLCP, "PLCP");
    }
    inline lcp_t release_lcp() {
        return release_ds(m_lcp, LCP, "LCP");
    }
    inline isa_t release_isa() {
        return release_ds(m_isa, ISA, "ISA");
    }

private:
    inline void discard_sa() {
        discard_ds(m_sa, SA);
    }
    inline void discard_phi() {
        discard_ds(m_phi, PHI);
    }
    inline void discard_plcp() {
        discard_ds(m_plcp, PLCP);
    }
    inline void discard_lcp() {
        discard_ds(m_lcp, LCP);
    }
    inline void discard_isa() {
        discard_ds(m_isa, ISA);
    }

    inline void discard_unneeded() {
        // discard unrequested structures
        if(!(m_ds_requested & SA)) discard_sa();
        if(!(m_ds_requested & PHI)) discard_phi();
        if(!(m_ds_requested & PLCP)) discard_plcp();
        if(!(m_ds_requested & LCP)) discard_lcp();
        if(!(m_ds_requested & ISA)) discard_isa();
    }

public:
    inline void require(dsflags_t flags, CompressMode cm = CompressMode::select) {
        m_ds_requested = flags;

        // TODO: we need something like a dependency graph here

        // construct requested structures
        cm = cm_select(cm,(m_cm == CompressMode::delayed ?
                                        CompressMode::coherent_delayed : m_cm));


        // Construct SA (don't compress yet)
        if(flags & SA) { require_sa(cm); discard_unneeded(); }

        // Construct Phi (don't compress yet)
        if(flags & PHI) { require_phi(cm); discard_unneeded(); }

        // Construct PLCP and compress if LCP is not requested
        if(flags & PLCP) {
            require_plcp(cm);
            discard_unneeded();
            if(cm == CompressMode::coherent_delayed && !(flags & LCP)) {
                m_plcp->compress();
            }
        }

        // Construct and compress LCP
        if(flags & LCP)  {
            require_lcp(cm);
            discard_unneeded();
            if(cm == CompressMode::coherent_delayed) m_lcp->compress();
        }

        // Construct and compress ISA
        if(flags & ISA)  {
            require_isa(cm);
            discard_unneeded();
            if(cm == CompressMode::coherent_delayed) m_isa->compress();
        }

        // Compress data structures that had dependencies
        if(cm == CompressMode::coherent_delayed) {
            if(m_sa) m_sa->compress();
            if(m_phi) m_phi->compress();
            if(m_plcp) m_plcp->compress();
        }
    }

    /// Accesses the input text at position i.
    inline value_type operator[](size_t i) const {
        return m_text[i];
    }

    /// Provides direct access to the input text.
    inline const value_type* text() const {
        return m_text.data();
    }

    /// Returns the size of the input text.
    inline size_t size() const {
        return m_text.size();
    }

    inline void print(std::ostream& out, size_t base) {
        size_t w = std::max(8UL, (size_t)std::log10((double)size()) + 1);
        out << std::setfill(' ');

        //Heading
        out << std::setw(w) << "i" << " | ";
        if(m_sa) out << std::setw(w) << "SA[i]" << " | ";
        if(m_phi) out << std::setw(w) << "Phi[i]" << " | ";
        if(m_plcp) out << std::setw(w) << "PLCP[i]" << " | ";
        if(m_lcp) out << std::setw(w) << "LCP[i]" << " | ";
        if(m_isa) out << std::setw(w) << "ISA[i]" << " | ";
        out << std::endl;

        //Separator
        out << std::setfill('-');
        out << std::setw(w) << "" << "-|-";
        if(m_sa) out << std::setw(w) << "" << "-|-";
        if(m_phi) out << std::setw(w) << "" << "-|-";
        if(m_plcp) out << std::setw(w) << "" << "-|-";
        if(m_lcp) out << std::setw(w) << "" << "-|-";
        if(m_isa) out << std::setw(w) << "" << "-|-";
        out << std::endl;

        //Body
        out << std::setfill(' ');
        for(size_t i = 0; i < size(); i++) {
            out << std::setw(w) << (i + base) << " | ";
            if(m_sa) out << std::setw(w) << ((*m_sa)[i] + base) << " | ";
            if(m_phi) out << std::setw(w) << (*m_phi)[i] << " | ";
            if(m_plcp) out << std::setw(w) << (*m_plcp)[i] << " | ";
            if(m_lcp) out << std::setw(w) << (*m_lcp)[i] << " | ";
            if(m_isa) out << std::setw(w) << ((*m_isa)[i] + base) << " | ";
            out << std::endl;
        }
    }
};

} //ns