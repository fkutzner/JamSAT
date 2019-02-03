/* Copyright (c) 2017,2018 Felix Kutzner (github.com/fkutzner)

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.

 Except as contained in this notice, the name(s) of the above copyright holders
 shall not be used in advertising or otherwise to promote the sale, use or
 other dealings in this Software without prior written authorization.

*/

/*
 This file exports the IPASIR API defined by Tomas Balyo and Armin Biere
 (see http://github.com/biotomas/ipasir).

 Note: You can use JamSAT by including http://github.com/biotomas/ipasir and
 linking against the JamSAT library works. There are two benefits of using this
 header when linking against JamSAT:
  - improved visibility information
  - optimized calls when linking against the JamSAT DLL on Windows

 For documentation of the ipasir_* functions, see http://github.com/biotomas/ipasir
*/

/* TODO: worry about the calling convention on Windows */

#ifndef JAMSAT_IPASIR_H_INCLUDED
#define JAMSAT_IPASIR_H_INCLUDED

/* clang-format off */
#if defined(JAMSAT_SHARED_LIB)
    #if defined(_WIN32) || defined(__CYGWIN__)
        #if defined(BUILDING_JAMSAT_SHARED_LIB)
            #if defined(__GNUC__)
                #define JAMSAT_PUBLIC_API __attribute__((dllexport))
            #elif defined(_MSC_VER)
                #define JAMSAT_PUBLIC_API __declspec(dllexport)
            #endif
        #else
            #if defined(__GNUC__)
                #define JAMSAT_PUBLIC_API __attribute__((dllimport))
            #elif defined(_MSC_VER)
                #define JAMSAT_PUBLIC_API __declspec(dllimport)
            #endif
        #endif
    #elif defined(__GNUC__)
        #define JAMSAT_PUBLIC_API __attribute__((visibility("default")))
    #endif

    #if !defined(JAMSAT_PUBLIC_API)
        #warning "Unknown compiler. Not adding visibility information to exported symbols."
    #endif
#else
    #define JAMSAT_PUBLIC_API
#endif
/* clang-format on */


#if defined(__cplusplus)
extern "C" {
#endif

extern JAMSAT_PUBLIC_API const char* ipasir_signature();
extern JAMSAT_PUBLIC_API void* ipasir_init();
extern JAMSAT_PUBLIC_API void ipasir_release(void* solver);
extern JAMSAT_PUBLIC_API void ipasir_add(void* solver, int lit_or_zero);
extern JAMSAT_PUBLIC_API void ipasir_assume(void* solver, int lit);
extern JAMSAT_PUBLIC_API int ipasir_solve(void* solver);
extern JAMSAT_PUBLIC_API int ipasir_val(void* solver, int lit);
extern JAMSAT_PUBLIC_API int ipasir_failed(void* solver, int lit);
extern JAMSAT_PUBLIC_API void
ipasir_set_terminate(void* solver, void* state, int (*terminate)(void* state));
extern JAMSAT_PUBLIC_API void ipasir_set_learn(void* solver,
                                               void* state,
                                               int max_length,
                                               void (*learn)(void* state, int* clause));

#if defined(__cplusplus)
}
#endif

#endif
