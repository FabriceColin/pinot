/*
 *  Copyright 2009-2011 Fabrice Colin
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _MEMORY_H
#define _MEMORY_H

#include "config.h"
#include <execinfo.h>
#include <string>
#include <iostream>
#ifdef HAVE_UMEM_H
#include <umem.h>

struct umem_allocator_tag
{
};

/// Based on an example gven at http://www.ddj.com/cpp/184403759
template <typename T>
class umem_allocator
{
	public:
		typedef T                 value_type;
		typedef value_type*       pointer;
		typedef const value_type* const_pointer;
		typedef value_type&       reference;
		typedef const value_type& const_reference;
		typedef typename std::size_t size_type;
		typedef typename std::ptrdiff_t difference_type;

		template <typename U>
		struct rebind
		{
			typedef umem_allocator<U> other;
		};

		umem_allocator()
		{
		}
		template <typename U>
		umem_allocator(const umem_allocator<U> &)
		{
		}
		~umem_allocator()
		{
		}

		static pointer address(reference x)
		{
			return &x;
		}

		static const_pointer address(const_reference x)
		{
			return x;
		}

		static size_type max_size()
		{
			return (std::numeric_limits<size_type>::max)();
		}

		static void construct(pointer p, const value_type &t)
		{
			new(p) T(t);
		}

		static void destroy(pointer p)
		{
			p->~T();
			// Avoid unused variable warning
			(void)p;
		}

		bool operator==(const umem_allocator &) const
		{
			return true;
		}

		bool operator!=(const umem_allocator &) const
		{
			return false;
		}

		static pointer allocate(size_type n)
		{
#ifdef DEBUG
			std::cout << "umem_allocator::allocate: allocating " << n << std::endl;
#endif
			void *ret = umem_alloc(n * sizeof(T), UMEM_DEFAULT);
			if (ret == NULL)
			{
				throw std::bad_alloc();
			}
			return static_cast<pointer>(ret);
		}

		static pointer allocate(const size_type n, const void *const)
		{
			return allocate(n);
		}

		static void deallocate(pointer p, size_type n)
		{
			if ((p == NULL) ||
				(n == 0))
			{
				return;
			}

#ifdef DEBUG
			std::cout << "umem_allocator::deallocate: freeing " << n << std::endl;
#endif
			umem_free(static_cast<void*>(p), n * sizeof(T));
		}

};

/// Specialization for void.
template<>
class umem_allocator<void>
{
	typedef void        value_type;
	typedef void*       pointer;
	typedef const void* const_pointer;

	template <class U>
	struct rebind
	{
		typedef umem_allocator<U> other;
	};

};
#else
#ifdef HAVE_EXT_MALLOC_ALLOCATOR_H
#include <ext/malloc_allocator.h>
#endif
#endif
#ifdef HAVE_BOOST_POOL_POOLFWD_HPP
// This header should be available if poolfwd.hpp is
#include <boost/pool/pool_alloc.hpp>
#endif

#include "Visibility.h"

#ifdef HAVE_BOOST_POOL_POOLFWD_HPP
#ifdef HAVE_UMEM_H
// Memory pool with umem
typedef boost::pool_allocator<char,
	umem_allocator> filter_allocator;
#else
// Memory pool with malloc (glibc's or any other implementation)
typedef boost::pool_allocator<char,
	boost::default_user_allocator_malloc_free,
	boost::details::pool::default_mutex, 131072> filter_allocator;
#endif
typedef std::basic_string<char, std::char_traits<char>, filter_allocator > dstring;
#else
#ifdef HAVE_UMEM_H
// No memory pool, plain umem
typedef umem_allocator<char> filter_allocator;
typedef std::basic_string<char, std::char_traits<char>, filter_allocator > dstring;
#else
#ifdef HAVE_EXT_MALLOC_ALLOCATOR_H
// No memory pool, plain malloc (glibc's or any other implementation)
typedef __gnu_cxx::malloc_allocator<char> filter_allocator;
typedef std::basic_string<char, std::char_traits<char>, filter_allocator > dstring;
#else
// No memory pool, default STL allocator
typedef std::allocator<char> filter_allocator;
typedef std::string dstring;
#endif
#endif
#endif

/// Memory usage related utilities.
class PINOT_EXPORT Memory
{
	public:
		/// Allocates a buffer.
		static char *allocateBuffer(off_t length);

		/// Frees a buffer.
		static void freeBuffer(char *pBuffer, off_t length);

		/// Returns the number of bytes in use.
		static int getUsage(void);

		/// Hints to the OS it can reclaim unused memory.
		static void reclaim(void);

	protected:
		Memory();

};

#endif // _MEMORY_H
