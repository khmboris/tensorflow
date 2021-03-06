/* Copyright 2020 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

// TODO(skye): this is largely a copy of tpu_api_dlsym_initializer.cc. Figure
// out how to deduplicate these files a little.

#include <dlfcn.h>

#include "tensorflow/core/platform/errors.h"
#include "tensorflow/core/platform/status.h"
#include "tensorflow/core/tpu/tpu_api_dlsym_set_fn.h"
#if !defined(PLATFORM_GOOGLE)
#include "tensorflow/core/tpu/tpu_executor_api.h"
#include "tensorflow/stream_executor/tpu/tpu_executor_c_api.h"
#include "tensorflow/stream_executor/tpu/tpu_platform.h"
#endif

namespace tensorflow {
namespace tpu {

#if defined(PLATFORM_GOOGLE)
Status InitializeTpuLibrary(void* library_handle) {
  return errors::Unimplemented("You must statically link in a TPU library.");
}
#else  // PLATFORM_GOOGLE
#include "tensorflow/core/tpu/tpu_executor_init_fns.inc"

Status InitializeTpuLibrary(void* library_handle) {
  Status s = SetExecutorStructFn(library_handle);

  // TPU platform registration must only be performed after the library is
  // loaded. We do not want to register a TPU platform in XLA without the
  // supporting library providing the necessary APIs.
  if (s.ok()) {
    void (*initialize_fn)();
    initialize_fn = reinterpret_cast<decltype(initialize_fn)>(dlsym(
        library_handle, "TfTpu_Initialize", /*argc=*/0, /*argv=*/nullptr));
    (*initialize_fn)();

    RegisterTpuPlatform();
  }

  return s;
}

bool FindAndLoadTpuLibrary() {
  void* library = dlopen("libtpu.so", RTLD_NOW);
  if (library) {
    InitializeTpuLibrary(library);
  }
  return true;
}

static bool tpu_library_finder = FindAndLoadTpuLibrary();
#endif  // PLATFORM_GOOGLE

}  // namespace tpu
}  // namespace tensorflow
