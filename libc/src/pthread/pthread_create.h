//===-- Implementation header for pthread_create function -------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIBC_SRC_PTHREAD_PTHREAD_CREATE_H
#define LLVM_LIBC_SRC_PTHREAD_PTHREAD_CREATE_H

#include "src/__support/macros/config.h"
#include <pthread.h>

namespace LIBC_NAMESPACE_DECL {

int pthread_create(pthread_t *__restrict thread,
                   const pthread_attr_t *__restrict attr,
                   __pthread_start_t func, void *arg);

} // namespace LIBC_NAMESPACE_DECL

#endif // LLVM_LIBC_SRC_PTHREAD_PTHREAD_CREATE_H
