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



inline
coot_runtime_t::~coot_runtime_t()
  {
  coot_extra_debug_sigprint_this(this);
  
  cleanup_cl();
  }



inline
coot_runtime_t::coot_runtime_t()
  {
  coot_extra_debug_sigprint_this(this);
  
  platform_id = NULL;
  device_id   = NULL;
  context     = NULL;
  cq          = NULL;
  
  valid              = false;
  device_64bit_sizet = false;
  device_64bit_float = false;
  
  
  bool status = false;
  
  std::string errmsg;
  
  status = init_cl(errmsg, false, 0, 0);
  
  if(status == false)
    {
    std::stringstream ss;
    
    ss << "coot_runtime: couldn't setup OpenCL: " << errmsg;
    coot_warn(ss.str());
    }
  
  interrogate_device(true);
  
  if(status != false)
    {
    status = init_kernels<u32>(u32_kernels, kernel_src::get_source(), kernel_id::get_names());
    if(status == false)  { coot_warn("coot_runtime: couldn't setup OpenCL kernels"); }
    
    status = init_kernels<s32>(s32_kernels, kernel_src::get_source(), kernel_id::get_names());
    if(status == false)  { coot_warn("coot_runtime: couldn't setup OpenCL kernels"); }
    
    status = init_kernels<u64>(u64_kernels, kernel_src::get_source(), kernel_id::get_names());
    if(status == false)  { coot_warn("coot_runtime: couldn't setup OpenCL kernels"); }
    
    status = init_kernels<s64>(s64_kernels, kernel_src::get_source(), kernel_id::get_names());
    if(status == false)  { coot_warn("coot_runtime: couldn't setup OpenCL kernels"); }
    
    status = init_kernels<float>(f_kernels, kernel_src::get_source(), kernel_id::get_names());
    if(status == false)  { coot_warn("coot_runtime: couldn't setup OpenCL kernels"); }
    }
  
  // TODO: determine if 64 bit floats are supported
  // TODO: if 64 bit floats are supported, initialise double kernels
  
  if(status != false)
    {
    // TODO: make call to clblasSetup() conditional on clBLAS being available
    cl_int clblas_status = clblasSetup();
    
    if(clblas_status != CL_SUCCESS)  { coot_warn("coot_runtime: couldn't setup clBLAS"); }
    }
  
  if(status == true)
    {
    (*this).valid = true;
    }
  else
    {
    cleanup_cl();
    }
  }



inline
void
coot_runtime_t::lock()
  {
  coot_extra_debug_sigprint();
  
  #if defined(COOT_USE_CXX11)
    {
    coot_extra_debug_print("calling mutex.lock()");
    mutex.lock();
    }
  #endif
  }



inline
void
coot_runtime_t::unlock()
  {
  coot_extra_debug_sigprint();
  
  #if defined(COOT_USE_CXX11)
    {
    coot_extra_debug_print("calling mutex.unlock()");
    mutex.unlock();
    }
  #endif
  }



inline
void
coot_runtime_t::cleanup_cl()
  {
  coot_extra_debug_sigprint();
  
  valid = false;
  
  if(cq != NULL)  { clFinish(cq); }
  
  clblasTeardown();  // TODO: make this conditional on clBLAS being available
  
  // TODO: go through each kernel vector
  
  const uword f_kernels_size = f_kernels.size();
  
  for(uword i=0; i<f_kernels_size; ++i)  { if(f_kernels.at(i) != NULL)  { clReleaseKernel(f_kernels.at(i)); } }
  
  if(cq      != NULL)  { clReleaseCommandQueue(cq); cq      = NULL; }
  if(context != NULL)  { clReleaseContext(context); context = NULL; }
  }



