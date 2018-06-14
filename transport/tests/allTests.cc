/*******************************************************************************
 |    allTests.cc
 |
 |  © Copyright IBM Corporation 2015,2016. All Rights Reserved
 |
 |    This program is licensed under the terms of the Eclipse Public License
 |    v1.0 as published by the Eclipse Foundation and available at
 |    http://www.eclipse.org/legal/epl-v10.html
 |
 |    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
 |    restricted by GSA ADP Schedule Contract with IBM Corp.
 *******************************************************************************/

#include <iostream>
#include <vector>

#include "bb/include/bbapi.h"
#include "bb/include/bbapi_types.h"
#include "Msg.h"

// Default log for this 'environment'...
extern txp::Log d_log;

txp::Log d_log(txp::Log::CONSOLE_AND_LOGFILE, "/bglhome/dherms/git/bluecoral/work/transport/tests/allTests.log");

int main(int argc, char** argv) {
	int rc = 0;

    rc = BB_InitLibrary(0, BBAPI_CLIENTVERSIONSTR);

    if(!rc)
    {
	    d_log.open();

	    d_log.enter(argv[0], __func__);

	    //NOTE:  Even though the message types are for FUSE_OPCODES, the
	    //       attributes added to these messages are not typical for those
	    //       specific opcodes.  Attributes are added below for test purposes
	    //       only...

	    // Create Msg_1 and dump it...
	    txp::Msg l_Msg_1(txp::FUSE_LOOKUP);
	    l_Msg_1.dump(d_log, "Msg_1, before allocating buffer");

	    // Allocate a heap buffer for Msg_1 and dump it again...
	    l_Msg_1.allocateHeapBuffer();
	    l_Msg_1.dump(d_log, "Msg_1, after allocating buffer");

	    // Copy Msg_1 to Msg_2 and dump Msg_2...
	    txp::Msg l_Msg_2 = l_Msg_1;
	    l_Msg_2.dump(d_log, "Msg_2, copied from Msg_1");

	    // Free the heap buffer associated with Msg_1 and dump it again...
	    l_Msg_1.freeHeapBuffer();
	    l_Msg_1.dump(d_log, "Msg_1, after freeing buffer");

	    // Create Msg_3 with a different version than the default and dump it...
	    txp::Version l_Version_1 = txp::Version(2, 1);
	    txp::Msg l_Msg_3(txp::FUSE_OPEN, l_Version_1);
	    l_Msg_3.dump(d_log, "Msg_3, (major,minor) = (2,1)");

	    // Create some 'attribute values' and associate them with Msg_3.
	    // Then, dump Msg_3...
	    uint64_t l_ino=10, l_size=11, l_blocks=12;
	    uint32_t l_atimensec=17;

	    txp::Attr_uint64 l_Attr1(txp::ino, l_ino);
	    l_Msg_3.addAttribute(&l_Attr1);
	    txp::Attr_uint64 l_Attr2(txp::size, l_size);
	    l_Msg_3.addAttribute(&l_Attr2);
	    txp::Attr_uint64 l_Attr3(txp::blocks, l_blocks);
	    l_Msg_3.addAttribute(&l_Attr3);
	    txp::Attr_uint32 l_Attr4(txp::atimensec, l_atimensec);
	    l_Msg_3.addAttribute(&l_Attr4);
	    l_Msg_3.dump(d_log, "Msg_3 after attributes added");

	    // Create l_Msg_4 with all possible attribute types and dump that message...
        txp::Msg l_Msg_4(txp::FUSE_OPENDIR);

	    char l_char = 'a';
	    uint8_t l_uint8 = 12;
	    uint16_t l_uint16 = 23;
	    uint32_t l_uint32 = 34;
	    uint64_t l_uint64 = 45;
	    int8_t l_int8 = 56;
	    int16_t l_int16 = 67;
	    int32_t l_int32 = -76;
	    int64_t l_int64 = -87;
	    char* l_ptr_char = &l_char;
	    uint8_t* l_ptr_uint8 = &l_uint8;
	    uint16_t* l_ptr_uint16 = &l_uint16;
	    uint32_t* l_ptr_uint32 = &l_uint32;
	    uint64_t* l_ptr_uint64 = &l_uint64;
	    int8_t* l_ptr_int8 = &l_int8;
	    int16_t* l_ptr_int16 = &l_int16;
	    int32_t* l_ptr_int32 = &l_int32;
	    int64_t* l_ptr_int64 = &l_int64;
	    char l_char_array[24] = "Here is some data...";
	    txp::CharArray_Element l_Element_1 = txp::CharArray_Element(sizeof(l_char_array), l_char_array);
	    txp::CharArray_Element l_Element_2 = txp::CharArray_Element(sizeof(l_char_array), l_char_array);
	    txp::CharArray_Element l_Element_3 = txp::CharArray_Element(sizeof(l_char_array), l_char_array);
	    txp::CharArray l_array_of_char_arrays = txp::CharArray();
	    l_array_of_char_arrays.push_back(l_Element_1);
	    l_array_of_char_arrays.push_back(l_Element_2);
	    l_array_of_char_arrays.push_back(l_Element_3);
	    txp::CharArray* l_ptr_array_of_char_arrays = &l_array_of_char_arrays;

	    txp::Attr_char l_Attr_1(txp::arg, l_char);
	    txp::Attr_uint8 l_Attr_2(txp::atime, l_uint8);
	    txp::Attr_uint16 l_Attr_3(txp::atimensec, l_uint16);
	    txp::Attr_uint32 l_Attr_4(txp::attr_valid, l_uint32);
	    txp::Attr_uint64 l_Attr_5(txp::attr_valid_nsec, l_uint64);
	    txp::Attr_int8 l_Attr_6(txp::bavail, l_int8);
	    txp::Attr_int16 l_Attr_7(txp::bfree, l_int16);
	    txp::Attr_int32 l_Attr_8(txp::blksize, l_int32);
	    txp::Attr_int64 l_Attr_9(txp::block, l_int64);
	    txp::AttrPtr_char l_Attr_10(txp::blocks, l_ptr_char);
	    txp::AttrPtr_uint8 l_Attr_11(txp::blocksize, l_ptr_uint8);
	    txp::AttrPtr_uint16 l_Attr_12(txp::bsize, l_ptr_uint16);
	    txp::AttrPtr_uint32 l_Attr_13(txp::cmd, l_ptr_uint32);
	    txp::AttrPtr_uint64 l_Attr_14(txp::congestion_threshold, l_ptr_uint64);
	    txp::AttrPtr_int8 l_Attr_15(txp::ctime, l_ptr_int8);
	    txp::AttrPtr_int16 l_Attr_16(txp::ctimensec, l_ptr_int16);
	    txp::AttrPtr_int32 l_Attr_17(txp::dev_major, l_ptr_int32);
	    txp::AttrPtr_int64 l_Attr_18(txp::dev_minor, l_ptr_int64);
	    txp::AttrPtr_char_array l_Attr_19(txp::end, l_char_array, 24);
	    txp::AttrPtr_array_of_char_arrays l_Attr_20(txp::buffer, &l_array_of_char_arrays);

	    l_Msg_4.addAttribute(&l_Attr_1);
	    l_Msg_4.addAttribute(&l_Attr_2);
	    l_Msg_4.addAttribute(&l_Attr_3);
	    l_Msg_4.addAttribute(&l_Attr_4);
	    l_Msg_4.addAttribute(&l_Attr_5);
	    l_Msg_4.addAttribute(&l_Attr_6);
	    l_Msg_4.addAttribute(&l_Attr_7);
	    l_Msg_4.addAttribute(&l_Attr_8);
	    l_Msg_4.addAttribute(&l_Attr_9);
	    l_Msg_4.addAttribute(&l_Attr_10);
	    l_Msg_4.addAttribute(&l_Attr_11);
	    l_Msg_4.addAttribute(&l_Attr_12);
	    l_Msg_4.addAttribute(&l_Attr_13);
	    l_Msg_4.addAttribute(&l_Attr_14);
	    l_Msg_4.addAttribute(&l_Attr_15);
	    l_Msg_4.addAttribute(&l_Attr_16);
	    l_Msg_4.addAttribute(&l_Attr_17);
	    l_Msg_4.addAttribute(&l_Attr_18);
	    l_Msg_4.addAttribute(&l_Attr_19);
	    l_Msg_4.addAttribute(&l_Attr_20);
	    l_Msg_4.dump(d_log, "Msg_4 with 20 attributes added, before serializeToHeapBuffer()");
	    // Allocate a heap buffer to l_Msg_4.  Build that message to the heap buffer and dump it...
	    // NOTE:  For any pointers to data, this build method copies those pointers into the heap space.
	    if (l_Msg_4.allocateHeapBuffer()) {
	    	printf("ERROR - Failure - l_Msg_4.allocateHeapBuffer()\n");
	    }

	    if (l_Msg_4.serializeToHeapBuffer()<=0) {
	    	printf("ERROR - Failure - l_Msg_4.serializeToHeapBuffer()\n");
	    }
	    l_Msg_4.dump(d_log, "Msg_4, after serializeToHeapBuffer()");

	    // Reset the heap buffer for l_Msg_4.  Build the message again to the heap buffer and dump it...
	    // NOTE:  For any pointers to data, this build method copies the data values into the heap space.
	    if (l_Msg_4.serializeWithValuesToHeapBuffer()<=0) {
	    	printf("ERROR - Failure - l_Msg_4.serializeWithValuesToHeapBuffer()\n");
	    }
	    l_Msg_4.dump(d_log, "Msg_4, after serializeWithValuesToHeapBuffer()");

	    // Create l_Msg_4b with a copy of all possible attribute types and dump that message...
        txp::Msg l_Msg_4b(txp::FUSE_OPENDIR);
	    txp::Attr_char l_Attr_1b = l_Attr_1;
	    txp::Attr_uint8 l_Attr_2b = l_Attr_2;
	    txp::Attr_uint16 l_Attr_3b = l_Attr_3;
	    txp::Attr_uint32 l_Attr_4b = l_Attr_4;
	    txp::Attr_uint64 l_Attr_5b = l_Attr_5;
	    txp::Attr_int8 l_Attr_6b = l_Attr_6;
	    txp::Attr_int16 l_Attr_7b = l_Attr_7;
	    txp::Attr_int32 l_Attr_8b = l_Attr_8;
	    txp::Attr_int64 l_Attr_9b = l_Attr_9;
	    txp::AttrPtr_char l_Attr_10b = l_Attr_10;
	    txp::AttrPtr_uint8 l_Attr_11b = l_Attr_11;
	    txp::AttrPtr_uint16 l_Attr_12b = l_Attr_12;
	    txp::AttrPtr_uint32 l_Attr_13b = l_Attr_13;
	    txp::AttrPtr_uint64 l_Attr_14b = l_Attr_14;
	    txp::AttrPtr_int8 l_Attr_15b = l_Attr_15;
	    txp::AttrPtr_int16 l_Attr_16b = l_Attr_16;
	    txp::AttrPtr_int32 l_Attr_17b = l_Attr_17;
	    txp::AttrPtr_int64 l_Attr_18b = l_Attr_18;
	    txp::AttrPtr_char_array l_Attr_19b = l_Attr_19;
	    txp::AttrPtr_array_of_char_arrays l_Attr_20b = l_Attr_20;

	    l_Msg_4b.addAttribute(&l_Attr_1b);
	    l_Msg_4b.addAttribute(&l_Attr_2b);
	    l_Msg_4b.addAttribute(&l_Attr_3b);
	    l_Msg_4b.addAttribute(&l_Attr_4b);
	    l_Msg_4b.addAttribute(&l_Attr_5b);
	    l_Msg_4b.addAttribute(&l_Attr_6b);
	    l_Msg_4b.addAttribute(&l_Attr_7b);
	    l_Msg_4b.addAttribute(&l_Attr_8b);
	    l_Msg_4b.addAttribute(&l_Attr_9b);
	    l_Msg_4b.addAttribute(&l_Attr_10b);
	    l_Msg_4b.addAttribute(&l_Attr_11b);
	    l_Msg_4b.addAttribute(&l_Attr_12b);
	    l_Msg_4b.addAttribute(&l_Attr_13b);
	    l_Msg_4b.addAttribute(&l_Attr_14b);
	    l_Msg_4b.addAttribute(&l_Attr_15b);
	    l_Msg_4b.addAttribute(&l_Attr_16b);
	    l_Msg_4b.addAttribute(&l_Attr_17b);
	    l_Msg_4b.addAttribute(&l_Attr_18b);
	    l_Msg_4b.addAttribute(&l_Attr_19b);
	    l_Msg_4b.addAttribute(&l_Attr_20b);

	    l_Msg_4b.dump(d_log, "Msg_4b with 20 copied attributes added");

        txp::Msg l_Msg_4c(txp::FUSE_OPENDIR);
	    l_Msg_4c.addAttribute(txp::arg, l_char);
	    l_Msg_4c.addAttribute(txp::atime, l_uint8);
	    l_Msg_4c.addAttribute(txp::atimensec, l_uint16);
	    l_Msg_4c.addAttribute(txp::attr_valid, l_uint32);
	    l_Msg_4c.addAttribute(txp::attr_valid_nsec, l_uint64);
	    l_Msg_4c.addAttribute(txp::bavail, l_int8);
	    l_Msg_4c.addAttribute(txp::bfree, l_int16);
	    l_Msg_4c.addAttribute(txp::blksize, l_int32);
	    l_Msg_4c.addAttribute(txp::block, l_int64);
	    l_Msg_4c.addAttribute(txp::blocks, l_ptr_char);
	    l_Msg_4c.addAttribute(txp::blocksize, l_ptr_uint8);
	    l_Msg_4c.addAttribute(txp::bsize, l_ptr_uint16);
	    l_Msg_4c.addAttribute(txp::cmd, l_ptr_uint32);
	    l_Msg_4c.addAttribute(txp::congestion_threshold, l_ptr_uint64);
	    l_Msg_4c.addAttribute(txp::ctime, l_ptr_int8);
	    l_Msg_4c.addAttribute(txp::ctimensec, l_ptr_int16);
	    l_Msg_4c.addAttribute(txp::dev_major, l_ptr_int32);
	    l_Msg_4c.addAttribute(txp::dev_minor, l_ptr_int64);
	    l_Msg_4c.addAttribute(txp::end, l_char_array, 24);
	    l_Msg_4c.addAttribute(txp::buffer, l_ptr_array_of_char_arrays);

	    l_Msg_4c.dump(d_log, "Msg_4c with 20 directly added attributes");

	    // Example of how a FUSE mkdir message may be built...
	    // NOTE:  The following declared variables probably will reside
	    //        in the FUSE header file, in one of the structs called
	    //        out in enum id in Common.h...  The name and name length
	    //        will be passed in another structure passed to the
	    //        called routine.
	    uint32_t mode = 0x22;
	    uint32_t umask = 0x755;
	    char name[22] = "/home/dlherms/new_dir";
	    uint32_t namelen = sizeof(name);

	    // Create the message...  If a version is not specified, the code will generate
	    // the latest version of the message...
	    txp::Msg l_Msg_5(txp::FUSE_MKDIR);
	    txp::Attr_uint32 l_Mode(txp::mode, mode);
	    txp::Attr_uint32 l_Umask(txp::umask, umask);
	    txp::AttrPtr_char_array l_Name(txp::name, name, namelen);
	    l_Msg_5.addAttribute(&l_Mode);
	    l_Msg_5.addAttribute(&l_Umask);
	    l_Msg_5.addAttribute(&l_Name);

	    // l_Msg_5 now represents the FUSE_MKDIR request that will eventually
	    // be RDMA'ed to the I/O node.  When preping to RDMA the message, the
	    // message will then need to be built into a location within the
	    // RDMA buffer pool.  For our prototype, we will be using sockets, so
	    // this meesage will be built into heap memory.

	    // Allocate heap of a default size for message 5...
	    if (l_Msg_5.allocateHeapBuffer()) {
	    	printf("ERROR - Failure - l_Msg_5.allocateHeapBuffer()\n");
	    }

	    // Build the message into heap...
	    if (l_Msg_5.serializeWithValuesToHeapBuffer()<=0) {
	    	printf("ERROR - Failure - l_Msg_5.serializeWithValuesToHeapBuffer()\n");
	    }

	    // It is now ready to send...  We will just dump it here...
	    l_Msg_5.dump(d_log, "Msg_5, FUSE_MKDIR");

	    // Messages 6 and 7 will continue 5...
	    txp::Msg l_Msg_6(txp::FUSE_MKDIR);
	    txp::Msg l_Msg_7(txp::FUSE_MKDIR);

	    // Allocate 16K heap for message 6...
	    if (l_Msg_6.allocateHeapBuffer(16384)) {
	    	printf("ERROR - Failure - l_Msg_6.allocateHeapBuffer(16384)\n");
	    }

	    // Allocate 32K heap for message 7...
	    if (l_Msg_7.allocateHeapBuffer(32768)) {
	    	printf("ERROR - Failure - l_Msg_7.allocateHeapBuffer(32768)\n");
	    }

	    // Chain the messages...
	    if (l_Msg_5.nextMsg(l_Msg_6)) {
	    	printf("ERROR - Failure - l_Msg_5.nextMsg(l_Msg_6)\n");
	    }
	    if (l_Msg_6.nextMsg(l_Msg_7)) {
	    	printf("ERROR - Failure - l_Msg_6.nextMsg(l_Msg_7)\n");
	    }

	    // Build the altered message 5 into heap...
	    if (l_Msg_5.serializeWithValuesToHeapBuffer()<=0) {
	    	printf("ERROR - Failure - l_Msg_5.serializeWithValuesToHeapBuffer()\n");
	    }

	    // Build message 6 into heap...
	    if (l_Msg_6.serializeWithValuesToHeapBuffer()<=0) {
	    	printf("ERROR - Failure - l_Msg_6.serializeWithValuesToHeapBuffer()\n");
	    }

	    // Build message 7 into heap...
	    if (l_Msg_7.serializeWithValuesToHeapBuffer()<=0) {
	    	printf("ERROR - Failure - l_Msg_7.serializeWithValuesToHeapBuffer()\n");
	    }

	    // Dump message 5 and 6...
	    l_Msg_5.dump(d_log, "Msg_5, after Msg_6 was marked as a continuation msg...");
	    l_Msg_6.dump(d_log, "Msg_6 is a continuation of Msg_5");
	    l_Msg_7.dump(d_log, "Msg_7 is a continuation of Msg_6");

	    // Create message from message 5's heap buffer with default copy option and dump it...
	    txp::Msg* l_Msg_A = 0;
	    if (txp::Msg::deserializeToMsg(l_Msg_A, l_Msg_5.getHeapBufferPtr())) {
	    	printf("ERROR - Failure - txp::Msg::deserializeToMsg(l_Msg_5.getHeapBufferPtr())\n");
	    } else {
	    	l_Msg_A->dump(d_log, "Re-fluffed Msg_5, default copy option...");
	    }

	    // Create message from message 6's heap buffer with default copy option and dump it...
	    txp::Msg* l_Msg_B = 0;
	    if (txp::Msg::deserializeToMsg(l_Msg_B, l_Msg_6.getHeapBufferPtr())) {
	    	printf("ERROR - Failure - txp::Msg::deserializeToMsg(l_Msg_6.getHeapBufferPtr())\n");
	    } else {
	    	l_Msg_B->dump(d_log, "Re-fluffed Msg_6, default copy option...");
	    }

	    // Create message from message 7's heap buffer with default copy option and dump it...
	    txp::Msg* l_Msg_C = 0;
	    if (txp::Msg::deserializeToMsg(l_Msg_C, l_Msg_7.getHeapBufferPtr())) {
	    	printf("ERROR - Failure - txp::Msg::deserializeToMsg(l_Msg_7.getHeapBufferPtr())\n");
	    } else {
	    	l_Msg_C->dump(d_log, "Re-fluffed Msg_7, default copy option...");
	    }

	    //  Create a response message to message 5 and dump it...
	    txp::Msg* l_Msg_8 = 0;
	    if (l_Msg_5.buildResponseMsg(l_Msg_8)) {
	    	printf("ERROR - Failure - l_Msg_5.buildResponseMsg(l_Msg_8)\n");
	    }
	    l_Msg_8->dump(d_log, "Response message built for Msg_5...");

	    //  Create a continuation message to the response message...
	    txp::Msg l_Msg_9(l_Msg_8->getMsgId());
	    l_Msg_9.dump(d_log, "Continuation message to the response message...");

	    // Add Msg_9 as a continuation to Msg_8...  Dump both...
	    l_Msg_8->nextMsg(l_Msg_9);
	    l_Msg_8->dump(d_log, "Response message built for Msg_5, after Msg_9 added as a continuation message...");
	    l_Msg_9.dump(d_log, "Continuation message to the response message, after it was added to Msg_8...");

	    // Create message from message 4's heap buffer with all copy options and dump it...
	    txp::Msg* l_Msg_D = 0;
        if (txp::Msg::deserializeToMsg(l_Msg_D, l_Msg_4.getHeapBufferPtr(), txp::COPY_DATA_TO_OBJECT)) {
	    	printf("ERROR - Failure - txp::Msg::deserializeToMsg(l_Msg_4.getHeapBufferPtr(), txp::BuildOption::COPY_DATA_TO_OBJECT)\n");
	    } else {
	    	l_Msg_D->dump(d_log, "l_Msg_D, re-fluffed Msg_4, COPY_DATA_TO_OBJECT...");
	    }

	    txp::Msg* l_Msg_E = 0;
	    rc = txp::Msg::deserializeToMsg(l_Msg_E, l_Msg_4.getHeapBufferPtr(), txp::COPY_DATA_TO_HEAP);
	    if (rc) {
	    	printf("ERROR - Failure - txp::Msg::deserializeToMsg(l_Msg_4.getHeapBufferPtr(), txp::BuildOption::COPY_DATA_TO_HEAP), RC=%d\n", rc);
	    } else {
	    	l_Msg_E->dump(d_log, "l_Msg_E, re-fluffed Msg_4, COPY_DATA_TO_HEAP...");
	    }

	    txp::Msg* l_Msg_F = 0;
	    if (txp::Msg::deserializeToMsg(l_Msg_F, l_Msg_4.getHeapBufferPtr(), txp::DO_NOT_COPY_DATA)) {
	    	printf("ERROR - Failure - txp::Msg::deserializeToMsg(l_Msg_4.getHeapBufferPtr(), txp::BuildOption::DO_NOT_COPY_DATA)\n");
	    } else {
	    	l_Msg_F->dump(d_log, "l_Msg_F, re-fluffed Msg_4, DO_NOT_COPY_DATA...");
	    }

	    // Build message D, E and F into the heap buffer...  Dump those messages...

	    if (l_Msg_D) {
	    	l_Msg_D->allocateHeapBuffer();
	    	// Build message D into heap...
	    	if (l_Msg_D->serializeToHeapBuffer()<=0) {
	    		printf("ERROR - Failure - l_Msg_D.serializeToHeapBuffer()\n");
	    	}
	    	l_Msg_D->dump(d_log, "l_Msg_D, re-fluffed Msg_4, COPY_DATA_TO_OBJECT, then serializeToHeapBuffer to heap buffer...");
	    }

	    if (l_Msg_D) {
	    	l_Msg_D->allocateHeapBuffer();
	    	// Build message D into heap...
	    	if (l_Msg_D->serializeWithValuesToHeapBuffer()<=0) {
	    		printf("ERROR - Failure - l_Msg_D.serializeWithValuesToHeapBuffer()\n");
	    	}
	    	l_Msg_D->dump(d_log, "l_Msg_D, re-fluffed Msg_4, COPY_DATA_TO_OBJECT, then serializeWithValuesToHeapBuffer to heap buffer...");
	    }

	    if (l_Msg_E) {
	    	l_Msg_E->allocateHeapBuffer();
	    	// Build message E into heap...
	    	if (l_Msg_E->serializeToHeapBuffer()<=0) {
	    		printf("ERROR - Failure - l_Msg_E.serializeToHeapBuffer()\n");
	    	}
	    	l_Msg_E->dump(d_log, "l_Msg_E, re-fluffed Msg_4, COPY_DATA_TO_HEAP, then serializeToHeapBuffer to heap buffer...");
	    }

	    if (l_Msg_E) {
	    	l_Msg_E->allocateHeapBuffer();
	    	// Build message E into heap...
	    	if (l_Msg_E->serializeWithValuesToHeapBuffer()<=0) {
	    		printf("ERROR - Failure - l_Msg_E.serializeWithValuesToHeapBuffer()\n");
	    	}
	    	l_Msg_E->dump(d_log, "l_Msg_E, re-fluffed Msg_4, COPY_DATA_TO_HEAP, then serializeWithValuesToHeapBuffer to heap buffer...");
	    }

	    if (l_Msg_F) {
	    	l_Msg_F->allocateHeapBuffer();
	    	// Build message F into heap...
	    	if (l_Msg_F->serializeToHeapBuffer()<=0) {
	    		printf("ERROR - Failure - l_Msg_F.serializeToHeapBuffer()\n");
	    	}
	    	l_Msg_F->dump(d_log, "l_Msg_F, re-fluffed Msg_4, DO_NOT_COPY_DATA, then serializeToHeapBuffer to heap buffer...");
	    }

	    if (l_Msg_F) {
	    	l_Msg_F->allocateHeapBuffer();
	    	// Build message F into heap...
	    	if (l_Msg_F->serializeWithValuesToHeapBuffer()<=0) {
	    		printf("ERROR - Failure - l_Msg_F.serializeWithValuesToHeapBuffer()\n");
	    	}
	    	l_Msg_F->dump(d_log, "l_Msg_F, re-fluffed Msg_4, DO_NOT_COPY_DATA, then serializeWithValuesToHeapBuffer to heap buffer...");
	    }

	    // Create a message in heap and dump it...
	    txp::Msg* l_Msg_10 = 0;
	    txp::Msg::buildMsg(txp::FUSE_OPEN, l_Msg_10);
	    l_Msg_10->dump(d_log, "Msg_10 built into heap...");

	    txp::Attr_char* l_Attr_A = 0;
	    txp::Attr_uint8* l_Attr_B = 0;
	    txp::Attr_uint16* l_Attr_C = 0;
	    txp::Attr_uint32* l_Attr_D = 0;
	    txp::Attr_uint64* l_Attr_E = 0;
	    txp::Attr_int8* l_Attr_F = 0;
	    txp::Attr_int16* l_Attr_G = 0;
	    txp::Attr_int32* l_Attr_H = 0;
	    txp::Attr_int64* l_Attr_I = 0;
	    txp::AttrPtr_char* l_Attr_J = 0;
	    txp::AttrPtr_uint8* l_Attr_K = 0;
	    txp::AttrPtr_uint16* l_Attr_L = 0;
	    txp::AttrPtr_uint32* l_Attr_M = 0;
	    txp::AttrPtr_uint64* l_Attr_N = 0;
	    txp::AttrPtr_int8* l_Attr_O = 0;
	    txp::AttrPtr_int16* l_Attr_P = 0;
	    txp::AttrPtr_int32* l_Attr_Q = 0;
	    txp::AttrPtr_int64* l_Attr_R = 0;
	    txp::AttrPtr_char_array* l_Attr_S = 0;

	    txp::Attr_char::buildAttr_char(txp::AttributeName(txp::entry_valid), l_char, l_Attr_A);
	    txp::Attr_uint8::buildAttr_uint8(txp::entry_valid_nsec, l_uint8, l_Attr_B);
	    txp::Attr_uint16::buildAttr_uint16(txp::error, l_uint16, l_Attr_C);
	    txp::Attr_uint32::buildAttr_uint32(txp::ffree, l_uint32, l_Attr_D);
	    txp::Attr_uint64::buildAttr_uint64(txp::fh, l_uint64, l_Attr_E);
	    txp::Attr_int8::buildAttr_int8(txp::files, l_int8, l_Attr_F);
	    txp::Attr_int16::buildAttr_int16(txp::flags, l_int16, l_Attr_G);
	    txp::Attr_int32::buildAttr_int32(txp::frsize, l_int32, l_Attr_H);
	    txp::Attr_int64::buildAttr_int64(txp::fsync_flags, l_int64, l_Attr_I);
	    txp::AttrPtr_char::buildAttrPtr_char(txp::generation, l_ptr_char, l_Attr_J);
	    txp::AttrPtr_uint8::buildAttrPtr_uint8(txp::getattr_flags, l_ptr_uint8, l_Attr_K);
	    txp::AttrPtr_uint16::buildAttrPtr_uint16(txp::gid, l_ptr_uint16, l_Attr_L);
	    txp::AttrPtr_uint32::buildAttrPtr_uint32(txp::in_iovs, l_ptr_uint32, l_Attr_M);
	    txp::AttrPtr_uint64::buildAttrPtr_uint64(txp::in_size, l_ptr_uint64, l_Attr_N);
	    txp::AttrPtr_int8::buildAttrPtr_int8(txp::ino, l_ptr_int8, l_Attr_O);
	    txp::AttrPtr_int16::buildAttrPtr_int16(txp::kh, l_ptr_int16, l_Attr_P);
	    txp::AttrPtr_int32::buildAttrPtr_int32(txp::len, l_ptr_int32, l_Attr_Q);
	    txp::AttrPtr_int64::buildAttrPtr_int64(txp::length, l_ptr_int64, l_Attr_R);
	    txp::AttrPtr_char_array::buildAttrPtr_char_array(txp::lk_flags, l_char_array, 24, l_Attr_S);

	    l_Msg_10->addAttribute(&l_Attr_1);
	    l_Msg_10->addAttribute(&l_Attr_2);
	    l_Msg_10->addAttribute(&l_Attr_3);
	    l_Msg_10->addAttribute(&l_Attr_4);
	    l_Msg_10->addAttribute(&l_Attr_5);
	    l_Msg_10->addAttribute(&l_Attr_6);
	    l_Msg_10->addAttribute(&l_Attr_7);
	    l_Msg_10->addAttribute(&l_Attr_8);
	    l_Msg_10->addAttribute(&l_Attr_9);
	    l_Msg_10->addAttribute(&l_Attr_10);
	    l_Msg_10->addAttribute(&l_Attr_11);
	    l_Msg_10->addAttribute(&l_Attr_12);
	    l_Msg_10->addAttribute(&l_Attr_13);
	    l_Msg_10->addAttribute(&l_Attr_14);
	    l_Msg_10->addAttribute(&l_Attr_15);
	    l_Msg_10->addAttribute(&l_Attr_16);
	    l_Msg_10->addAttribute(&l_Attr_17);
	    l_Msg_10->addAttribute(&l_Attr_18);
	    l_Msg_10->addAttribute(&l_Attr_19);

	    l_Msg_10->dump(d_log, "Msg_10 after adding 19 attributes...");

	    // Allocate heap buffer for message 10...
	    if (l_Msg_10->allocateHeapBuffer()) {
	    	printf("ERROR - Failure - l_Msg_10.allocateHeapBuffer()\n");
	    }

	    // Build message 10 into the heap buffer...
	    if (l_Msg_10->serializeToHeapBuffer()<=0) {
	    	printf("ERROR - Failure - l_Msg_10.serializeToHeapBuffer()\n");
	    }
	    l_Msg_10->dump(d_log, "Msg_10 after building into the heap buffer...");

	    // Build message 10 as values into the heap buffer...
	    if (l_Msg_10->serializeWithValuesToHeapBuffer()<=0) {
	    	printf("ERROR - Failure - l_Msg_10.serializeWithValuesToHeapBuffer()\n");
	    }
	    l_Msg_10->dump(d_log, "Msg_10 after building values into the heap buffer...");

	    d_log.exit(argv[0], __func__, rc);
    }

	printf("allTests complete, rc = %d\n", rc);

	return rc;
}
