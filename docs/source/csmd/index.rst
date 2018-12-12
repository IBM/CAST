CSM Infrastructure
==================

The managing process of CSM. The infrastructure facilitates the interaction of the local CSM APIs
and the CSM Database and cluster Compute nodes.

A Broad general visualization of the infrastructure has been reproduced below:

.. graphviz::
    digraph G {
        User -> Utility;
        Utility -> Master;

        Master -> Utility;
        Master -> Master;

        Master -> "CSM Database";
        "CSM Database" -> Master

        Master -> Aggregator;
        Aggregator -> Master;

        Aggregator -> Compute;
        Compute -> Aggregator;

        User [shape=Mdiamond];
        "CSM Database" [shape=cylinder];
    }




.. toctree::
    :maxdepth: 2

    csm_daemon.rst
    csmd_config.rst
