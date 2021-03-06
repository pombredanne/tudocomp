#pragma once

#include <tudocomp/Meta.hpp>
#include <tudocomp/Env.hpp>

namespace tdc {

/// \brief Interface for algorithms.
///
/// This is the base for classes that use an environment (\ref Env) to receive
/// options or communicate with the framework in different ways.
///
/// Algorithms are required to implement also a static function \c meta() that
/// returns a Meta object, containing information about the algorithm.
class Algorithm {
    Env m_env;
public:
    virtual ~Algorithm() = default;
    Algorithm(Algorithm const&) = default;
    Algorithm(Algorithm&&) = default;
    Algorithm& operator=(Algorithm const&) = default;
    Algorithm& operator=(Algorithm&&) = default;

    /// \cond DELETED
    inline Algorithm() = delete;
    /// \endcond

    /// \brief Instantiates an algorithm in the specified environment.
    ///
    /// \param env The environment for the algorithm to work in.
    inline Algorithm(Env&& env): m_env(std::move(env)) {
        /*
            //FIXME:

            The following spooky lines of code are supposed to do absolutely
            nothing except tell the compilation framework that we are really
            serious about requiring "malloc".

            Not including an explicit call of "malloc" somewhere will cause the
            compiler not to link against our custom "malloc" implementation in
            the "tudocomp_stat" module.

            Until we know a better way to ensure this, this should stay.
        */
        volatile void* x = malloc(0);
        if(x) { free((void*)x); x = nullptr; }
    }

    /// \brief Provides access to the environment that the algorithm works in.
    /// \return The environment that the algorithm works in.
    inline Env& env() { return m_env; }
    inline const Env& env() const { return m_env; }
};

}

