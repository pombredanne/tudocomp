#pragma once

#include <string>
#include <vector>

namespace tdc {

using dsid_t = int;
using dsid_list_t = std::vector<dsid_t>;

namespace ds {
    static constexpr dsid_t SUFFIX_ARRAY  = 0x7DC01;
    static constexpr dsid_t INVERSE_SUFFIX_ARRAY = 0x7DC02;
    static constexpr dsid_t LCP_ARRAY = 0x7DC03;
    static constexpr dsid_t PHI_ARRAY = 0x7DC04;
    static constexpr dsid_t PLCP_ARRAY = 0x7DC05;

    inline std::string name_for(dsid_t id) {
        switch(id) {
            case SUFFIX_ARRAY:         return "suffix_array";
            case INVERSE_SUFFIX_ARRAY: return "inverse_suffix_array";
            case LCP_ARRAY:            return "lcp_array";
            case PHI_ARRAY:            return "phi_array";
            case PLCP_ARRAY:           return "plcp_array";
            default:
                return std::string("#") + std::to_string(id);
        }
    }
}

}
