.. _CASTSearch:

Cast Search
===========

The cast search mechanism is a GUI utility for searching for allocations in the Big Data Store.

.. contents::
    :local:

Installation
------------

Installation of the `CAST Search` plugin is performed through the |csm-kibana| rpm:

.. code:: bash

   rpm -ivh ibm-csm-bds-kibana-*.noarch.rpm

.. attention:: Kibana must be installed first please refer to the :ref: `cast-kibana` documentation.


Configuration
-------------

After installing the plugin the following steps must be taken to begin using the plugin.


1. Select the `Management` tab from the sidebar:

.. image:: https://user-images.githubusercontent.com/1195452/49386058-bc7fb180-f6ec-11e8-98c2-0621c1212c47.png
    :alt: Step One
    :height: 200px

2. Select the Kibana `Index Patterns`:

.. image:: https://user-images.githubusercontent.com/1195452/49386060-bc7fb180-f6ec-11e8-89e9-993e1541e2e9.png
    :alt: Step Two
    :height: 200px

3. If the `cast-allocation` index pattern is not present create a new index pattern.
If the index pattern is present skip to `Step Seven`_:

.. image:: https://user-images.githubusercontent.com/1195452/49386061-bd184800-f6ec-11e8-8f65-7eb27d8d0f2e.png
    :alt: Step Three
    :height: 200px

4. Input `cast-allocation` in the index pattern name:

.. image:: https://user-images.githubusercontent.com/1195452/49386062-bd184800-f6ec-11e8-9790-b79139feec30.png
    :alt: Step Four
    :height: 200px

5. Select `@timestamp` for the time filter (this will sort records by update date by default:

.. image:: https://user-images.githubusercontent.com/1195452/49386063-bd184800-f6ec-11e8-84f0-19e7367b7621.png
    :alt: Step Five
    :height: 200px


6. Verify that the `cast-allocation` index pattern is now present:

.. image:: https://user-images.githubusercontent.com/1195452/49386064-bd184800-f6ec-11e8-96d5-e5ec996b1dc0.png
    :alt: Step Six
    :height: 200px

.. _Step Seven:

7. Select the `Visualize` sidebar tab, then select the `CAST Search` option:

.. image:: https://user-images.githubusercontent.com/1195452/49386065-bd184800-f6ec-11e8-9be6-7ea71c1879ce.png
    :alt: Step Seven
    :height: 400px

8. Select the add option, by default this will select the `Allocation ID` option. 
If the user wishes to search on Job IDs, select `Job ID` in the dropdown.

.. image:: https://user-images.githubusercontent.com/1195452/49386066-bd184800-f6ec-11e8-967b-ab279d2fb399.png
    :alt: Step Eight
    :height: 400px

9. A listing of fields should now be visible. Select the `Apply changes` button before saving the visualization:

.. image:: https://user-images.githubusercontent.com/1195452/49386067-bd184800-f6ec-11e8-9550-2dd548576845.png
    :alt: Step Nine
    :height: 400px

.. _Step Ten:

10. Save the Visualization so the plugin may be used from a dashboard:

.. image:: https://user-images.githubusercontent.com/1195452/49386068-bd184800-f6ec-11e8-9d5a-e219a97f99b9.png
    :alt: Step Ten
    :height: 400px

11. Select the Dashboard sidebar tab, then create a new dashboard:

.. image:: https://user-images.githubusercontent.com/1195452/49386070-bdb0de80-f6ec-11e8-87e1-82f1a1af10f4.png
    :alt: Step Eleven
    :height: 200px

12. Select the `Add` option, then select the visualizer created in `Step Ten`_ and `Add new Visualization`:

.. image:: https://user-images.githubusercontent.com/1195452/49386071-bdb0de80-f6ec-11e8-9f56-6631c42775be.png
    :alt: Step Twelve
    :height: 200px

13. The plugin should now be usable:

.. image:: https://user-images.githubusercontent.com/1195452/49386072-bdb0de80-f6ec-11e8-87c2-6aab563eb5de.png
    :alt: Step Thirteen
    :height: 200px