inline
bool
coot_runtime_t::init_cl(std::string& out_errmsg, const bool manual_selection, const uword wanted_platform_id, const uword wanted_device_id)
  {
  coot_extra_debug_sigprint();
  
  cl_int  status      = 0;
  cl_uint n_platforms = 0;
  
  status = clGetPlatformIDs(0, NULL, &n_platforms);
  
  if((status != CL_SUCCESS) || (n_platforms == 0))
    {
    out_errmsg = "no OpenCL platforms available";
    return false;
    }
  
  std::vector<cl_platform_id> platform_ids(n_platforms);
  
  status = clGetPlatformIDs(n_platforms, &(platform_ids[0]), NULL);
  
  if(status != CL_SUCCESS)
    {
    out_errmsg = "couldn't get info on OpenCL platforms";
    return false;
    }
  

  // go through each platform
  
  std::vector< std::vector<cl_device_id> > device_ids(n_platforms);
  std::vector< std::vector<int         > > device_pri(n_platforms);  // device priorities
  
  for(size_t platform_count = 0; platform_count < n_platforms; ++platform_count)
    {
    cl_platform_id tmp_platform_id = platform_ids.at(platform_count);
    
    cl_uint local_n_devices = 0;
    
    status = clGetDeviceIDs(tmp_platform_id, CL_DEVICE_TYPE_ALL, 0, NULL, &local_n_devices);
    
    if((status != CL_SUCCESS) || (local_n_devices == 0))
      {
      continue;  // go to the next platform
      }
    
    std::vector<cl_device_id>& local_device_ids = device_ids.at(platform_count);
    std::vector<int>&          local_device_pri = device_pri.at(platform_count);
    
    local_device_ids.resize(local_n_devices);
    local_device_pri.resize(local_n_devices);
    
    status = clGetDeviceIDs(tmp_platform_id, CL_DEVICE_TYPE_ALL, local_n_devices, &(local_device_ids[0]), NULL);
    
    // go through each device on this platform
    for(size_t local_device_count = 0; local_device_count < local_n_devices; ++local_device_count)
      {
      local_device_pri.at(local_device_count) = 0;
      
      cl_device_id local_device_id = local_device_ids.at(local_device_count);
      
      cl_device_type      device_type_val = 0;
      cl_device_fp_config device_fp64_val = 0;
      
      char device_vstr[1024];
      
      status = 0;
      
      status |= clGetDeviceInfo(local_device_id, CL_DEVICE_TYPE,             sizeof(cl_device_type),      &device_type_val, NULL);
      status |= clGetDeviceInfo(local_device_id, CL_DEVICE_DOUBLE_FP_CONFIG, sizeof(cl_device_fp_config), &device_fp64_val, NULL);
      status |= clGetDeviceInfo(local_device_id, CL_DEVICE_VERSION,          sizeof(device_vstr),         &device_vstr,     NULL);
      
      if(status != CL_SUCCESS)
        {
        out_errmsg = "couldn't get device info";
        return false;
        }
      
      // extract OpenCL version
      device_vstr[ sizeof(device_vstr)-1 ] = char(0);  // ensure the array is null terminated
      const std::string tmp_str(device_vstr);
      
      // find the gaps in the version string, in case each version number becomes more than one digit
      
      std::string::size_type skip = tmp_str.find_first_of(" ");  // skip "OpenCL" at the start
      
      std::string::size_type major_ver_start = tmp_str.find_first_not_of(" ", skip           );
      std::string::size_type major_ver_end   = tmp_str.find_first_of    (".", major_ver_start);
      std::string::size_type minor_ver_start = tmp_str.find_first_not_of(".", major_ver_end  );
      std::string::size_type minor_ver_end   = tmp_str.find_first_of    (" ", minor_ver_start);
      
      if( (skip == std::string::npos) || (major_ver_start == std::string::npos) || (major_ver_end == std::string::npos) || (minor_ver_end == std::string::npos) )
        {
        out_errmsg = "device has garbled OpenCL version string";
        return false;
        }
      
      major_ver_end--;
      minor_ver_end--;
      
      std::istringstream major_ver_ss( tmp_str.substr(major_ver_start, major_ver_end - major_ver_start + 1) );
      std::istringstream minor_ver_ss( tmp_str.substr(minor_ver_start, minor_ver_end - minor_ver_start + 1) );
      
      uword major_ver = 0;
      uword minor_ver = 0;
      
      major_ver_ss >> major_ver;
      minor_ver_ss >> minor_ver;
      
      const bool device_ver_ok = ( (major_ver >= 2) || ( (major_ver >= 1) && (minor_ver >= 2) ) );
      
      // prefer GPUs over CPUs and other devices
      // prefer devices with 64 bit float support
      // prefer devices with OpenCL 1.2 or greater
      if(device_type_val == CL_DEVICE_TYPE_GPU)  { local_device_pri.at(local_device_count) +=  2; }
      if(device_fp64_val != 0                 )  { local_device_pri.at(local_device_count) +=  1; }
      if(device_ver_ok   == false             )  { local_device_pri.at(local_device_count)  = -1; }
      }
    }
  
  
  // find the device with highest priority
  
  bool found_device = false;
  
  
  if(manual_selection)
    {
    // TODO: need to check for bounds
    
    platform_id = platform_ids.at(wanted_platform_id);
    
    std::vector<cl_device_id>& local_device_ids = device_ids.at(wanted_platform_id);
    device_id = local_device_ids.at(wanted_device_id);
    
    found_device = true;
    }
  else
    {
    int best_val = -1;
    
    for(size_t platform_count = 0; platform_count < n_platforms; ++platform_count)
      {
      std::vector<cl_device_id>& local_device_ids = device_ids.at(platform_count);
      std::vector<int>&          local_device_pri = device_pri.at(platform_count);
      
      size_t local_n_devices = local_device_ids.size();
      
      for(size_t local_device_count = 0; local_device_count < local_n_devices; ++local_device_count)
        {
        const int tmp_val = local_device_pri.at(local_device_count);
        
        // cout << "platform_count: " << platform_count << "  local_device_count: " << local_device_count << "  priority: " << tmp_val << "   best_val: " << best_val << endl;
        
        if(best_val < tmp_val)
          {
          best_val = tmp_val;
          
          found_device = true;
          
          platform_id = platform_ids.at(platform_count);
          device_id   = local_device_ids.at(local_device_count);
          }
        }
      }
    }
  
  
  if(found_device == false)
    {
    out_errmsg = "couldn't find a suitable device";
    return false;
    }
  
  
  
  cl_context_properties properties[3] = { CL_CONTEXT_PLATFORM, cl_context_properties(platform_id), 0 };
  
  status = 0;
  context = clCreateContext(properties, 1, &device_id, NULL, NULL, &status);
  
  if((status != CL_SUCCESS) || (context == NULL))
    {
    out_errmsg = "couldn't create context";
    return false;
    }
  
  status = 0;
  cq = clCreateCommandQueue(context, device_id, 0, &status);
  
  // NOTE: http://stackoverflow.com/questions/28500496/opencl-function-found-deprecated-by-visual-studio
  // NOTE: clCreateCommandQueue is deprecated as of OpenCL 2.0, but it will be supported for the "foreseeable future"
  // NOTE: clCreateCommandQueue is replaced with clCreateCommandQueueWithProperties in OpenCL 2.0
  
  if((status != CL_SUCCESS) || (cq == NULL))
    {
    out_errmsg = "couldn't create command queue";
    return false;
    }
  
  return true;
  }


