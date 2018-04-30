HAProxy Configuration
*********************

.. note:: In the following documentation, examples requiring replacement will be annotated with the bash style 
    `${variable_name}` and followed by an explanation of the variable.


Quick Configuration
===================




Configuration
=============

The supplied HAProxy_Logstash.conf file lays out the framework for how to set up the HAProxy for
Logstash. 

.. Note:: As Logstash only accepts tcp requests only the default, global and listen sections
   have been implemented. A complete proxy should be sufficient for the tcp connections.
   To increase the number of things this proxy is capable of doing, the proxy may be split.


Running HAProxy
===============

After installing HAProxy and configuring the config file to your specification start
the HAProxy Daemon as follows:

.. code-block:: bash

    # Clear any old instances of haproxy, collisions might cause issues.
    ps -ef | awk '/haproxy/{print $2}' |  xargs kill -9

    # If your config file name differs be sure to change it!
    haproxy -f haproxy_logstash.conf



Scratch Notes
=============
HAProxy (High Availability Proxy).

Load Balancing

ACL (Access Control List)

http://cbonte.github.io/haproxy-dconv/configuration-1.4.html#7
https://www.digitalocean.com/community/tutorials/an-introduction-to-haproxy-and-load-balancing-concepts



Backends are the set of servers that receive forwarded requests.
which load balancing algorithm
list of servers and ports.


Frontends define how the requests should be forwareded to backends.

Layer 4 - Based on frontend ip. (might be what's needed for the LA) mode tcp

Layer 7 - Based on Application access (e.g. / vs /blog) mode http


leastconn might be the best, but due tothe short bursts of the connection it might not
be any better than round robin.

sticky sessions for more when a static connection is needed.

High Availablility 

2 Load Balancers, if one goes down switch to the floating one.

http://www.haproxy.org/download/1.4/doc/configuration.txt


http://cbonte.github.io/haproxy-dconv/1.7/intro.html


rsyslog batches logging by default.

