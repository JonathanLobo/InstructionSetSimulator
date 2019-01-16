#########################################
#		Inputs to the simulation		#
#########################################
import argparse
parser = argparse.ArgumentParser(description='Configuration options for this simulation.')

parser.add_argument('--program',
                    dest="program",
                    required=True,
                    action='store',
                    help='The program trace to run in the simulator')

parser.add_argument('--tomconfig',
                    dest="tomconfig",
                    required=True,
                    action='store',
                    help='The JSON file for configuring the simulator')

parser.add_argument('--output',
                    dest="output",
                    required=True,
                    action='store',
                    help='The JSON file to output stats to')

# Parse arguments
args = parser.parse_args()


## Print info ##
print("Running simulator using program " + args.program)
print("Configuration in " + args.tomconfig)


#####################################
#	Load JSON file with latencies	#
#####################################
import json
with open(args.tomconfig, 'r') as inp_file:
  sim_config=json.load(inp_file)

int_config = sim_config.get("integer")
div_config = sim_config.get("divider")
mult_config = sim_config.get("multiplier")
ls_config = sim_config.get("ls")
cache_config = sim_config.get("cache")
clock_speed = sim_config.get("clock")

print("Sim Config:")
print(json.dumps(sim_config, indent=2))


#####################################
#	SST stuff                     	#
#####################################
import sst

cache_link_latency = "100ps"

core = sst.Component("XSim_core","XSim.core")

core.addParams({
    "clock_frequency": clock_speed,
    "program": args.program,
    "output": args.output,
	"verbose": 0,
    "int_number": int_config.get("number"),
    "int_resnumber": int_config.get("resnumber"),
    "int_latency": int_config.get("latency"),
    "div_number": div_config.get("number"),
    "div_resnumber": div_config.get("resnumber"),
    "div_latency": div_config.get("latency"),
    "mult_number": mult_config.get("number"),
    "mult_resnumber": mult_config.get("resnumber"),
    "mult_latency": mult_config.get("latency"),
    "ls_number": ls_config.get("number"),
    "ls_resnumber": ls_config.get("resnumber"),
    "ls_latency": ls_config.get("latency")
})

memory = sst.Component("data_memory", "memHierarchy.MemController")
memory.addParams(
{
    'backend':				"memHierarchy.simpleMem",
	'backend.mem_size':		"10MiB",
	'clock':				"1GHz",
    'backend.access_time':  "100ns"
})

l1_cache = sst.Component("l1cache", "memHierarchy.Cache")
l1_cache.addParams({
    "associativity":        cache_config.get("associativity"),
    "cache_line_size":      16, # Same as block size
    "cache_size":           cache_config.get("size"),
    "cache_frequency":      clock_speed, # Same as cpu
    "access_latency_cycles":1,
    "L1":                   True
})

cpu_cache_link = sst.Link("cpu_cache_link")
cache_data_memory_link = sst.Link("cache_data_memory_link")

cpu_cache_link.connect(
(core, "data_memory_link", cache_link_latency),
(l1_cache, "high_network_0", cache_link_latency)
)

cache_data_memory_link.connect(
(l1_cache, "low_network_0", cache_link_latency),
(memory, "direct_link", cache_link_latency)
)

# cpu_data_memory_link = sst.Link("cpu_data_memory_link")
# cpu_data_memory_link.connect(
#     (core, "data_memory_link", cache_link_latency),
#     (memory, "direct_link", cache_link_latency)
# )

# Enable SST Statistics Outputs for this simulation
# statLevel = 16
# statFile = "stats.csv"
# sst.setStatisticLoadLevel(statLevel)
# sst.enableAllStatisticsForAllComponents({"type":"sst.AccumulatorStatistic"})
#
# sst.setStatisticOutput("sst.statOutputCSV")
# sst.setStatisticOutputOptions( {
#     "filepath"  : statFile,
# 	"separator" : ", "
# 	} )