inline
bool
coot_runtime_t::interrogate_device(const bool print_details)
  {
  coot_extra_debug_sigprint();
  
  // TODO: optionally disable/enable printing the device details ?  (eg. when COOT_EXTRA_DEBUG is enabled)
  
  cl_device_type      device_type = 0;
  cl_uint             device_bits = 0;
  cl_device_fp_config device_fp64 = 0;
  
  cl_char vendor_name[1024]; // TODO: use dynamic memory allocation (podarray or std::vector)
  cl_char device_name[1024];
  cl_char device_vstr[1024];  // vstr = version string
  
  vendor_name[0] = cl_char(0);
  device_name[0] = cl_char(0);
  
  cl_uint tmp_align   = 0;
  cl_uint tmp_n_units = 0;

  clGetDeviceInfo(device_id, CL_DEVICE_VENDOR,              sizeof(vendor_name),         &vendor_name, NULL);
  clGetDeviceInfo(device_id, CL_DEVICE_NAME,                sizeof(device_name),         &device_name, NULL);
  clGetDeviceInfo(device_id, CL_DEVICE_VERSION,             sizeof(device_vstr),         &device_vstr, NULL);
  clGetDeviceInfo(device_id, CL_DEVICE_TYPE,                sizeof(cl_device_type),      &device_type, NULL);
  clGetDeviceInfo(device_id, CL_DEVICE_ADDRESS_BITS,        sizeof(cl_uint),             &device_bits, NULL);
  clGetDeviceInfo(device_id, CL_DEVICE_DOUBLE_FP_CONFIG,    sizeof(cl_device_fp_config), &device_fp64, NULL);
  clGetDeviceInfo(device_id, CL_DEVICE_MEM_BASE_ADDR_ALIGN, sizeof(cl_uint),             &tmp_align,   NULL);
  clGetDeviceInfo(device_id, CL_DEVICE_MAX_COMPUTE_UNITS,   sizeof(cl_uint),             &tmp_n_units, NULL);
  
  (*this).device_64bit_float = (device_fp64 != 0);
  (*this).n_units            = uword(tmp_n_units);  // TODO: add sanity check to ensure n_units >= 1
  
  
  
  
  if(print_details)
    {
    get_stream_err1() << "coot_runtime::interrogate_device():" << std::endl;
    
    if(device_type != CL_DEVICE_TYPE_GPU)
      {
      get_stream_err1() << "WARNING: device is not a GPU" << std::endl;
      }
    
    get_stream_err1() << "        vendor: " << vendor_name << std::endl;
    get_stream_err1() << "        device: " << device_name << std::endl;
    get_stream_err1() << "version string: " << device_vstr << std::endl;
    get_stream_err1() << "          bits: " << device_bits << std::endl;
    get_stream_err1() << "          fp64: " << ( (device_fp64 != 0) ? "yes" : "no" ) << std::endl;
    get_stream_err1() << "         align: " << tmp_align   << std::endl;
    get_stream_err1() << "       n_units: " << tmp_n_units << std::endl;
    }
  
  // contrary to the official OpenCL specification (OpenCL 1.2, sec 4.2 and sec 6.1.1).
  // certain OpenCL implementations use internal size_t which doesn't corresond to CL_DEVICE_ADDRESS_BITS
  // example: Clover from Mesa 13.0.4, running as AMD OLAND (DRM 2.48.0 / 4.9.14-200.fc25.x86_64, LLVM 3.9.1)
  
  const char* tmp_program_src = \
    "__kernel void coot_interrogate(__global uint* out) \n"
    "  {                                                \n"
    "  const size_t i = get_global_id(0);               \n"
    "  if(i == 0)                                       \n"
    "    {                                              \n"
    "    out[0] = (uint)sizeof(size_t);                 \n"
    "    out[1] = (uint)sizeof(void*);                  \n"
    "    out[2] = (uint)(__OPENCL_VERSION__);           \n"
    "    }                                              \n"
    "  }                                                \n";
  
  bool found_width = false;
  
  cl_program tmp_program     = NULL;
  cl_kernel  tmp_kernel      = NULL;
  cl_mem     tmp_dev_mem     = NULL;
  cl_uint    tmp_host_mem[4] = { 0, 0, 0, 0 };
  
  cl_int status = 0;
  
  tmp_program = clCreateProgramWithSource(context, 1, (const char **)&(tmp_program_src), NULL, &status);
  
  if(status == CL_SUCCESS)
    {
    status = clBuildProgram(tmp_program, 0, NULL, NULL, NULL, NULL);
    
    if(status == CL_SUCCESS)
      {
      status = 0;
      tmp_kernel = clCreateKernel(tmp_program, "coot_interrogate", &status);
      
      if(status == CL_SUCCESS)
        {
        status = 0;
        tmp_dev_mem = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_uint)*4, NULL, &status);
        
        status = 0;
        clSetKernelArg(tmp_kernel, 0, sizeof(cl_mem),  &tmp_dev_mem);
        status = clEnqueueTask(cq, tmp_kernel, 0, NULL, NULL);  // TODO: replace with clEnqueueNDRangeKernel to avoid deprecation warnings
        
        if(status == CL_SUCCESS)
          {
          clFinish(cq);
          
          status = clEnqueueReadBuffer(cq, tmp_dev_mem, CL_TRUE, 0, sizeof(cl_uint)*4, tmp_host_mem, 0, NULL, NULL);
          
          if(status == CL_SUCCESS)
            {
            clFinish(cq);
            
            if(print_details)
              {
              get_stream_err1() << "sizeof(size_t): " << tmp_host_mem[0] << std::endl;
              get_stream_err1() << " sizeof(void*): " << tmp_host_mem[1] << std::endl;
              get_stream_err1() << "OPENCL_VERSION: " << tmp_host_mem[2] << std::endl;
              }
            
            const cl_uint device_sizet_width = tmp_host_mem[0];
            const cl_uint device_opencl_ver  = tmp_host_mem[2];
            
            if( (device_sizet_width == 4) || (device_sizet_width == 8) )
              {
              found_width = true;
              if(device_sizet_width == 8)  { (*this).device_64bit_sizet = true; }
              }
            
            // TODO: for paranoia, check consistency with the version extracted from the version string (CL_DEVICE_VERSION)
            
            if(device_opencl_ver < 120)
              {
              coot_warn("device has OpenCL version lower than 1.2");
              }
            }
          }
        }
      }
    }
  
  if(found_width == false)  { coot_warn("coot_runtime: size_t has unsupported width; using 32 bit word"); }
  
  if(tmp_dev_mem != NULL)  { clReleaseMemObject(tmp_dev_mem); }
  if(tmp_kernel  != NULL)  { clReleaseKernel(tmp_kernel);        }
  if(tmp_program != NULL)  { clReleaseProgram(tmp_program);      }
  
  return found_width;
  }



