#ifndef __MIPS_CORE_HPP__
#define __MIPS_CORE_HPP__

#include <sst/core/component.h>
#include <sst/core/sst_types.h>
#include <sst/core/output.h>
#include <sst/core/interfaces/simpleMem.h>
#include <sst_core/sst_memory.hpp>
#include <queue>

struct instruction {
	char hex[5];
	int state; // 0 is Issue, 1 is Read Operands, 2 is Execute, 3 is Write Register/Broadcast
	uint16_t mem_address; // used by Issue only for lw/sw
	int load_store;
	int rs1; // set in Issue
	int rs2; // set in Issue
	int rdest; // set in Issue
	char src1[5]; // set to an FU by reservation station in Read Operands
	char src2[5]; // set to an FU by reservation station in Read Operands
	int src1_available; // used by reservation station
	int src2_available; // used by reservation station
	int src1_renamed;
	int src2_renamed;
	int src1_read;
	int src2_read;
	int cycles; // only used by FUs in Execute
	int cycle_issued;
};

struct reservation_station {
	char op;
	struct instruction inst;
	char id[5];
	int busy;
	int got_fu;
	int first_in_queue;
};

struct functional_unit {
	char op;
	int latency;
	struct instruction inst;
	int id;
	char broadcast_id[5];
	int busy;
	int num_executed;
	int ready_to_broadcast;
	struct reservation_station * res;
};

namespace XSim
{
namespace SST
{

class core :
		public ::SST::Component
{
	private:
		using Super=::SST::Component;
		using Cycle_t=::SST::Cycle_t;
		using SimpleMem=::SST::Interfaces::SimpleMem;
		using Request=SimpleMem::Request;
		using Output=::SST::Output;
		using ComponentId_t=::SST::ComponentId_t;
		using Params=::SST::Params;
		using Clock=::SST::Clock;

	public:
		core(ComponentId_t id, Params& params);
		virtual void init(unsigned int phase) override final;
		virtual void setup() override final;
		virtual void finish() override final;
	private:
		core();
		core(const core& params);
		core operator=(const core& params);
		virtual ~core() = default;
		void parseProgram(std::string program);
		int issue(std::string inst);
		void readOperands(struct reservation_station * res);
		void execute(struct functional_unit * fu);
		void writeRegister(struct functional_unit * fu);
		void outputStats();
		std::string stringifyFu(struct functional_unit * fu);

	protected:
		bool tick(Cycle_t cycle);
		void memory_callback(Request *ev);

	private:
		// The core clock frequency
		std::string clock_frequency="0Hz";
		// Prints verbose information
		int verbose=0;
		// Link to SST output
		Output *output=nullptr;
		// Link to SST SimpleMem
		SimpleMem *data_memory_link=nullptr;
		// My wrapper to simplify memory access with SST's SimpleMem
		SSTMemory *memory_latency=nullptr;

		std::vector<std::string> program;
		std::queue<uint16_t> memAccesses;

		bool mem_pending = false;

		std::array<int16_t, 8> regFile;
		std::array<uint16_t,65536> memory;

		int int_latency = 1;
		int div_latency = 1;
		int mult_latency = 1;
		int ls_latency = 1;

		int int_number = 1;
		int div_number = 1;
		int mult_number = 1;
		int ls_number = 1;

		int int_resnumber = 1;
		int div_resnumber = 1;
		int mult_resnumber = 1;
		int ls_resnumber = 1;

};

}
}

#endif
