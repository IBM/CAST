## Use Case 1
**Script Name**: findJobTimeRange.py

**Description** : `A tool for finding when a job was running through the big data store.`

1. Queries `cast-allocation` for an `allocation id` or `primary job id`/ `secondary job id` combo
2. Display the start and end time of the job

**Arguments**:
```
-h, --help            show this help message and exit
  -a int, --allocationid int
                        The allocation ID of the job.
  -j int, --jobid int   The job ID of the job.
  -s int, --jobidsecondary int
                        The secondary job ID of the job (default : 0).
  -t hostname:port, --target hostname:port
                        An Elasticsearch server to be queried. This defaults
                        to the contents of environment variable
                        "CAST_ELASTIC".
  -H [host [host ...]], --hostnames [host [host ...]]
                        A list of hostnames to filter the results to          
```

## Use Case 2
**Script Name**: findJobKeys.py

**Description**: `A tool for finding keywords during the run time of a job.`

1. Queries `cast-allocation` for an `allocation id` or `primary job id` / `secondary job id` combo
2. Use the allocation to filter logs on `hostname` and `time range` and search for any messages matching a list of `keywords` (`*` is wild).
3. Display keyword statistics.

**Arguments**:
```
-h, --help            show this help message and exit
  -a int, --allocationid int
                        The allocation ID of the job.
  -j int, --jobid int   The job ID of the job.
  -s int, --jobidsecondary int
                        The secondary job ID of the job (default : 0).
  -t hostname:port, --target hostname:port
                        An Elasticsearch server to be queried. This defaults
                        to the contents of environment variable
                        "CAST_ELASTIC".
  -k [key [key ...]], --keywords [key [key ...]]
                        A list of keywords to search for in the Big Data Store
                        (default : *).
  -H [host [host ...]], --hostnames [host [host ...]]
                        A list of hostnames to filter the results to
```

## Use Case 3

**Description**: `A tool for finding metrics about the nodes participating in the supplied job id`

1. queries `cast-allocation` for an `allocation id` or `primary job id` / `secondary job id` combo.
2. Using the results of the previous query then query the environmental data nodes that participated in the job.
3. Display job run metrics

**Arguments**:
```
 -h, --help            show this help message and exit
  -a int, --allocationid int
                        The allocation ID of the job.
  -j int, --jobid int   The job ID of the job.
  -s int, --jobidsecondary int
                        The secondary job ID of the job (default : 0).
  -t hostname:port, --target hostname:port
                        An Elasticsearch server to be queried. This defaults
                        to the contents of environment variable
                        "CAST_ELASTIC".
  -H [host [host ...]], --hostnames [host [host ...]]
                        A list of hostnames to filter the results to
```

## Use Case 4

**Script Name**: findJobsRunning.py

**Description**: `A tool for finding jobs running at the specified time.`

1. Queries `cast-allocation` for any jobs that contained the specified time in their run time.
2. Display the `allocation id`, `job id`, and `secondary job id` of any running jobs.

**Arguments**:
```
-h, --help            show this help message and exit
  -t hostname:port, --target hostname:port
                        An Elasticsearch server to be queried. This defaults
                        to the contents of environment variable
                        "CAST_ELASTIC".
  -T timestamp, --time 
                         The time which should be searched for running jobs.
  -d days, --days 
                         The days before and after the timestamp to include in the range.    
  -hr hours, --hours 
                         The hours before and after the timestamp to include in the range.
  -s size, --size 
                         The number of results to be returned.                         
  -H [host [host ...]], --hostnames [host [host ...]]
                        A list of hostnames to filter the results to
```




