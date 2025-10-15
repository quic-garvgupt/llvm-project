// Verify that BareMetal driver forwards user -I as --includepaths to ld.lld,
// irrespective of LTO enablement, preserves ordering for multiple -I,
// and emits nothing when no -I is provided.
//
// RUN: rm -rf %t && mkdir -p %t && mkdir -p %t/inc %t/dir1 %t/dir2
//
// With LTO enabled, -I is forwarded.
// RUN: %clang -### --target=riscv32-unknown-elf -nostdlib -fuse-ld=lld -flto %s -I %t/inc 2>&1 | FileCheck %s --check-prefix=WITHLTO
//
// Without LTO, -I is still forwarded.
// RUN: %clang -### --target=riscv32-unknown-elf -nostdlib -fuse-ld=lld %s -I %t/inc 2>&1 | FileCheck %s --check-prefix=NOLTO
//
// With no -I, nothing is forwarded.
// RUN: %clang -### --target=riscv32-unknown-elf -nostdlib -fuse-ld=lld %s 2>&1 | FileCheck %s --check-prefix=NOI
//
// Multiple -I are forwarded in order.
// RUN: %clang -### --target=riscv32-unknown-elf -nostdlib -fuse-ld=lld %s -I %t/dir1 -I %t/dir2 2>&1 | FileCheck %s --check-prefix=MULTI
//
// WITHLTO: "--includepaths=
// WITHLTO-SAME: {{.*}}inc
//
// NOLTO: "--includepaths=
// NOLTO-SAME: {{.*}}inc
//
// NOI-NOT: --includepaths
//
// MULTI: "--includepaths={{.*}}dir1"
// MULTI-SAME: "--includepaths={{.*}}dir2"

int main(void) { return 0; }
