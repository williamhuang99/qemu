/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation, or (at your option) any
 * later version. See the COPYING file in the top-level directory.
 */

#ifndef CSKY_TARGET_ELF_H
#define CSKY_TARGET_ELF_H
static inline const char *cpu_get_model(uint32_t eflags)
{
#ifdef TARGET_CSKYV1
    return "ck610ef";
#else
    return "ck810f";
#endif
}
#endif
