import sst

cache_link_latency = "300ps"

core = sst.Component("XSim_core","XSim.core")

core.addParams({
    "clock_frequency": "1MHz",
	"n_read_requests": 5,
	"verbose": 0

})

memory = sst.Component("data_memory", "memHierarchy.MemController")
memory.addParams(
{
    'backend':				"memHierarchy.simpleMem",
	'backend.mem_size':		"10MiB",
	'clock':				"200KHz",
    'backend.access_time':  "100ns"
})


cpu_data_memory_link = sst.Link("cpu_data_memory_link")
cpu_data_memory_link.connect((core, "data_memory_link", cache_link_latency), (memory, "direct_link", cache_link_latency))


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
