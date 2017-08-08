Open optimizing parallelizing system (OpenOPS)
=========================

Introduction
--------
Open optimizing parallelizing system (OpenOPS) is based on optimizing parallelizing system OPS (www.ops.rsu.ru). Step-by-step OpenOPS system is creating by opening of OPS code with all modification varieties.

Optimizing parallelizing system (OPS) can be helpful for optimizing parallelizing compilers creation for new parallelizing computing architectures. For example, experimental web-auto-parallelizer for computing cluster and graphics card http://ops.opsgroup.ru/ was developed on OPS base.
Currently, work is being carried out for displaying the C programming language into PLD accelerator.


The difference between OPS and GCC or LLVM compilers family lies in higher level of internal representation (IR). On the one hand, this aspect reduces the class of internal languages, but, on the other hand, it extends class of target architectures. Moreover, high-level IR is more convenient to creating interactive mode of program optimization into the compiler. At the moment OPS input is C99 language with the Clang parser and Fortran. In the first version OpenOPS is just C with Clang parser. 

Beside the fact that in IR OPS programs maps by Clang parser, there is an opposite mapping – converter from IR OPS into Clang. This allows using OPS in Clang compiler infrastructure. 

The first OpenOPS version will contains only IR OPS and some functions allows to except in IR OPS the C programming language by parser Clang. That’s enough for code generator development from IR OPS to any computing architecture. 

In later OpenOPS versions it supposed to be opened the Dependencies Graph code of information connection which is essential to information dependencies analysis capable to prevents the parallelizing.

It is further proposed to open the program transformations code into IR OPS. Those transformations will be able to transform programme cycles, which don’t allow direct parallelizing (vectorization, pipelining) to process that allows optimization.

How to build OpenOPS for Linux
-------------------------
### You may need

This steps will be necessary before the beginning of building (there are in brackets are names of packets for Ubuntu/Debian):

1.Install standard programs for building apps: g ++, make etc. If you work in Ubuntu/Debian there is enough to set build-essential.
2.Install 7z archiver (packets for Ubuntu/Debian: p7zip-full).
3.Install Qt library not less than 5.0.0. version. For Ubuntu>= 13.04 packets qt5-default, libqt5svg5-dev are enough. For other systems Qt may be used official installer: http://qt-project.org/downloads
4.Install wget utility. 


### Independences Building
OPS project depends on several outside libraries. Beforehand they must be collected from the original texts. Then there are instructions for building and setting of projects. In example, we consider that original texts are located in directory ~/Src and setting of finished libraries is made on directory ~/Lib/<nameoflibrary>. However, there can be used any other directories. They can be deleted after every library setting.

### LLVM и Clang 3.3 building.

The building is carries out by using following commands:
<pre>
cd ~/Src
wget http://llvm.org/releases/3.3/llvm-3.3.src.tar.gz      # download LLVM 3.3
wget http://llvm.org/releases/3.3/cfe-3.3.src.tar.gz       # download Clang 3.3
tar -xvf llvm-3.3.src.tar.gz                               # unpack LLVM
tar -xvf cfe-3.3.src.tar.gz                                # unpack Clang
mv -T cfe-3.3.src llvm-3.3.src/tools/clang                 # move clang into LLVM
mkdir llvm-3.3.build                                       # make catalog of building 
cd llvm-3.3.build
cmake \
  -D CMAKE_BUILD_TYPE=Debug                               `: debug configuration, have to be used Release or RelWithDebInfo` \
  -D LLVM_REQUIRES_RTTI=1                                 `:  to switch on type’s information` \
  -D LLVM_TARGETS_TO_BUILD="X86;Sparc;ARM"                `: target platforms are at random` \
  -D BUILD_SHARED_LIBS=1                                  `: dynamic libraries building` \
  -D LLVM_INCLUDE_EXAMPLES=0 \
  -D LLVM_INCLUDE_TESTS=0 \
  -D CMAKE_INSTALL_PREFIX=~/Lib/llvm-3.3.install \        `: the path to directory to be installed ` \
  ../llvm-3.3.src
make                                                       # start of building
make install                                               # library installation
</pre>

### OpenOPS building

The building of OpenOPS is carries out by using cmake system. The building is managed by the following variable (the major are bold):
* OPS_LLVM_DIR - directory, where LLVM is set;
* OPS_PARALLEL_BUILD – parallelizing building flag. It is on by default.
* BUILD_SHARED_LIBS – flag is managed by dynamic libraries building. It is off by default. It allows to reduce the time of building and the size of resulted executed files.
* Flag is managed by dynamic libraries building. It is off by default.

The building of OpenOPS is carries out by using following commands:
#### make catalog of building 
<pre>mkdir ops-build && cd ops-build</pre>
#### To run cmake for Makefile generation
<pre>cmake \
  -D CMAKE_BUILD_TYPE=Debug              `: debug configuration`\
  -D OPS_LLVM_DIR=~/Lib/llvm-3.3.install `: the path to LLVM`\
  -D BUILD_SHARED_LIBS=1                 `: use dynamic libraries`\
  ../ops</pre>
#### To run building
<pre>make</pre>
