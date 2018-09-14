# CSM API Python Bindings Guide

## Tables of Contents:
1. About
1. User Notes
1. Importing
1. Connection to CSM
1. Accessing the CSM API
1. [FAQ - Frequently Asked Questions](https://github.com/NickyDaB/CAST/blob/research/csm_python_apis/csmi/python_samples/guide.md#faq---frequently-asked-questions)

## About
The CSM API Python Bindings library works similar to other C Python binding libraries. CSM APIs can be accessed in Python because they are bound to C via Boost. More technical details can be found here: https://wiki.python.org/moin/boost.python/GettingStarted but understanding all of this is not required to use CSM APIs in Python. This guide provides a central location for users looking to utilize CSM APIs via Python. If you believe this guide to be incomplete, then please make a pull request with your additional content. 

## User Notes
Accessing CSM APIs in Python is very similar to accessing them in C. If you are familiar with the process, then you are already in a good position. If not, then the CSM API team suggest reading up on some CSM API documentation and guides. 

## Importing
Before writing your script and accessing CSM APIs, you must first import the CSM library into your script. 

```Python

import sys
sys.path.append('.')

#add the python library to the path 
sys.path.append('/u/nbuonar/repos/CAST/work/csm/lib')

# eventually append the path as part of an rpm install 

import lib_csm_py as csm
import lib_csm_inv_py as inv
```

*ToDo:* 
* The above import path: `'/u/nbuonar/repos/CAST/work/csm/lib'` is user relative. This should be replaces with the RPM install location when CSM determines where this libary is shipped. 

First you should say where the library is located. Which is what we did above in the first section.
```Python

import sys
sys.path.append('.')

#add the python library to the path 
sys.path.append('/u/nbuonar/repos/CAST/work/csm/lib')
```

Second, you should import the main CSM library `lib_csm_py`. We did this and then nicknamed it `csm` for ease of use later in our script. 

```Python
import lib_csm_py as csm
```

Then, you should import any appropriate sub libraries for the CSM APIs that you will be using. If you want workload manager APIs such as `csm_allocation_query` then import the workload manager library `lib_csm_wm_py`. If you want inventory APIs, such as `csm_node_attributes_update`, then import the inventory library `lib_csm_inv_py`. Look at CSM API documentation for a full list of all CSM API libraries. 

For my example, I have imported the inventory library and nicknamed it `inv` for ease of use later.
```Python
import lib_csm_inv_py as inv
```


## Connection to CSM

At this point, a Python script can connect to CSM the same way a user would connect to CSM in the C language. You must connect to CSM by running the CSM init function before calling any CSM APIs. This init function is located in the main CSM library we imported earlier. 

In Python, we do this below:
```Python
csm.init_lib()
```

Just like in C, this function takes care of connecting to CSM.

## Accessing the CSM API

Below I have some code from an example script of setting a node to `IN_SERVICE` via `csm_node_attributes_update`

```Python
input = inv.node_attributes_update_input_t()
nodes=["allie","n01","bobby"]
input.set_node_names(nodes)
input.state = csm.csmi_node_state_t.CSM_NODE_IN_SERVICE

rc,handler,output = inv.node_attributes_update(input)

print rc 

if rc == csm.csmi_cmd_err_t.CSMERR_UPDATE_MISMATCH:
    print output.failure_count
    for i in range(0, output.failure_count):
        print output.get_failure_node_names(i)
```

Let's break down some important lines here for first time users of the CSM Python library. 

```Python
input = inv.node_attributes_update_input_t()
```

Here we are doing a few things. Just like in C, before we call the API we need to set up the input for the API. We do this on this line. Because this is an inventory API, we can find its input struct in the inventory library we imported earlier via `inv`, and we create this as `input`. 

We now fill `input`. 

## FAQ - Frequently Asked Questions
### How do I access and set arrays in the CSM Python library. 
When using the CSM Python library arrays must be `set` and `get`. 
#### Get

Example: 
```Python
if(result.dimms_count > 0):
        print("  dimms:")
        for j in range (0, result.dimms_count):
            dimm = result.get_dimms(j)
            print("    - serial_number:     " + str(dimm.serial_number))    
```

Here let's assume that the dimms_count is > 0. Let's say 3. The code will loop through each dimm, printing its serial number. The important line here is: `dimm = result.get_dimms(j)` here we are accessing an array. 

Arrays in the CSM Python library must be accessed using this `get_` function. Following the pattern of `get_ARRAYNAME`. The array names and fields of a CSM struct are the same as the C versions. Please look at CSM API documentation for a list of your struct and struct field names. 

So in our example here, our struct has an array named `dimms`. To access it, we must call `get_dimms(j)`. `j` here represents the element we want to access. `dimm` represents how we will store this element. 

Once stored, `dimm` can be accessed like any other struct. `print("    - serial_number:     " + str(dimm.serial_number))   ` 

#### Set


Example: 
```Python
input = inv.node_attributes_update_input_t()
nodes=["node_01","node_02","node_03"]
input.set_node_names(nodes)
input.state = csm.csmi_node_state_t.CSM_NODE_IN_SERVICE
```

Here we want to use `csm_node_attributes_update` to set a few nodes to `IN_SERVICE`. The API's input takes in a list of nodes. So in Python we will need to set this array of node names. The important line here is: `input.set_node_names(nodes)` here we are setting the array of the struct to an array we previously created. 

Before we can call `set_node_names(nodes)` we need to populate `nodes`.  

```Python
nodes=["node_01","node_02","node_03"]
```

Once `nodes` has been defined, we can call: `set_node_names(nodes)`. 

Arrays in the CSM Python library must be set using this `set_` function. Following the pattern of `set_ARRAYNAME`. The `set_` function requires a single parameter of a populated array. (Here that is `nodes`.) The array names and fields of a CSM struct are the same as the C versions. Please look at CSM API documentation for a list of your struct and struct field names. 

So in our example here, our struct has an array named `node_names`. To set it, we must call `input.set_node_names(nodes)`. `nodes` here represents the Python array we already created in the previous line. `input` represents the parent struct that contains this array. 
