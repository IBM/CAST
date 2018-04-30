###########################################################
#     llvm_build.sh
# 
#     Copyright IBM Corporation 2015,2016. All Rights Reserved
# 
#     This program is licensed under the terms of the Eclipse Public License
#     v1.0 as published by the Eclipse Foundation and available at
#     http://www.eclipse.org/legal/epl-v10.html
# 
#     U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#     restricted by GSA ADP Schedule Contract with IBM Corp.
###########################################################


majorversion=RELEASE_600
minorversion=final
LLVM_TAG=tags/$majorversion/$minorversion

mkdir llvm.$majorversion.$minorversion
cd llvm.$majorversion.$minorversion

svn co http://llvm.org/svn/llvm-project/llvm/$LLVM_TAG llvm
cd llvm/tools
svn co http://llvm.org/svn/llvm-project/cfe/$LLVM_TAG clang
#svn co http://llvm.org/svn/llvm-project/lldb/$LLVM_TAG lldb
cd ../..
cd llvm/projects
svn co http://llvm.org/svn/llvm-project/compiler-rt/$LLVM_TAG compiler-rt
cd ../..
mkdir build
cd build
/opt/ibm/cmake/bin/cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release ../llvm
make -j160
#sudo make install

echo To install:
echo % cd llvm/build
echo % sudo make install
