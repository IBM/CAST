# Obtaining Log Statistics

This python script parses log files to calculate the start and end time of API calls on the different types of nodes that generate these logs. From the start and end time, the script calculates:
- `frequency` at which the API was called
- `mean` run time
- `median` run time
- `minimum` run time
- `maximum` run time
- `standard deviation` run time
The script also captures job ID collisions when start and end API's do not match

## Setup
This script handles `Master`, `Computer`, `Utility`, and, `Aggregator` logs. These must be placed under the `csm_log_utility/Logs` directory unders their respective types. 

**Running the script**
There are three ways of running the logs with time formats:
```
Format: <Start Date> <Start Time>
Format: YYYY-MM-DD HH:MM::SS
```
```python
#1. Run through the logs in its entirety:
python API_Statistics.py

#2. Run through the logs with a specific start time:
python API_Statistics.py <Start Date> <Start Time> 

#3. Run through the logs with a specific start and end time:
python API_Statistics.py <Start Date> <Start Time> <End Date> <End Time>
```

## Output
The script will output to the screen as well as into separate Reports under `csm_log_utility/Reports` under their respective log types.The report includes errors and calculated statistics.
