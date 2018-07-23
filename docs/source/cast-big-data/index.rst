Big Data Store
==============

CAST supports the integration of the ELK stack as a Big Data solution. Support for this solution
is bundled in the `csm-big-data` rpm in the form of suggested configurations and support scripts.

Configuration order is not strictly enforced for the ELK stack, however, this resource generally
assumes the components of the stack are installed in the following order:

    #. Logstash
    #. Elasticsearch
    #. Kibana 

.. toctree::
    :maxdepth: 2

    logstash.rst 
    elasticsearch.rst
    kibana.rst
    beats.rst
    data-aggregators.rst
    python-guide.rst
    csm-event-correlator.rst
    timeline.rst


