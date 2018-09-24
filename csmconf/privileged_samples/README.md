# Privleged Prolog/Epilog Samples #

Contained within this directory is an assortment of sample scripts for use with CSM
epilog and prolog functionality. Each script has a brief comment outlining the behavior
and usage of the script, please consult the script source for more details. 


## Prolog Scripts ##


## Epilog Scripts ##

* `epilog_process_cleanup.sh`
  * Removes any processes associated with the user id in the `CSM_USER_NAME` environment variable.


## Calling bash scripts from Python ##

CSM recommends using either the `os` or `subprocess` module to invoke bash scripts.


### os ###

The `os` module is easy to invoke, however it offers limited control of the output
of the bash script. For more details, please consult the official `os` module documentation.

```python
import os

os.system(<script-name>)
```


### subprocess ###

The `subprocess` module is somewhat more complicated than the `os` module, however, CSM
recommends it due to its access to script output and error codes. For more details,
please consult the official `subprocess` module documentation.

```python
import subprocess

process = subprocess.Popen(['<script-name>'], stdout=subprocess.PIPE)
out, err = process.communicate()
```
