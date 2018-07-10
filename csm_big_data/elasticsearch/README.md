# Quick Setup #

CAST provides a script for creating the index templates supported by CAST. The `createIndices.sh`
script is a basic script which performs CURLs against the local elasticsearch REST API. A remote
elasticsearch node may be specified as a parameter to the script.

The current iteration of the script has a fixed port number and only performs a PUT (No delete operation at this time).

Usage on an elasticsearch node:

``` bash

    $ ./createIndices.sh
```

Please note the hostname used for the elasticsearch node is the value stored in `$HOSTNAME`.

This script will `PUT` a collection of index templates from the contents of `templates` to the 
elasticsearch service. The status of this operation and the name of the index will be printed
when the script is run, 200 is a success, anything else is an error.

Documentation for these indices may be viewed on the 
[read-the-docs](cast.rtfd.io/en/latest/cast-big-data/elasticsearch.html#indices).
