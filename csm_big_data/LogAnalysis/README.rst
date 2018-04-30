.. _log-analysis: 

Log Analysis
************

The interface to the Big Data Store, accessible through both RESTful APIs and a web 
interface. 

.. note:: In the following documentation, examples requiring replacement will be annotated with the bash style 
    `${variable_name}` and followed by an explanation of the variable.

**Contents:**

.. contents::
   :local:

Quick Configuration
-------------------
:Configuration Script:
    `/opt/ibm/csm/bigdata/LogAnalysis/quick_config.sh`

To quickly deploy the default configuration supplied in this sample, run
`/opt/ibm/csm/bigdata/LogAnalysis/quick_config.sh`

To install the defaults by hand, please refer to the steps below:

.. warning:: These steps must be done as the ioala user!

1. Copy `/opt/ibm/csm/bigdata/LogAnalysis/DSV_props` to 
    `${LogAnalysis Home}/unity_content/DSVToolkit*`

.. note:: In each DSV_props file ${LogAnalysis Home} is set to 
    `/opt/IBM/ioala/LogAnalysis` for the `scalaHome` value.

2. Execute the following to generate the DSV:

    .. code-block:: bash

        cd ${LogAnalysis Home}/unity_content/DSVToolkit*/

        # For each props file run the following.
        python dsvGen.py ${props_file} -d -f -o -u ${USER} -p ${PASS}

.. note:: ${USER} and ${PASS} are the username and password of the Log Analysis
   user with Read/Write Access.

.. note:: The python script must be run for each property copied.

3. Add the data source through the GUI:
    
    a. In a browser navigate to `https://\[server ip]:9987/Unity/login.jsp`
        
        * Default username/password is unityadmin/unityadmin
        * This is the default port configuration.

    b. Select the Create Data Source option.
    
    c. Input the **Host Name**, this is the name of the props file for the insight pack
        being configured without the **.props** extensions (ex. syslog.props becomes syslog)
    
    d. Input the data paths:
        :File Path: Indentical to the Host Name in the previous step (syslog).
        :Type: Select the generated Insight Pack DSV which is visible in the drop down (syslog_DSV).
        :Collection: Only one option should be selectable (syslog_DSV-Collection).
    
    e. The **Name** should match the **File Path** and **Host Name**.
    
    f. Apply steps a-e for each insight pack configured in steps 1-2.

.. note:: quick_config.sh will execute steps 1 and 2 of this quick configuration.

.. note:: This documentation assumes that Log Analysis has been installed.


Properties
----------

:Sample Directory: 
    | /opt/ibm/csm/bigdata/LogAnalysis/DSV_props/

:Destination Directory:
    | ${LogAnalysis Home}/unity_content/DSVToolkit_v1.1.0.3/


The DSV_props director supplied with this document generate insight packs that 
allow the output from Logstash to be input to the Big Data Store. The 
most noteworthy options in these config files are noted below:

:SCALA_server:
    
    :scalaHome: Set to `/opt/IBM/ioala/LogAnalysis` by default, 
        this is the home directory of LogAnalysis.

:DSV_file:
    
    :moduleName: The name of the Data Separated Value module to be generated. 
        This field should follow this format: `{logsource name}_DSV`

:field${Number}_indexConfig:
    
    :name: The name of the field being configured.
    
    :dataType: The type of the data stored in the field.

    :filterable: Whether the field can be indext for filtering, for 
        irregular/greedy data dumps it is recommended that this field be set to 
        false.

.. note:: For a more comprehensive breakdown of parameters, please consult the 
    `IBM Operations Analytics - Log Analysis : Installation, Configuration, and Administration Guide`

.. warning:: If `scalaHome` does not match the Log Analysis Home directory 
    consequences may arise.



