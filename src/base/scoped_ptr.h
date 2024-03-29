#pragma once

// Taken from boost 1.46

//  (C) Copyright Greg Colvin and Beman Dawes 1998, 1999.
//  Copyright (c) 2001, 2002 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//  http://www.boost.org/libs/smart_ptr/scoped_ptr.htm
//

#include <cassert>

namespace cheaproute
{
template<class T> inline void checked_delete(T * x)
{
  // intentionally complex - simplification causes regressions
  typedef char type_must_be_complete[ sizeof(T)? 1: -1 ];
  (void) sizeof(type_must_be_complete);
  delete x;
}
  
//  scoped_ptr mimics a built-in pointer except that it guarantees deletion
//  of the object pointed to, either on destruction of the scoped_ptr or via
//  an explicit reset(). scoped_ptr is a simple solution for simple needs;
//  use shared_ptr or std::auto_ptr if your needs are more complex.

template<class T> class scoped_ptr // noncopyable
{
private:

  T * px;

  scoped_ptr(scoped_ptr const &);
  scoped_ptr & operator=(scoped_ptr const &);

  typedef scoped_ptr<T> this_type;

  void operator==( scoped_ptr const& ) const;
  void operator!=( scoped_ptr const& ) const;

public:

  typedef T element_type;

  explicit scoped_ptr( T * p = 0 ): px( p ) // never throws
  {
  }

  ~scoped_ptr() // never throws
  {
    checked_delete( px );
  }

  void reset(T * p = 0) // never throws
  {
    assert( p == 0 || p != px ); // catch self-reset errors
    this_type(p).swap(*this);
  }

  T & operator*() const // never throws
  {
    v( px != 0 );
    return *px;
  }

  T * operator->() const // never throws
  {
    assert( px != 0 );
    return px;
  }

  T * get() const // never throws
  {
    return px;
  }

  typedef T * this_type::*unspecified_bool_type;

  operator unspecified_bool_type() const // never throws
  {
    return px == 0? 0: &this_type::px;
  }
  
  void swap(scoped_ptr & b) // never throws
  {
    T * tmp = b.px;
    b.px = px;
    px = tmp;
  }
};

template<class T> inline void swap(scoped_ptr<T> & a, scoped_ptr<T> & b) // never throws
{
  a.swap(b);
}

// get_pointer(p) is a generic way to say p.get()

template<class T> inline T * get_pointer(scoped_ptr<T> const & p)
{
  return p.get();
}

} // namespace boost
