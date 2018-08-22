from datetime import datetime

class Log:
	def __init__(self, line):
		# line1 is the metadata: [COMPUTE]2018-05-30 17:26:40.718991     csmapi::info
		# line2 is the data    : [365788516]; csm_allocation_delete start
		self.line = line
		self.line1 = line.split('|')[0]
		self.line2 = line.split('|')[1]
		self.handle_meta_data(self.line1)
		self.handle_data(self.line2)
		
	def handle_meta_data(self, line):
		line = line.split()
		if '[' in line[0]:
			self.Type = line[0].split(']')[0] + ']'
			self.Date = line[0].split(']')[1]
		else:
			self.Type = '[MASTER]'
			self.Date = line[0]
		self.Time = line[1]
		self.DateTime = datetime.strptime(self.Date+' ' +self.Time, '%Y-%m-%d %H:%M:%S.%f')
		self.Csm = line[2].split('::')
		
	def handle_data(self, line):
		self.Data = map(str.strip,(line.split('; '))) 

	def printing(self):
		print 'Type: 	 ' + self.Type
		print 'Date:     ' + self.Date
		print 'Time:     ' + self.Time
		print 'DateTime: ' + str(self.DateTime)
		print 'Csm:  	 ' + str(self.Csm)
		print 'Data: 	 ' + str(self.Data)
		print ''

# compute = '[COMPUTE]2018-05-30 17:26:40.718991     csmapi::info     | [365788516]; csm_allocation_delete start'
# master = '2018-05-30 16:32:55.067376     csmapi::info     | CSM_CMD_node_resources_query_all[2102082113]; Client Sent; PID: 29752; UID:0; GID:0'
# aggregate = '[AGG]2018-05-30 16:25:20.611547       csmd::info     | DAEMONSTATE: Compute node 10.7.8.9:59322 switched to PRIMARY connection.'
# util = '[UTL]2018-05-31 01:25:27.669148     csmapi::info     | CSM_CMD_node_resources_query_all[645948134]; Client Sent; PID: 49482; UID:0; GID:0'

# o1 = Log(compute)
# o2 = Log(master)
# o3 = Log(aggregate)
# o4 = Log(util)

# o1.printing()
# o2.printing()
# o3.printing()
# o4.printing()
