# Add metadata on loop

## Steps
### 1. Put pass source code in the right path

The source code and files for this pass are available in the LLVM source tree in the `lib/Transforms/AddMetadataOnLoop` directory. Create it one for the initial creation.

### 2. Update `CMakeLists.txt` files

#### 2.1 Add following code into `lib/Transforms/AddMetadataOnLoop/CMakeLists.txt`. Create it one for the initial creation.
```
  add_llvm_loadable_module( LLVMAddMetadataOnLoop MODULE
    AddMetadataOnLoop.cpp
    PLUGIN_TOOL
    opt
  )
```
  
#### 2.2. Add the following line into `lib/Transforms/CMakeLists.txt`:
`add_subdirectory(AddMetadataOnLoop)`
  
  
### 3. `cd` to `$TESTCODE_DIR`. 
Suppose the current c++ file is `$TESTCODE_DIR/main.cpp`. The corresponding original readable LLVM IR filename: `main.ll`, The updated `.ll` filename after running the pass: `main-inst.ll`, in which the metadata is added.
   
```
   $LLVM_OBJECT_DIR=/your/llvm/object/path
   
   $LLVM_OBJECT_DIR/bin/clang++ -g -emit-llvm main.cpp -S -o main.ll
   $LLVM_OBJECT_DIR/bin/opt -load $LLVM_OBJECT_DIR/lib/LLVMAddMetadataOnLoop.so -addmetadataonloop main.ll -S -o main-inst.ll
```
   
## Result:
   1. Can add metadata to the back edge for each loop
   2. Failed to add metadata to the instruction generates the loop induction variable update, i.e. step instruction for each loop  
   3. The pass can be compiled into a shared object that can be loaded by llvm opt with command line option and invoked by command line option flags
   4. Tested in LLVM 7.0.1

<!--
## NOTE:
   1. Does not consider the loop induction variable initialization outside the loop
-->
   
## Others:
LLVM build steps (clang code is under llvm/tools, may takes about hour):

  
```
   $cd /path/to/put/llvm/objects/
   $mkdir build
   $cd build
   $cmake -G "Unix Makefiles" -DLLVM_TARGETS_TO_BUILD="X86;ARM" -DCMAKE_BUILD_TYPE=Release -DLLVM_ENABLE_ASSERTIONS=on /path/to/llvm/src/
   $make -j4
```
# Done
