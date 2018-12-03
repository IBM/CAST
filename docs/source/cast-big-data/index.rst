Big Data Store
==============

CAST supports the integration of the ELK stack as a Big Data solution. Support for this solution
is bundled in the `csm-big-data` rpm in the form of suggested configurations and support scripts.

Configuration order is not strictly enforced for the ELK stack, however, this resource generally
assumes the components of the stack are installed in the following order:

    #. Elasticsearch
    #. Kibana 
    #. Logstash

This installation order minimizes the likelihood of improperly ingested data being stored in 
elasticsearch.

.. warning:: If the index mappings are not created properly timestamp data may be improperly
    stored. If this occurs the user will need to reindex the data to fix the problem. Please
    read the elasticsearch section carefully before ingesting data.


.. toctree::
    :maxdepth: 2

    elasticsearch.rst
    kibana.rst
    logstash.rst 
    data-aggregators.rst
    beats.rst
    python-guide.rst
    csm-event-correlator.rst
    cast-search.rst


