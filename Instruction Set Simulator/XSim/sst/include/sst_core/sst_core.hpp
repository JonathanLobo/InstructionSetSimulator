#ifndef __MIPS_CORE_HPP__
#define __MIPS_CORE_HPP__

#include <sst/core/component.h>
#include <sst/core/sst_types.h>
#include <sst/core/output.h>
#include <sst/core/interfaces/simpleMem.h>
#include <sst_core/sst_memory.hpp>

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
		void execute(std::string instruction);
		int getLatency(std::string instruction);
		void outputStats();

	protected:
		bool tick(Cycle_t cycle);
		void memory_callback(Request *ev);

	private:
		// The core clock frequency
		std::string clock_frequency="0Hz";
		// Number of read requests
		std::uint16_t n_read_requests=1;
		// Number of write requests
		std::uint16_t n_write_requests=1;
		// Prints verbose information
		int verbose=0;
		// Link to SST output
		Output *output=nullptr;
		// Link to SST SimpleMem
		SimpleMem *data_memory_link=nullptr;
		// My wrapper to simplify memory access with SST's SimpleMem
		SSTMemory *memory_latency=nullptr;
		// Execution done
		bool busy=false;

		std::vector<std::string> program;
		std::array<int16_t, 8> regFile;
		std::array<uint16_t,65536> memory;

		std::uint16_t mul_lat = 2;
		std::uint16_t add_lat = 2;
		std::uint16_t sub_lat = 2;
		std::uint16_t and_lat = 2;
		std::uint16_t nor_lat = 2;
		std::uint16_t div_lat = 2;
		std::uint16_t mod_lat = 2;
		std::uint16_t exp_lat = 2;
};

}
}

#endif
