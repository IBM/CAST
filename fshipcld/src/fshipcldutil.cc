/*******************************************************************************
 |    fshipcldutil.cc
 |
 |  © Copyright IBM Corporation 2016,2016. All Rights Reserved
 |
 |    This program is licensed under the terms of the Eclipse Public License
 |    v1.0 as published by the Eclipse Foundation and available at
 |    http://www.eclipse.org/legal/epl-v10.html
 |
 |    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
 |    restricted by GSA ADP Schedule Contract with IBM Corp.
 *******************************************************************************/
//!
//! \file   fshipcldutil.cc
//! \author Mike Aho <meaho@us.ibm.com>
//! \date   Wed Nov  9 13:45:53 2016
//! 
//! \brief  include utilities for fshipcld.cc
//! 
//! 
//!


#include <string>
//#include <boost/lexical_cast.hpp>
#include "fshipcld.h"
#include "logging.h"
#include <boost/program_options.hpp>

//! \brief Dump init flags between fuse module and fshipcld
//! \param flags See fuse_init_in and fuse_init_out flags in fuse.h
//!
//! \return Any bits extra are returned as the residue.  Nonzero value
//!         means fuse module has added bits.
//!
uint32_t dump_init_flags(uint32_t flags) {
  if (flags & FUSE_ASYNC_READ) {
    LOG(fshipcld, info) << "FUSE_ASYNC_READ: asynchronous read requests="
                        << (flags & FUSE_ASYNC_READ);
  } else {
    LOG(fshipcld, debug) << "FUSE_ASYNC_READ: asynchronous read requests="
                         << (flags & FUSE_ASYNC_READ);
  }
  if (flags & FUSE_POSIX_LOCKS) {
    LOG(fshipcld, info)
        << "FUSE_POSIX_LOCKS: remote locking for POSIX file locks="
        << (flags & FUSE_POSIX_LOCKS);
  } else {
    LOG(fshipcld, debug)
        << "FUSE_POSIX_LOCKS: remote locking for POSIX file locks="
        << (flags & FUSE_POSIX_LOCKS);
  }
  if (flags & FUSE_FILE_OPS) {
    LOG(fshipcld, info) << "FUSE_FILE_OPS: kernel sends file handle for fstat, "
                           "etc... (not yet supported)="
                        << (flags & FUSE_FILE_OPS);
  } else {
    LOG(fshipcld, debug) << "FUSE_FILE_OPS: kernel sends file handle for "
                            "fstat, etc... (not yet supported)="
                         << (flags & FUSE_FILE_OPS);
  }
  if (flags & FUSE_ATOMIC_O_TRUNC) {
    LOG(fshipcld, info) << "FUSE_ATOMIC_O_TRUNC: handles the O_TRUNC open flag "
                           "in the filesystem="
                        << (flags & FUSE_ATOMIC_O_TRUNC);
  } else {
    LOG(fshipcld, debug) << "FUSE_ATOMIC_O_TRUNC: handles the O_TRUNC open "
                            "flag in the filesystem="
                         << (flags & FUSE_ATOMIC_O_TRUNC);
  }
  if (flags & FUSE_EXPORT_SUPPORT) {
    LOG(fshipcld, info)
        << "FUSE_EXPORT_SUPPORT: filesystem handles lookups of . and ..="
        << (flags & FUSE_EXPORT_SUPPORT);
  } else {
    LOG(fshipcld, debug)
        << "FUSE_EXPORT_SUPPORT: filesystem handles lookups of . and ..="
        << (flags & FUSE_EXPORT_SUPPORT);
  }
  if (flags & FUSE_BIG_WRITES) {
    LOG(fshipcld, info)
        << "FUSE_BIG_WRITES: filesystem can handle write size larger than 4kB="
        << (flags & FUSE_BIG_WRITES);
  } else {
    LOG(fshipcld, debug)
        << "FUSE_BIG_WRITES: filesystem can handle write size larger than 4kB="
        << (flags & FUSE_BIG_WRITES);
  }
  if (flags & FUSE_DONT_MASK) {
    LOG(fshipcld, info)
        << "FUSE_DONT_MASK: dont apply umask to file mode on create operations="
        << (flags & FUSE_DONT_MASK);
  } else {
    LOG(fshipcld, debug)
        << "FUSE_DONT_MASK: dont apply umask to file mode on create operations="
        << (flags & FUSE_DONT_MASK);
  }
  if (flags & FUSE_SPLICE_WRITE) {
    LOG(fshipcld, info)
        << "FUSE_SPLICE_WRITE: kernel supports splice write on the device="
        << (flags & FUSE_SPLICE_WRITE);
  } else {
    LOG(fshipcld, debug)
        << "FUSE_SPLICE_WRITE: kernel supports splice write on the device="
        << (flags & FUSE_SPLICE_WRITE);
  }
  if (flags & FUSE_SPLICE_MOVE) {
    LOG(fshipcld, info)
        << "FUSE_SPLICE_MOVE: kernel supports splice move on the device="
        << (flags & FUSE_SPLICE_MOVE);
  } else {
    LOG(fshipcld, debug)
        << "FUSE_SPLICE_MOVE: kernel supports splice move on the device="
        << (flags & FUSE_SPLICE_MOVE);
  }
  if (flags & FUSE_SPLICE_READ) {
    LOG(fshipcld, info)
        << "FUSE_SPLICE_READ: kernel supports splice read on the device="
        << (flags & FUSE_SPLICE_READ);
  } else {
    LOG(fshipcld, debug)
        << "FUSE_SPLICE_READ: kernel supports splice read on the device="
        << (flags & FUSE_SPLICE_READ);
  }
  if (flags & FUSE_FLOCK_LOCKS) {
    LOG(fshipcld, info)
        << "FUSE_FLOCK_LOCKS: remote locking for BSD style file locks="
        << (flags & FUSE_FLOCK_LOCKS);
  } else {
    LOG(fshipcld, debug)
        << "FUSE_FLOCK_LOCKS: remote locking for BSD style file locks="
        << (flags & FUSE_FLOCK_LOCKS);
  }
  if (flags & FUSE_HAS_IOCTL_DIR) {
    LOG(fshipcld, info)
        << "FUSE_HAS_IOCTL_DIR: kernel supports ioctl on directories="
        << (flags & FUSE_HAS_IOCTL_DIR);
  } else {
    LOG(fshipcld, debug)
        << "FUSE_HAS_IOCTL_DIR: kernel supports ioctl on directories="
        << (flags & FUSE_HAS_IOCTL_DIR);
  }
  if (flags & FUSE_AUTO_INVAL_DATA) {
    LOG(fshipcld, info)
        << "FUSE_AUTO_INVAL_DATA: automatically invalidate cached pages="
        << (flags & FUSE_AUTO_INVAL_DATA);
  } else {
    LOG(fshipcld, debug)
        << "FUSE_AUTO_INVAL_DATA: automatically invalidate cached pages="
        << (flags & FUSE_AUTO_INVAL_DATA);
  }
  if (flags & FUSE_DO_READDIRPLUS) {
    LOG(fshipcld, info)
        << "FUSE_DO_READDIRPLUS: do READDIRPLUS (READDIR+LOOKUP in one)="
        << (flags & FUSE_DO_READDIRPLUS);
  } else {
    LOG(fshipcld, debug)
        << "FUSE_DO_READDIRPLUS: do READDIRPLUS (READDIR+LOOKUP in one)="
        << (flags & FUSE_DO_READDIRPLUS);
  }
  if (flags & FUSE_READDIRPLUS_AUTO) {
    LOG(fshipcld, info) << "FUSE_READDIRPLUS_AUTO: adaptive readdirplus="
                        << (flags & FUSE_READDIRPLUS_AUTO);
  } else {
    LOG(fshipcld, debug) << "FUSE_READDIRPLUS_AUTO: adaptive readdirplus="
                         << (flags & FUSE_READDIRPLUS_AUTO);
  }
  if (flags & FUSE_ASYNC_DIO) {
    LOG(fshipcld, info) << "FUSE_ASYNC_DIO: asynchronous direct I/O submission="
                        << (flags & FUSE_ASYNC_DIO);
  } else {
    LOG(fshipcld, debug)
        << "FUSE_ASYNC_DIO: asynchronous direct I/O submission="
        << (flags & FUSE_ASYNC_DIO);
  }

#ifdef FUSE_WRITEBACK_CACHE
  if (flags & FUSE_WRITEBACK_CACHE) {
    LOG(fshipcld, info)
        << "FUSE_WRITEBACK_CACHE: kernel supports zero-message opens="
        << (flags & FUSE_WRITEBACK_CACHE);
  } else {
    LOG(fshipcld, debug)
        << "FUSE_WRITEBACK_CACHE: kernel supports zero-message opens="
        << (flags & FUSE_WRITEBACK_CACHE);
  }
#endif
#ifdef FUSE_NO_OPEN_SUPPORT
  if (flags & FUSE_NO_OPEN_SUPPORT) {
    LOG(fshipcld, info)
        << "FUSE_NO_OPEN_SUPPORT: use writeback cache for buffered writes="
        << (flags & FUSE_NO_OPEN_SUPPORT);
  } else {
    LOG(fshipcld, debug)
        << "FUSE_NO_OPEN_SUPPORT: use writeback cache for buffered writes="
        << (flags & FUSE_NO_OPEN_SUPPORT);
  }
#endif
#ifdef FUSE_PARALLEL_DIROPS
  if (flags & FUSE_PARALLEL_DIROPS) {
    LOG(fshipcld, info)
        << "FUSE_PARALLEL_DIROPS: allow parallel lookups and readdir="
        << (flags & FUSE_PARALLEL_DIROPS);
  } else {
    LOG(fshipcld, debug)
        << "FUSE_PARALLEL_DIROPS: allow parallel lookups and readdir="
        << (flags & FUSE_PARALLEL_DIROPS);
  }
#endif
  uint32_t residue =
      flags &
      ~(FUSE_ASYNC_READ | FUSE_POSIX_LOCKS | FUSE_FILE_OPS |
        FUSE_ATOMIC_O_TRUNC | FUSE_EXPORT_SUPPORT | FUSE_BIG_WRITES |
        FUSE_DONT_MASK | FUSE_SPLICE_WRITE | FUSE_SPLICE_MOVE |
        FUSE_SPLICE_READ | FUSE_FLOCK_LOCKS | FUSE_HAS_IOCTL_DIR |
        FUSE_AUTO_INVAL_DATA | FUSE_DO_READDIRPLUS | FUSE_READDIRPLUS_AUTO |
        FUSE_ASYNC_DIO);
  return residue;
}
