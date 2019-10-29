# Tools for Processing bbServer Console Logs and Stagein and Stageout Logs

## Organization of Logs to be Processed

The easiest way to have multiple console logs from multiple bbServers analyzed is
to put them in a directory structure like:
    2019-10-16              <- Root directory that is passed as *arg1* below.
                               It's name does not affect the processing of the logs.
      +---------- *Repeated for each bbServer*
      | bbServerN           <- Subdirectory for all console logs for that bbServerN.
      |                        By default, it's name will be used to identify that
      |                        bbServer in the analysis.
      |     oldlogs         <- Archieved logs for that bbServer (similar to how they
      |                        are archieved in /var/log on each server).
      +----------

Any file matching the name pattern given for *arg2* below will be processed as a
bbServer console log, with the exception for files with 'stagein' or 'stageout' in
their names.  Those files will be processsed as stagein and stageout logs
respectively.

The stagein and stageout logs can be located anywhere within the directory structure.
It is most natural for them to be located under the root directory.

The flightlogs can be located for each bbSServer under bbServerN.  (This is where they
are stored in /var/log on each server).  Current processing does not analyze them
and they will be skipped over given an appropriate filter as given by *arg2* below.

If this directory structure is not used, then *arg3* below can be specified to indicate
the name of the bbServer.  All found console logs will then be associated with that
one named bbServer.

## How to Process Logs

Using the scripts in CAST/bb/tools/scripts, issue:
    python processConsoleLogs.py *arg1* *arg2* *arg3*
        where arg1 is the path to the root directory where the console logs reside.
                   (default is .)
          and arg2 is a regex giving a pattern used to recursively search for console
                   logs to process starting with the path given by arg1.
                   (default is .*) (e.g., console_20191016.*)
          and arg3 which is optional.  If specified, it gives the name of a bbServer.
                   All found console logs will be associated with that single bbServer.

This will create a subdirectory under the path given by arg1 named /Analysis.
Within that subdirectory, a .pickle file will be generated giving an intermediate
representation of the information contained within the console, stagein, and
stageout logs.


## How to Generate Output/Reports

Using the scripts in CAST/bb/tools/scripts, issue:
    python performAllAnalysis.py *arg1*
        where arg1 is the path to the root directory of where the console logs reside.
                   (default is .)

This will create text and json files within the /Analysis subdirectory giving
overall analysis of the data found within the logs and generate an additional
subdirectory under /Analysis for each jobid found in the logs.  Each of those
jobid subdirectories have additional files generated within for details related
to that specific jobid.

While the root directory to the logs is passed, only information contained
within the .pickle file is used by this script, and any other script, used
to generate output/reports.

## Additional Information

Any of the scripts in CAST/bb/tools/scripts can be run in a similar fashion
as the instructions given under How to Generate Output/Reports with a single
argument giving the path to the root directory.  Running any of the other
scripts will generate a subset of the analysis results.  However, the easiest
approach is to simply run performAllAnalysis.py which simply runs them all.
