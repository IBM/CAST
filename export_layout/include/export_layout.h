/***************************************************************************
 *
 * Copyright IBM Corporation 2016,2017. All Rights Reserved
 *
 * This file is part of the CORAL export_layout kernel module.
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice, 
 *     this list of conditions and the following disclaimer. 
 *  2. Redistributions in binary form must reproduce the above copyright 
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 *  3. The name of International Business Machines may not be used to endorse 
 *     or promote products derived from this software without specific prior 
 *     written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS "AS IS" AND ANY 
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR 
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Alternatively, provided that this notice is retained in full, this
 * software may be distributed under the terms of the GNU General
 * Public License ("GPL") as published by the Free Software Foundation;
 * either version 2 of the License, or (at your option) any later version,
 * in which case the provisions of the GPL apply INSTEAD OF those given above.
 *
 ****************************************************************************/

#include <linux/types.h>

struct export_layout_extent {
	__u64 file_offset;
	__u64 blkdev_offset;
	__u64 length;
};

struct export_layout_transfer_setup {
	int fd;					// in
	__u32 writing;				// in
	__u64 offset;				// in
	__u64 length;				// in
	__u32 extent_count_max;			// in
	__u32 extent_count;			// out
	struct export_layout_extent extent[];	// out
};

struct export_layout_transfer_finalize {
	int status;
};

#define EXPORT_LAYOUT_IOC_TRANSFER_SETUP \
	_IOWR('e', 1, struct export_layout_transfer_setup)

#define EXPORT_LAYOUT_IOC_TRANSFER_FINALIZE \
	_IOWR('e', 2, struct export_layout_transfer_finalize)
