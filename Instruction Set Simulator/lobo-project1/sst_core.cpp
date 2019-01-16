#include <sst/core/sst_config.h>
#include <sst_core/sst_core.hpp>
#include <sstream>
#include <sst/core/interfaces/simpleMem.h>
#include <sst/core/interfaces/stringEvent.h>
#include <sst/core/simulation.h>
#include <cstdlib>
#include <fstream>
#include <bitset>
#include <cmath>

#define print_request(req_to_print) \
{\
	std::cout<<\
	(req_to_print->cmd==Interfaces::SimpleMem::Request::Command::Write?"Write":\
	(req_to_print->cmd==Interfaces::SimpleMem::Request::Command::Read?"Read":\
	(req_to_print->cmd==Interfaces::SimpleMem::Request::Command::ReadResp?"ReadResp":\
	(req_to_print->cmd==Interfaces::SimpleMem::Request::Command::WriteResp?"WriteResp":"Some flush"\
	))))<<std::endl<<\
	"\tTo Address "<<req_to_print->addr<<std::endl<<\
	"\tSize "<<req_to_print->size<<std::endl<<\
	"\tId "<<req_to_print->id<<std::endl;\
	}

namespace XSim
{
namespace SST
{

std::string instruction = "";
uint16_t pc = 0;
int lat_counter = 0;
int state = 0; //state 0 is fetch, state 1 is execute
int pc_changed = 0; //1 if PC was changed, 0 otherwise
int halt = 0; //set to 1 by a halt instruction
int num_instructions = 0;
int num_cycles = 0;
int num_add = 0, num_sub = 0, num_and = 0, num_nor = 0, num_div = 0, num_mul = 0;
int num_mod = 0, num_exp = 0, num_lw = 0, num_sw = 0, num_liz = 0, num_lis = 0;
int num_lui = 0, num_bp = 0, num_bn = 0, num_bx = 0, num_bz = 0, num_jr = 0;
int num_jalr = 0, num_j = 0, num_halt = 0, num_put = 0;

core::core(ComponentId_t id, Params& params):
	Component(id)
{
	//***********************************************************************************************
	//*	Some of the parameters may not be available here, e.g. if --run-mode is "init"				*
	//*		As a consequence, this section shall not throw any errors. Instead use setup section	*
	//***********************************************************************************************

	// verbose is one of the configuration options for this component
	verbose = (uint32_t) params.find<uint32_t>("verbose", verbose);
	// clock_frequency is one of the configuration options for this component
	clock_frequency=params.find<std::string>("clock_frequency",clock_frequency);

	std::string progPath = "";
	progPath = params.find<std::string>("program", progPath);
	parseProgram(progPath);

	// initialize all registers to 0
	regFile.fill(0);

	// read in latencies
	add_lat = params.find<uarch_t>("add",add_lat);
	sub_lat = params.find<uarch_t>("sub",sub_lat);
	and_lat = params.find<uarch_t>("and",and_lat);
	nor_lat = params.find<uarch_t>("nor",nor_lat);
	div_lat = params.find<uarch_t>("div",div_lat);
	mul_lat = params.find<uarch_t>("mul",mul_lat);
	mod_lat = params.find<uarch_t>("mod",mod_lat);
	exp_lat = params.find<uarch_t>("exp",exp_lat);


	// Create the SST output with the required verbosity level
	output = new Output("mips_core[@t:@l]: ", verbose, 0, Output::STDOUT);

	bool success;
	output->verbose(CALL_INFO, 1, 0, "Configuring cache connection...\n");

	//
	// The following code creates the SST interface that will connect to the memory hierarchy
	//
	data_memory_link = dynamic_cast<SimpleMem*>(Super::loadModuleWithComponent("memHierarchy.memInterface", this, params));
	SimpleMem::Handler<core> *data_handler=
			new SimpleMem::Handler<core>(this,&core::memory_callback);
	if(!data_memory_link)
	{
		output->fatal(CALL_INFO, -1, "Error loading memory interface module.\n");
	}
	else
	{
		output->verbose(CALL_INFO, 1, 0, "Loaded memory interface successfully.\n");
	}
	success=data_memory_link->initialize("data_memory_link", data_handler );
	if(success)
	{
		output->verbose(CALL_INFO, 1, 0, "Loaded memory initialize routine returned successfully.\n");
	}
	else
	{
		output->fatal(CALL_INFO, -1, "Failed to initialize interface: %s\n", "memHierarchy.memInterface");
	}
	output->verbose(CALL_INFO, 1, 0, "Configuration of memory interface completed.\n");

	// tell the simulator not to end without us
	registerAsPrimaryComponent();
	primaryComponentDoNotEndSim();
}

void core::parseProgram(std::string path) {
	// std::cout << path << "\n";

	std::ifstream inputFile(path);
	std::string hex;
	uint16_t value;

	while (std::getline(inputFile, hex))
	{
    std::istringstream iss(hex);
    if (iss >> hex) {
			if (hex.substr(0,1) != "#") {
				value = strtoul(hex.substr(0,std::string::npos).c_str(), NULL, 16);
				program.push_back(std::bitset<16>(value).to_string());
			}
		}
	}

	std::cout << "Program Length: " << program.size() << "\n";

}

void core::execute(std::string instruction) {

	pc_changed = 0;

	std::string opcode = instruction.substr(0,5);

	char type;

	int rd, rs, rt;

	std::string imm;

	if (opcode.substr(0,1) == "0" || opcode == "10011") {
		type = 'R';
	} else if (opcode.substr(0,1) == "1" && opcode != "10011" && opcode != "11000") {
		type = 'I';
	} else {
		type = 'X';
	}

	if (type == 'R') {
		rd = std::stoi(instruction.substr(5, 3), nullptr, 2);
		rs = std::stoi(instruction.substr(8, 3), nullptr, 2);
		rt = std::stoi(instruction.substr(11, 3), nullptr, 2);

		// std::cout << "RD: " << rd << "\n";
		// std::cout << "RS: " << rs << "\n";
		// std::cout << "RT: " << rt << "\n";

	} else if (type == 'I') {
		rd = std::stoi(instruction.substr(5, 3), nullptr, 2);
		imm = instruction.substr(8, 8);
		// std::cout << "Imm: " << imm << "\n";
	} else {
		imm = instruction.substr(5, 11);
	}

	if (opcode == "00000") {
		//add
		regFile[rd] = regFile[rs] + regFile[rt];
		// std::cout << "r" << rd << " = " << "r" << rs << " + " << "r" << rt << "\n";
		// std::cout << "r" << rd << " value: " << regFile[rd] << "\n";
		num_add++;

	} else if (opcode == "00001") {
		//sub
		regFile[rd] = regFile[rs] - regFile[rt];
		// std::cout << "r" << rd << " = " << "r" << rs << " - " << "r" << rt << "\n";
		// std::cout << "r" << rd << " value: " << regFile[rd] << "\n";
		num_sub++;

	}  else if (opcode == "00010") {
		//and
		regFile[rd] = regFile[rs] & regFile[rt];
		num_and++;

	}  else if (opcode == "00011") {
		//nor
		regFile[rd] = ~(regFile[rs] | regFile[rt]);
		num_nor++;

	}  else if (opcode == "00100") {
		//div
		regFile[rd] = regFile[rs] / regFile[rt];
		num_div++;

	}  else if (opcode == "00101") {
		//mul
		regFile[rd] = regFile[rs] * regFile[rt];
		num_mul++;

	}  else if (opcode == "00110") {
		//mod
		regFile[rd] = regFile[rs] % regFile[rt];
		num_mod++;

	}  else if (opcode == "00111") {
		//exp
		regFile[rd] = std::pow(regFile[rs], regFile[rt]);
		num_exp++;

	}  else if (opcode == "01000") {
		//lw
		// Simulate memory latency
		uint16_t address = regFile[rs];
		std::function<void(uarch_t, uarch_t)> callback_function = [this](uarch_t request_id, uarch_t addr)
		{
			this->busy=false;
			std::cout<<"Read request "<<request_id<<": Address "<<addr<<std::endl;
		};
		memory_latency->read(address, callback_function);

		busy=true;

		regFile[rd] = memory[address];
		num_lw++;

	}  else if (opcode == "01001") {
		//sw
		// Simulate memory latency
		uint16_t address = regFile[rs];
		std::function<void(uarch_t, uarch_t)> callback_function = [this](uarch_t request_id, uarch_t addr)
		{
			this->busy=false;
			std::cout<<"Write request "<<request_id<<": Address "<<addr<<std::endl;
		};
		memory_latency->write(address, callback_function);

		busy=true;

		memory[address] = regFile[rt];
		num_sw++;

	}  else if (opcode == "10000") {
		//liz
		regFile[rd] = std::stoi(("00000000" + imm), nullptr, 2);
		num_liz++;

	}  else if (opcode == "10001") {
		//lis
		if (imm.substr(0, 1) == "1") {
			regFile[rd] = std::stoi(("11111111" + imm), nullptr, 2);
		} else {
			regFile[rd] = std::stoi(("00000000" + imm), nullptr, 2);
		}
		num_lis++;

	}  else if (opcode == "10010") {
		//lui
		std::string rdVal = std::bitset<16>(regFile[rd]).to_string();
		regFile[rd] = std::stoi(imm + rdVal.substr(8,8), nullptr, 2);
		num_lui++;

	}  else if (opcode == "10100") {
		//bp
		if (regFile[rd] > 0) {
			pc = std::stoi(("0000000" + imm.substr(0, 8) + "0"), nullptr, 2);
			pc_changed = 1;
		}
		num_bp++;

	}  else if (opcode == "10101") {
		//bn
		if (regFile[rd] < 0) {
			pc = std::stoi(("0000000" + imm.substr(0, 8) + "0"), nullptr, 2);
			pc_changed = 1;
		}
		num_bn++;

	}  else if (opcode == "10110") {
		//bx
		if (regFile[rd] != 0) {
			pc = std::stoi(("0000000" + imm.substr(0, 8) + "0"), nullptr, 2);
			pc_changed = 1;
		}
		num_bx++;

	}  else if (opcode == "10111") {
		//bz
		if (regFile[rd] == 0) {
			pc = std::stoi(("0000000" + imm.substr(0, 8) + "0"), nullptr, 2);
			pc_changed = 1;
		}
		num_bz++;

	}  else if (opcode == "01100") {
		//jr
		pc = regFile[rs] & 0xFFFE;
		pc_changed = 1;
		num_jr++;

	}  else if (opcode == "10011") {
		//jalr
		regFile[rd] = pc + 2;
		pc = regFile[rs] & 0xFFFE;
		pc_changed = 1;
		num_jalr++;

	}  else if (opcode == "11000") {
		//j
		std::string pc_bits = std::bitset<16>(regFile[rd]).to_string().substr(0, 4);
		pc = std::stoi((pc_bits + imm.substr(0, 11) + "0"), nullptr, 2);
		pc_changed = 1;
		num_j++;

	}  else if (opcode == "01101") {
		//halt
		halt = 1;
		num_halt++;

	}  else if (opcode == "01110") {
		//put
		std::cout << regFile[rs] << "\n";
		num_put++;

	}

	num_instructions++;

	// std::cout << type << "\n";

}

int core::getLatency(std::string instruction) {
	std::cout << instruction << "\n";
	std::string opcode = instruction.substr(0,5);

	int latency = 2;

	if (opcode == "00000") {
		latency = add_lat;
	} else if (opcode == "00001") {
		latency = sub_lat;
	} else if (opcode == "00010") {
		latency = and_lat;
	} else if (opcode == "00011") {
		latency = nor_lat;
	} else if (opcode == "00100") {
		latency = div_lat;
	} else if (opcode == "00101") {
		latency = mul_lat;
	} else if (opcode == "00110") {
		latency = mod_lat;
	} else if (opcode == "00111") {
		latency = exp_lat;
	}

	return latency;
}

void core::outputStats() {
	std::string registers =  std::string("{\"registers\":[\n\t{")
		+ "\"r0\":" + std::to_string(regFile[0]) + ", \"r1\":" + std::to_string(regFile[1]) + ", \"r2\":" + std::to_string(regFile[2]) + ", \"r3\":" + std::to_string(regFile[3])
		+ ",\n\t \"r4\":" + std::to_string(regFile[4]) + ", \"r5\":" + std::to_string(regFile[5]) + ", \"r6\":" + std::to_string(regFile[6]) + ", \"r7\":" + std::to_string(regFile[7])
		+ "\n\t}],";

	std::string stats = std::string("\n \"stats\":[\n\t{")
		+ "\"add\":" + std::to_string(num_add) + ",\t\"sub\":" + std::to_string(num_sub) + ",\t\"and\":" + std::to_string(num_and)
		+ ",\n\t \"nor\":" + std::to_string(num_nor) + ",\t\"div\":" + std::to_string(num_div) + ",\t\"mul\":" + std::to_string(num_mul)
		+ ",\n\t \"mod\":" + std::to_string(num_mod) + ",\t\"exp\":" + std::to_string(num_exp) + ",\t\"lw\":" + std::to_string(num_lw)
		+ ",\n\t \"sw\":" + std::to_string(num_sw) + ",\t\"liz\":" + std::to_string(num_liz) + ",\t\"lis\":" + std::to_string(num_lis)
		+ ",\n\t \"lui\":" + std::to_string(num_lui) + ",\t\"bp\":" + std::to_string(num_bp) + ",\t\t\"bn\":" + std::to_string(num_bn)
		+ ",\n\t \"bx\":" + std::to_string(num_bx) + ",\t\"bz\":" + std::to_string(num_bz) + ",\t\t\"jr\":" + std::to_string(num_jr)
		+ ",\n\t \"jalr\":" + std::to_string(num_jalr) + ",\t\"j\":" + std::to_string(num_j) + ",\t\t\"halt\":" + std::to_string(num_halt)
		+ ",\n\t \"put\":" + std::to_string(num_put)
		+ ",\n\t \"instructions\":" + std::to_string(num_instructions)
		+ ",\n\t \"cycles\":" + std::to_string(num_cycles)
		+ "\n\t}]\n}";

	std::string outputString = registers + stats;
	// std::cout << outputString << "\n";
	std::ofstream out("stats.json");
  out << outputString;
  out.close();

}


void core::setup()
{
	output->output("Setting up.\n");
	Super::registerExit();

	// Create a tick function with the frequency specified
	Super::registerClock( clock_frequency, new Clock::Handler<core>(this, &core::tick ) );

	// Memory latency is used to make write/read requests to the SST simulated memory
	//  Simple wrapper to register callbacks
	memory_latency = new SSTMemory(data_memory_link);
}


void core::init(unsigned int phase)
{
	output->output("Initializing.\n");
	// Nothing to do here
}

void core::finish()
{
}

bool core::tick(Cycle_t cycle)
{
	//***********************************************************************************************
	//*	What you need to do is to change the logic in this function with instruction execution *
	//***********************************************************************************************
	std::cout<<"core::tick"<<std::endl;
	num_cycles++;

	if (!busy) {
		// If we are waiting for memory response, do nothing
		if (state == 0) {
			// We are in fetch
			if ((pc/2) < program.size() && !halt) {
				// make sure there are more instructions
				instruction = program[pc/2];
				state = 1;
				lat_counter = getLatency(instruction) - 1;
			} else {
				// finish simulation
				primaryComponentOKToEndSim();
				unregisterExit();
				outputStats();
				return true;
			}
		} else {
			// We are in execute
			lat_counter--;
			if (lat_counter == 0) {
				// execute instruction, increment PC, and return to fetch
				execute(instruction);
				if (!pc_changed) {
					pc += 2;
				}
				state = 0;
			}
		}

	}

	return false;
}


void core::memory_callback(SimpleMem::Request *ev)
{
	if(memory_latency)
	{
		memory_latency->callback(ev);
	}
}

}
}
