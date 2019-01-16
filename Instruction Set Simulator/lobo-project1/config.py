#########################################
#		Inputs to the simulation		#
#########################################
import argparse
parser = argparse.ArgumentParser(description='Configuration options for this simulation.')

parser.add_argument('--program',
                    dest="program",
                    required=True,
                    action='store',
                    help='The program to run in the simulator')

parser.add_argument('--latencies',
                    dest="latencies",
                    required=True,
                    action='store',
                    help='The json file with the instruction latencies')
# Parse arguments
args = parser.parse_args()


## Print info ##
print("Running simulator using program "+args.program)
print("Latencies in "+args.latencies)



#####################################
#	Load JSON file with latencies	#
#####################################
import json
with open(args.latencies, 'r') as inp_file:
  latencies=json.load(inp_file)
print("Latencies:")
print(json.dumps(latencies, indent=2))



#####################################
#	SST stuff                     	#
#####################################
import sst

cache_link_latency = "100ps"

core = sst.Component("XSim_core","XSim.core")

core.addParams({
    "clock_frequency": "1GHz",
    "program": args.program,
	"verbose": 0

})

core.addParams(latencies)

memory = sst.Component("data_memory", "memHierarchy.MemController")
memory.addParams(
{
    'backend':				"memHierarchy.simpleMem",
	'backend.mem_size':		"10MiB",
	'clock':				"1GHz",
    'backend.access_time':  "100ns"
})


cpu_data_memory_link = sst.Link("cpu_data_memory_link")
cpu_data_memory_link.connect(
    (core, "data_memory_link", cache_link_latency),
    (memory, "direct_link", cache_link_latency)
)


# Enable SST Statistics Outputs for this simulation
statLevel = 16
statFile = "stats.csv"
sst.setStatisticLoadLevel(statLevel)
sst.enableAllStatisticsForAllComponents({"type":"sst.AccumulatorStatistic"})

sst.setStatisticOutput("sst.statOutputCSV")
sst.setStatisticOutputOptions( {
    "filepath"  : statFile,
	"separator" : ", "
	} )