template<typename eT>
inline
bool
coot_runtime_t::init_kernels(std::vector<cl_kernel>& kernels, const std::string& source, const std::vector<std::string>& names)
  {
  coot_extra_debug_sigprint();
  
  cl_int status;
  
  // TODO: get info using clquery ?
  
  coot_runtime_t::program_wrapper prog_holder;  // program_wrapper will automatically call clReleaseProgram() when it goes out of scope
  

  // cl_program clCreateProgramWithSource(cl_context context, cl_uint count, const char **strings, const size_t *lengths, cl_int *errcode_ret);
  // strings = An array of N pointers (where N = count) to optionally null-terminated character strings that make up the source code. 
  // lengths = An array with the number of chars in each string (the string length). If an element in lengths is zero, its accompanying string is null-terminated.
  //           If lengths is NULL, all strings in the strings argument are considered null-terminated.
  //           Any length value passed in that is greater than zero excludes the null terminator in its count. 
  
  
  status = 0;
  
  const char* source_c_str = source.c_str();
  
  prog_holder.prog = clCreateProgramWithSource(context, 1, &source_c_str, NULL, &status);
  
  if((status != CL_SUCCESS) || (prog_holder.prog == NULL))
    {
    cout << "status: " << coot_cl_error::as_string(status) << endl;
    
    std::cout << "coot_runtime::init_kernels(): couldn't create program" << std::endl;
    return false;
    }
  
  // TODO: the defines need to be progressively built
  // use std::string, concatenation and .c_str()
  
  std::string build_options;
  std::string prefix;
  
  if(is_same_type<eT, u32>::yes)
    {
    prefix = "u32_";
    
    build_options += "-D PREFIX=";
    build_options += prefix;
    build_options += " ";
    build_options += "-D eT=uint";
    build_options += " ";
    build_options += "-D promoted_eT=float";
    build_options += " ";
    }
  else
  if(is_same_type<eT, s32>::yes)
    {
    prefix = "s32_";
    
    build_options += "-D PREFIX=";
    build_options += prefix;
    build_options += " ";
    build_options += "-D eT=int";
    build_options += " ";
    build_options += "-D promoted_eT=float";
    build_options += " ";
    }
  else
  if(is_same_type<eT, u64>::yes)
    {
    prefix = "u64_";
    
    build_options += "-D PREFIX=";
    build_options += prefix;
    build_options += " ";
    build_options += "-D eT=ulong";
    build_options += " ";
    build_options += "-D promoted_eT=float";
    build_options += " ";
    }
  else
  if(is_same_type<eT, s64>::yes)
    {
    prefix = "s64_";
    
    build_options += "-D PREFIX=";
    build_options += prefix;
    build_options += " ";
    build_options += "-D eT=long";
    build_options += " ";
    build_options += "-D promoted_eT=float";
    build_options += " ";
    }
  else
  if(is_same_type<eT, float>::yes)
    {
    prefix = "f_";
    
    build_options += "-D PREFIX=";
    build_options += prefix;
    build_options += " ";
    build_options += "-D eT=float";
    build_options += " ";
    build_options += "-D promoted_eT=float";
    build_options += " ";
    }
  else
  if(is_same_type<eT, double>::yes)
    {
    prefix = "d_";
    
    build_options += "-D PREFIX=";
    build_options += prefix;
    build_options += " ";
    build_options += "-D eT=double";
    build_options += " ";
    build_options += "-D promoted_eT=double";
    build_options += " ";
    }
  
  
  build_options += ((sizeof(uword) >= 8) && device_64bit_sizet) ? "-D UWORD=ulong" : "-D UWORD=uint";
    
  status = clBuildProgram(prog_holder.prog, 0, NULL, build_options.c_str(), NULL, NULL);
  
  if(status != CL_SUCCESS)
    {
    cout << "status: " << coot_cl_error::as_string(status) << endl;
    
    size_t len = 0;
    char buffer[10240];  // TODO: use std::vector<char> or podarray

    clGetProgramBuildInfo(prog_holder.prog, device_id, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
    std::cout << "coot_runtime::init_kernels(): couldn't build program;"              << std::endl;
    std::cout << "coot_runtime::init_kernels(): output from clGetProgramBuildInfo():" << std::endl;
    std::cout << buffer << std::endl;
    
    return false;
    }
  
  
  const uword n_kernels = names.size();
  
  kernels.resize(n_kernels);
  
  for(uword i=0; i < n_kernels; ++i)
    {
    status = 0;
    
    const std::string kernel_name = prefix + names.at(i);
    
    kernels.at(i) = clCreateKernel(prog_holder.prog, kernel_name.c_str(), &status);
    
    if((status != CL_SUCCESS) || (kernels[i] == NULL))
      {
      std::cout << coot_cl_error::as_string(status) << endl;
      std::cout << "kernel_name: " << kernel_name << endl;
      return false;
      }
    }
  
  return true;
  }



inline
uword
coot_runtime_t::get_n_units() const
  {
  return (valid) ? n_units : uword(0);
  }



inline
bool
coot_runtime_t::is_valid() const
  {
  return valid;
  }



inline
bool
coot_runtime_t::has_64bit_sizet() const
  {
  return device_64bit_sizet;
  }



inline
bool
coot_runtime_t::has_64bit_float() const
  {
  return device_64bit_float;
  }



template<typename eT>
inline
cl_mem
coot_runtime_t::acquire_memory(const uword n_elem)
  {
  coot_extra_debug_sigprint();
  
  coot_check_runtime_error( (valid == false), "coot_runtime::acquire_memory(): runtime not valid" );
  
  if(n_elem == 0)  { return NULL; }
  
  coot_debug_check
   (
   ( size_t(n_elem) > (std::numeric_limits<size_t>::max() / sizeof(eT)) ),
   "coot_runtime::acquire_memory(): requested size is too large"
   );
  
  cl_int status = 0;
  cl_mem dev_mem = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(eT)*(std::max)(uword(1), n_elem), NULL, &status);
  
  coot_check_bad_alloc( ((status != CL_SUCCESS) || (dev_mem == NULL)), "coot_runtime::acquire_memory(): not enough memory on device" );
  
  return dev_mem;
  }



