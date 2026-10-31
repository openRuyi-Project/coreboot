/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef ARCH_CACHE_H
#define ARCH_CACHE_H

/* TODO: implement these API stubs once caching is available on RISC-V */
static inline void dcache_clean_all(void) {}
static inline void dcache_invalidate_all(void) {}
static inline void dcache_clean_invalidate_all(void) {}

/* SoC hook called by every hart after moving to ramstage code and stacks. */
void exit_car(void);

#endif /* ARCH_CACHE_H */
