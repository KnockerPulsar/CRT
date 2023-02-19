#pragma once

/* 
 * To be able to use a struct in both OpenCL and C++ comfortably.
 *
 *
 * Example:
 * SHARED_STRUCT_START(Foo) {
 *  ... // Foo impl.
 * } SHARED_STRUCT_END(Foo);
 * 
 *
 * Expands to (on OpenCL's side):
 * typedef struct {
 *  ... // Foo impl.
 * } Foo;
 * 
 *
 * Or on C++'s side:
 * struct Foo {
 *  ... // Foo impl.
 * };
 */

#ifdef OPENCL 

#define SHARED_STRUCT_START(s) typedef struct 
#define SHARED_STRUCT_END(s) s

#else 

#define SHARED_STRUCT_START(s) struct s 
#define SHARED_STRUCT_END(s) // Nothing

#endif