inline
void
coot_runtime_t::release_memory(cl_mem dev_mem)
  {
  coot_extra_debug_sigprint();
  
  coot_debug_check( (valid == false), "coot_runtime not valid" );
  
  if(dev_mem)  { clReleaseMemObject(dev_mem); }
  }



inline
cl_context
coot_runtime_t::get_context()
  {
  coot_extra_debug_sigprint();
  
  coot_debug_check( (valid == false), "coot_runtime not valid" );
  
  return context;
  }



inline
cl_command_queue
coot_runtime_t::get_cq()
  {
  coot_extra_debug_sigprint();
  
  coot_debug_check( (valid == false), "coot_runtime not valid" );
  
  return cq;
  }



template<typename eT>
inline
cl_kernel
coot_runtime_t::get_kernel(const kernel_id::enum_id num)
  {
  coot_extra_debug_sigprint();
  
  coot_debug_check( (valid == false), "coot_runtime not valid" );
  
       if(is_same_type<eT,u32   >::yes)  { return u32_kernels.at(num); }
  else if(is_same_type<eT,s32   >::yes)  { return s32_kernels.at(num); }
  else if(is_same_type<eT,u64   >::yes)  { return u64_kernels.at(num); }
  else if(is_same_type<eT,s64   >::yes)  { return s64_kernels.at(num); }
  else if(is_same_type<eT,float >::yes)  { return   f_kernels.at(num); }
  else if(is_same_type<eT,double>::yes)  { return   d_kernels.at(num); }
  else { coot_debug_check(true, "unsupported element type" ); }
  }



