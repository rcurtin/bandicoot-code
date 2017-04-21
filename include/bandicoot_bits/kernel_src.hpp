// Copyright 2017 Conrad Sanderson (http://conradsanderson.id.au)
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ------------------------------------------------------------------------



struct kernel_src
  {
  static inline const std::string&  get_source();
  static inline       std::string  init_source();
  };



inline
const std::string&
kernel_src::get_source()
  {
  static const std::string source = init_source();
  
  return source;
  }



// TODO: inplace_set_scalar() could be replaced with explicit call to clEnqueueFillBuffer()
// present in OpenCL 1.2: http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clEnqueueFillBuffer.html

// TODO: need submat analogues of all functions

// TODO: need specialised handling for cx_float and cx_double
// for example (cx_double * cx_double) is not simply (double2 * double2)
  

inline
std::string
kernel_src::init_source()
  {
  // NOTE: kernel names must match the list in the kernel_id struct
  
  std::string source = \
  "#ifdef cl_khr_pragma_unroll \n"
  "#pragma OPENCL EXTENSION cl_khr_pragma_unroll : enable \n"
  "#endif \n"
  "#ifdef cl_amd_pragma_unroll \n"
  "#pragma OPENCL EXTENSION cl_amd_pragma_unroll : enable \n"
  "#endif \n"
  "#ifdef cl_nv_pragma_unroll \n"
  "#pragma OPENCL EXTENSION cl_nv_pragma_unroll : enable \n"
  "#endif \n"
  "#ifdef cl_intel_pragma_unroll \n"
  "#pragma OPENCL EXTENSION cl_intel_pragma_unroll : enable \n"
  "#endif \n"
  "\n"
  "#define COOT_FN2(ARG1,ARG2)  ARG1 ## ARG2 \n"
  "#define COOT_FN(ARG1,ARG2) COOT_FN2(ARG1,ARG2) \n"
  "\n"
  "__kernel void COOT_FN(PREFIX,inplace_set_scalar)(__global eT* out, const eT val, const UWORD N) \n"
  "  { \n"
  "  const UWORD i = get_global_id(0); \n"
  "  if(i < N)  { out[i] = val; } \n"
  "  } \n"
  "\n"
  "__kernel void COOT_FN(PREFIX,inplace_plus_scalar)(__global eT* out, const eT val, const UWORD N) \n"
  "  { \n"
  "  const UWORD i = get_global_id(0); \n"
  "  if(i < N)  { out[i] += val; } \n"
  "  } \n"
  "\n"
  "__kernel void COOT_FN(PREFIX,inplace_minus_scalar)(__global eT* out, const eT val, const UWORD N) \n"
  "  { \n"
  "  const UWORD i = get_global_id(0); \n"
  "  if(i < N)  { out[i] -= val; } \n"
  "  } \n"
  "\n"
  "__kernel void COOT_FN(PREFIX,inplace_mul_scalar)(__global eT* out, const eT val, const UWORD N) \n"
  "  { \n"
  "  const UWORD i = get_global_id(0); \n"
  "  if(i < N)  { out[i] *= val; } \n"
  "  } \n"
  "\n"
  "__kernel void COOT_FN(PREFIX,inplace_div_scalar)(__global eT* out, const eT val, const UWORD N) \n"
  "  { \n"
  "  const UWORD i = get_global_id(0); \n"
  "  if(i < N)  { out[i] /= val; } \n"
  "  } \n"
  "\n"
  "__kernel void COOT_FN(PREFIX,inplace_plus_array)(__global eT* out, __global const eT* A, const UWORD N) \n"
  "  { \n"
  "  const UWORD i = get_global_id(0); \n"
  "  if(i < N)  { out[i] += A[i]; } \n"
  "  } \n"
  "\n"
  "__kernel void COOT_FN(PREFIX,inplace_minus_array)(__global eT* out, __global const eT* A, const UWORD N) \n"
  "  { \n"
  "  const UWORD i = get_global_id(0); \n"
  "  if(i < N)  { out[i] -= A[i]; } \n"
  "  } \n"
  "\n"
  "__kernel void COOT_FN(PREFIX,inplace_mul_array)(__global eT* out, __global const eT* A, const UWORD N) \n"
  "  { \n"
  "  const UWORD i = get_global_id(0); \n"
  "  if(i < N)  { out[i] *= A[i]; } \n"
  "  } \n"
  "\n"
  "__kernel void COOT_FN(PREFIX,inplace_div_array)(__global eT* out, __global const eT* A, const UWORD N) \n"
  "  { \n"
  "  const UWORD i = get_global_id(0); \n"
  "  if(i < N)  { out[i] /= A[i]; } \n"
  "  } \n"
  "\n"
  "__kernel void COOT_FN(PREFIX,equ_array_plus_scalar)(__global eT* out, __global const eT* A, const eT val, const UWORD N) \n"
  "  { \n"
  "  const UWORD i = get_global_id(0); \n"
  "  if(i < N)  { out[i] = A[i] + val; } \n"
  "  } \n"
  "\n"
  "__kernel void COOT_FN(PREFIX,equ_array_neg)(__global eT* out, __global const eT* A, const eT val, const UWORD N) \n"
  "  { \n"
  "  (void)(val); \n"
  "  const UWORD i = get_global_id(0); \n"
  "  if(i < N)  { out[i] = -(A[i]); } \n"
  "  } \n"
  "\n"
  "__kernel void COOT_FN(PREFIX,equ_array_minus_scalar_pre)(__global eT* out, __global const eT* A, const eT val, const UWORD N) \n"
  "  { \n"
  "  const UWORD i = get_global_id(0); \n"
  "  if(i < N)  { out[i] = val - A[i]; } \n"
  "  } \n"
  "\n"
  "__kernel void COOT_FN(PREFIX,equ_array_minus_scalar_post)(__global eT* out, __global const eT* A, const eT val, const UWORD N) \n"
  "  { \n"
  "  const UWORD i = get_global_id(0); \n"
  "  if(i < N)  { out[i] = A[i] - val; } \n"
  "  } \n"
  "\n"
  "__kernel void COOT_FN(PREFIX,equ_array_mul_scalar)(__global eT* out, __global const eT* A, const eT val, const UWORD N) \n"
  "  { \n"
  "  const UWORD i = get_global_id(0); \n"
  "  if(i < N)  { out[i] = A[i] * val; } \n"
  "  } \n"
  "\n"
  "__kernel void COOT_FN(PREFIX,equ_array_div_scalar_pre)(__global eT* out, __global const eT* A, const eT val, const UWORD N) \n"
  "  { \n"
  "  const UWORD i = get_global_id(0); \n"
  "  if(i < N)  { out[i] = val / A[i]; } \n"
  "  } \n"
  "\n"
  "__kernel void COOT_FN(PREFIX,equ_array_div_scalar_post)(__global eT* out, __global const eT* A, const eT val, const UWORD N) \n"
  "  { \n"
  "  const UWORD i = get_global_id(0); \n"
  "  if(i < N)  { out[i] = A[i] / val; } \n"
  "  } \n"
  "\n"
  "__kernel void COOT_FN(PREFIX,equ_array_square)(__global eT* out, __global const eT* A, const eT val, const UWORD N) \n"
  "  { \n"
  "  (void)(val); \n"
  "  const UWORD i = get_global_id(0); \n"
  "  if(i < N)  { const eT Ai = A[i]; out[i] = Ai * Ai; } \n"
  "  } \n"
  "\n"
  "__kernel void COOT_FN(PREFIX,equ_array_sqrt)(__global eT* out, __global const eT* A, const eT val, const UWORD N) \n"
  "  { \n"
  "  (void)(val); \n"
  "  const UWORD i = get_global_id(0); \n"
  "  if(i < N)  { out[i] = (eT)sqrt( (promoted_eT)(A[i]) ); } \n"
  "  } \n"
  "\n"
  "__kernel void COOT_FN(PREFIX,equ_array_plus_array)(__global eT* out, __global const eT* A, __global const eT* B, const UWORD N) \n"
  "  { \n"
  "  const UWORD i = get_global_id(0); \n"
  "  if(i < N)  { out[i] = A[i] + B[i]; } \n"
  "  } \n"
  "\n"
  "__kernel void COOT_FN(PREFIX,equ_array_minus_array)(__global eT* out, __global const eT* A, __global const eT* B, const UWORD N) \n"
  "  { \n"
  "  const UWORD i = get_global_id(0); \n"
  "  if(i < N)  { out[i] = A[i] - B[i]; } \n"
  "  } \n"
  "\n"
  "__kernel void COOT_FN(PREFIX,equ_array_mul_array)(__global eT* out, __global const eT* A, __global const eT* B, const UWORD N) \n"
  "  { \n"
  "  const UWORD i = get_global_id(0); \n"
  "  if(i < N)  { out[i] = A[i] * B[i]; } \n"
  "  } \n"
  "\n"
  "__kernel void COOT_FN(PREFIX,equ_array_div_array)(__global eT* out, __global const eT* A, __global const eT* B, const UWORD N) \n"
  "  { \n"
  "  const UWORD i = get_global_id(0); \n"
  "  if(i < N)  { out[i] = A[i] / B[i]; } \n"
  "  } \n"
  "\n"
  "__kernel void COOT_FN(PREFIX,sum_all)(__global eT* out, __global const eT* A, const UWORD start, const UWORD end) \n"
  "  { \n"
  "  const UWORD id = get_global_id(0); \n"
  "  if(id == 0) \n"
  "    { \n"
  "    eT acc = (eT)(0); \n"
  "    #pragma unroll \n"
  "    for(UWORD i=start; i<=end; ++i) \n"
  "      { acc += A[i]; } \n"
  "    out[0] = acc; \n"
  "    } \n"
  "  } \n"
  "\n"
  "__kernel void COOT_FN(PREFIX,sum_colwise)(__global eT* out, __global const eT* A, const UWORD n_rows, const UWORD n_cols) \n"
  "  { \n"
  "  const UWORD col = get_global_id(0); \n"
  "  if(col < n_cols) \n"
  "    { \n"
  "    __global const eT* colptr = &(A[ col*n_rows ]); \n"
  "    eT acc = (eT)(0); \n"
  "    #pragma unroll \n"
  "    for(UWORD i=0; i<n_rows; ++i) \n"
  "      { acc += colptr[i]; } \n"
  "    out[col] = acc; \n"
  "    } \n"
  "  } \n"
  "\n"
  "__kernel void COOT_FN(PREFIX,sum_rowwise)(__global eT* out, __global const eT* A, const UWORD n_rows, const UWORD n_cols) \n"
  "  { \n"
  "  const UWORD row = get_global_id(0); \n"
  "  if(row < n_rows) \n"
  "    { \n"
  "    eT acc = (eT)(0); \n"
  "    #pragma unroll \n"
  "    for(UWORD i=0; i<n_cols; ++i) \n"
  "      { acc += A[ row + i*n_rows ]; } \n"
  "    out[row] = acc; \n"
  "    } \n"
  "  } \n"
  "\n"
  "__kernel void COOT_FN(PREFIX,submat_sum_colwise)(__global eT* out, __global const eT* A, const UWORD n_rows, const UWORD n_cols, const UWORD start_col, const UWORD start_row, const UWORD end_row) \n"
  "  { \n"
  "  const UWORD col = get_global_id(0); \n"
  "  if(col < n_cols) \n"
  "    { \n"
  "    __global const eT* colptr = &(A[ col*n_rows ]); \n"
  "    eT acc = (eT)(0); \n"
  "    #pragma unroll \n"
  "    for(UWORD i=start_row; i<=end_row; ++i) \n"
  "      { acc += colptr[i]; } \n"
  "    out[col-start_col] = acc; \n"
  "    } \n"
  "  } \n"
  "\n"
  "__kernel void COOT_FN(PREFIX,submat_sum_rowwise)(__global eT* out, __global const eT* A, const UWORD n_rows, const UWORD n_cols, const UWORD start_row, const UWORD start_col, const UWORD end_col) \n"
  "  { \n"
  "  const UWORD row = get_global_id(0); \n"
  "  if(row < n_rows) \n"
  "    { \n"
  "    eT acc = (eT)(0); \n"
  "    #pragma unroll \n"
  "    for(UWORD i=start_col; i<=end_col; ++i) \n"
  "      { acc += A[ row + i*n_rows ]; } \n"
  "    out[row-start_row] = acc; \n"
  "    } \n"
  "  } \n"
  "\n"
  ;
  
  return source;
  }