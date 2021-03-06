#pragma once

#include <tudocomp/util.hpp>
#include <tudocomp/Env.hpp>
#include <tudocomp/Compressor.hpp>
#include <tudocomp/ds/IntVector.hpp>

#include <tudocomp/compressors/esp/landmarks.hpp>
#include <tudocomp/compressors/esp/utils.hpp>
#include <tudocomp/compressors/esp/esp_math.hpp>
#include <tudocomp/compressors/esp/RoundContext.hpp>

namespace tdc {namespace esp {

template<typename Source>
class MetablockContext {
    RoundContext<Source>* m_parent;
public:
    RoundContext<Source>& rctx() {
        return *m_parent;
    }

    DebugMetablockContext debug;

    MetablockContext(RoundContext<Source>& ctx, DebugMetablockContext dbg):
        m_parent(&ctx),
        debug(dbg) {}

    void push_block(size_t width, size_t type) {
        debug.block(width, type);
        rctx().push_back(width, type);
    }

    inline void eager_mb13(const Source& src, size_t t) {
        [&]() {
            size_t j = src.size();
            for (size_t i = 0; i < j;) {
                size_t remaining_len = j - i;
                switch (remaining_len) {
                    case 4:
                        push_block(2, t);
                        push_block(2, t);
                        return;
                    case 3:
                        push_block(3, t);
                        return;
                    case 2:
                        push_block(2, t);
                        return;
                    case 1:
                        push_block(1, t);
                        //DCHECK_GT(remaining_len, 1);
                        return;
                    case 0:
                        return;
                    default:
                        push_block(3, t);
                        i += 3;
                }
            }
        }();
        rctx().debug_check_advanced(src.size());
    }

    inline void eager_mb2(const Source& src) {
        auto& ctx = rctx();

        auto A = src;
        DCHECK(A.size() > 0);
        auto type_3_prefix_len = std::min(iter_log(ctx.alphabet_size),
                                        A.size());

        // Handle non-m2 prefix
        {
            auto type_3_prefix = A.slice(0, type_3_prefix_len);
            eager_mb13(type_3_prefix, 3);
            if (type_3_prefix_len == A.size()) { return; }
        }

        auto type_2_suffix_size = src.size() - type_3_prefix_len;

        // Prepare scratchpad buffer
        auto& buf = ctx.scratchpad;
        buf.clear();
        buf.reserve(A.cend() - A.cbegin());
        buf.insert(buf.cbegin(), A.cbegin(), A.cend());

        // Iterate on the buffer by combing each two adjacent elements.
        // This reduces the size by `iter_log(alphabet_size) == type_3_prefix_len`
        // the alphabet to size 6.
        {
            debug.mb2_initial(buf);
            debug.mb2_reduce_to_6_start();
            for (uint shrink_i = 0; shrink_i < type_3_prefix_len; shrink_i++) {
                for (size_t i = 1; i < buf.size(); i++) {
                    auto left  = buf[i - 1];
                    auto right = buf[i];
                    buf[i - 1] = label(left, right);
                }
                buf.pop_back();

                debug.mb2_reduce_to_6_step(buf);
            }

            DCHECK_LE(calc_alphabet_size(buf), 6);
        }

        // Reduce further to alphabet 3
        {
            debug.mb2_reduce_to_3_start();

            // final pass: reduce to alphabet 3
            for(uint to_replace = 3; to_replace < 6; to_replace++) {
                do_for_neighbors(buf, [&](size_t i, ConstGenericView<size_t> neighbors) {
                    auto& e = buf[i];
                    if (e == to_replace) {
                        e = 0;
                        for (auto n : neighbors) { if (n == e) { e++; } }
                        for (auto n : neighbors) { if (n == e) { e++; } }
                    }
                });

                debug.mb2_reduce_to_3_step(buf);
            }

            DCHECK(calc_alphabet_size(buf) <= 3);
            DCHECK(no_adjacent_identical(buf));
        }

        // find landmarks:
        {
            // TODO: Maybe store in high bits of buf to reduce memory?
            // buf gets reduced to 2 bit values anyway, and stays around long enough
            IntVector<uint_t<1>> landmarks(buf.size());

            do_for_neighbors(buf, [&](size_t i, ConstGenericView<size_t> neighbors) {
                bool is_high_landmark = true;
                for (auto e : neighbors) {
                    if (e > buf[i]) {
                        is_high_landmark = false;
                    }
                }
                if (is_high_landmark) {
                    landmarks[i] = 1;
                }
            });

            debug.mb2_high_landmarks(landmarks);

            do_for_neighbors(buf, [&](size_t i, ConstGenericView<size_t> neighbors) {
                bool is_low_landmark = true;
                for (auto e : neighbors) {
                    if (e < buf[i]) {
                        is_low_landmark = false;
                    }
                }
                // if there is a large enough landmark-less gap, mark it as well
                if (is_low_landmark) {
                    //if (i > 0 && i < buf.size() - 1)
                    if (   (!(i > 0)              || (landmarks[i - 1] == 0u))
                        && (!(i < buf.size() - 1) || (landmarks[i + 1] == 0u))
                    ) {
                        landmarks[i] = 1;
                    }
                }
            });

            debug.mb2_high_and_low_landmarks(landmarks);

            DCHECK(check_landmarks(landmarks, true));

            // Split at landmarks

            landmark_spanner(
                landmarks.size(),
                [&](size_t i) {
                    return landmarks[i] == uint_t<1>(1);
                },
                [&](size_t left, size_t right) {
                    push_block(right - left + 1, 2);
                },
                ctx.behavior_landmarks_tie_to_right
            );
        }

        ctx.debug_check_advanced(type_2_suffix_size);
    }
};

}}