// TODO: should this be in the run-time library, to ensure only one copy of the runtime is active?
// TODO: don't instantiate coot_runtime_t if COOT_USE_WRAPPER is enabled?
// TODO: put instantiation in an anonymous namespace to avoid linking conflicts?
coot_runtime_t coot_runtime;



//
// program_wrapper

inline
coot_runtime_t::program_wrapper::program_wrapper()
  {
  coot_extra_debug_sigprint();
  
  prog = NULL;
  }



inline
coot_runtime_t::program_wrapper::~program_wrapper()
  {
  coot_extra_debug_sigprint();
  
  if(prog != NULL)
    {
    clReleaseProgram(prog);
    }
  }



//
// cq_guard

inline
coot_runtime_t::cq_guard::cq_guard()
  {
  coot_extra_debug_sigprint();
  
  coot_runtime.lock();
  
  if(coot_runtime.is_valid())
    {
    coot_extra_debug_print("calling clFinish()");
    clFinish(coot_runtime.get_cq());  // force synchronisation
    
    //coot_extra_debug_print("calling clFlush()");
    //clFlush(coot_runtime.get_cq());  // submit all enqueued commands
    }
  }



inline
coot_runtime_t::cq_guard::~cq_guard()
  {
  coot_extra_debug_sigprint();
  
  if(coot_runtime.is_valid())
    {
    coot_extra_debug_print("calling clFlush()");
    clFlush(coot_runtime.get_cq());  // submit all enqueued commands
    }
  
  coot_runtime.unlock();
  }




//
// adapt_uword

inline
coot_runtime_t::adapt_uword::adapt_uword(const uword val)
  {
  if((sizeof(uword) >= 8) && coot_runtime.has_64bit_sizet())
    {
    size  = sizeof(u64);
    addr  = (void*)(&val64);
    val64 = u64(val);
    }
  else
    {
    size   = sizeof(u32);
    addr   = (void*)(&val32);
    val32  = u32(val);
    
    coot_check_runtime_error( ((sizeof(uword) >= 8) && (val > 0xffffffffU)), "adapt_uword: given value doesn't fit into unsigned 32 bit integer" );
    }
  }