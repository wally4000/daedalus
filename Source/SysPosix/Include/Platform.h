/*
Copyright (C) 2008 StrmnNrmn

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/



#ifndef SYSPOSIX_INCLUDE_PLATFORM_H_
#define SYSPOSIX_INCLUDE_PLATFORM_H_

#ifdef __GNUC__
#define DAEDALUS_EXPECT_LIKELY(c) __builtin_expect((c),1)
#define DAEDALUS_EXPECT_UNLIKELY(c) __builtin_expect((c),0)

#define DAEDALUS_ATTRIBUTE_NOINLINE __attribute__((noinline))
#endif

#define DAEDALUS_HALT			__builtin_trap()
//#define DAEDALUS_HALT			__builtin_debugger()

template <typename T>
T make_uncached_ptr(T ptr) {
    return 0;
}

#endif // SYSPOSIX_INCLUDE_PLATFORM_H_
