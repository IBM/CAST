CAST Documentation
==================

The following documentation outlines the process for contributing and building the CAST documentation.

Building the Documentation
--------------------------

CAST uses `Read the Docs`_ to distribute documentation, however for development builds the following
process should be followed to to build the documentation for CAST.

1. Install Sphinx with the ReadTheDocs theme:

.. code-block:: bash

    pip install sphinx sphinx_rtd_theme recommonmark

2. Build the documentation:

.. code-block:: bash

     cd <CAST-Dir>/docs
     make html

A copy of the read the docs documentation should now have been built and placed in the
`./build/html` directory.

.. warning:: If there are any warnings in the make, please do not push your changes!


Adding Documentation
--------------------

CAST documentation is written using the RST (`reStructuredText`_) markup syntax. RST was selcted
in favor of Markdown for in spec support of tables and more rich link markup syntax. Documentation
writers are *strongly* encouraged to review the `RST Reference`_ before contributing as RST syntax,
even if they are familiar with other markup languages.

CAST uses a set of flat category directories in the `./source` directory to organize documentation.
Each of these category directories should have an `index.rst` file. This index file should 
include a brief description of the category and a `toctree` referencing other documents in the 
directory. This `toctree` will define the ordering of topics for the category. A sample is provided
below:

.. code-block:: none

    .. toctree::
       :maxdepth: 2
    
       logstash.rst
       elasticsearch.rst
       kibana.rst


When adding a new category the `index.rst` of the category must be added to the root `index.rst`
toctree.

All documentation changes must be submitted through pull requests as once documents are in the 
correct branch they will be instantly made public in the CAST documentation.

Please run the `make html` command described in `Building the Documentation`_, do not commit a
documentation change that produces any warnings or errors. Additionally, a visual examination of 
the built HTML should be performed to verify that no malformatted content is released to the 
public.


Style
-----

Documentation should be free from spelling errors and written professionally. Please no cursing,
colloquialisms or memes. All documents should be written in the third person and perfer generic
agendered pronouns for describing the user (*your*, *the user*, etc.). 



.. Links
.. _Read the Docs: https://cast.readthedocs.org
.. _reStructuredText: http://docutils.sourceforge.net/rst.html
.. _RST Reference: http://docutils.sourceforge.net/docs/user/rst/quickref.html
