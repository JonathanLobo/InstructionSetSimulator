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
#include <limits.h>
#include <algorithm>

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

std::array<std::string, 8> regFileStatus; // store the FU that will write here

std::vector<struct reservation_station> res;

std::vector<struct functional_unit> fu;

std::string inst = "";
uint16_t pc = 0;
int num_cycles = 0;
int num_stalls = 0;
int num_reg_reads = 0;
std::string output_path = "stats.json";

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

	output_path = params.find<std::string>("output", output_path);

	// read in parameters
	int_number = params.find<uarch_t>("int_number",int_number);
	div_number = params.find<uarch_t>("div_number",div_number);
	mult_number = params.find<uarch_t>("mult_number",mult_number);
	ls_number = params.find<uarch_t>("ls_number",ls_number);

	int_resnumber = params.find<uarch_t>("int_resnumber",int_resnumber);
	div_resnumber = params.find<uarch_t>("div_resnumber",div_resnumber);
	mult_resnumber = params.find<uarch_t>("mult_resnumber",mult_resnumber);
	ls_resnumber = params.find<uarch_t>("ls_resnumber",ls_resnumber);

	int_latency = params.find<uarch_t>("int_latency",int_latency);
	div_latency = params.find<uarch_t>("div_latency",div_latency);
	mult_latency = params.find<uarch_t>("mult_latency",mult_latency);
	ls_latency = params.find<uarch_t>("ls_latency",ls_latency);

	// initialize Reg File
	for (int i = 0; i < (int)regFileStatus.size(); i++) {
		regFileStatus[i] = "RF";
	}

	// initialize reservation stations
	for (int i = 0; i < int_resnumber; i++) {
		struct reservation_station temp;
		temp.op = 'I';
		temp.busy = 0;
		temp.got_fu = 0;
		char buf[3];
		sprintf(buf, "I%d", i+1);
		strcpy(temp.id, buf);
		res.push_back(temp);
		// std::cout << temp.id <<std::endl;
	}
	for (int i = 0; i < div_resnumber; i++) {
		struct reservation_station temp;
		temp.op = 'D';
		temp.busy = 0;
		temp.got_fu = 0;
		char buf[3];
		sprintf(buf, "D%d", i+1);
		strcpy(temp.id, buf);
		res.push_back(temp);
		// std::cout << temp.id <<std::endl;
	}
	for (int i = 0; i < mult_resnumber; i++) {
		struct reservation_station temp;
		temp.op = 'M';
		temp.busy = 0;
		temp.got_fu = 0;
		char buf[3];
		sprintf(buf, "M%d", i+1);
		strcpy(temp.id, buf);
		res.push_back(temp);
		// std::cout << temp.id <<std::endl;
	}
	for (int i = 0; i < ls_resnumber; i++) {
		struct reservation_station temp;
		temp.op = 'L';
		temp.busy = 0;
		temp.got_fu = 0;
		char buf[3];
		sprintf(buf, "L%d", i+1);
		strcpy(temp.id, buf);
		res.push_back(temp);
		// std::cout << temp.id <<std::endl;
	}

	// initialize functional units
	for (int i = 0; i < int_number; i++) {
		struct functional_unit temp;
		temp.op = 'I';
		temp.latency = int_latency;
		temp.busy = 0;
		temp.ready_to_broadcast = 0;
		temp.id = i;
		temp.num_executed = 0;
		fu.push_back(temp);
		// std::cout << temp.op <<std::endl;
	}
	for (int i = 0; i < div_number; i++) {
		struct functional_unit temp;
		temp.op = 'D';
		temp.latency = div_latency;
		temp.busy = 0;
		temp.ready_to_broadcast = 0;
		temp.id = i;
		temp.num_executed = 0;
		fu.push_back(temp);
		// std::cout << temp.op <<std::endl;
	}
	for (int i = 0; i < mult_number; i++) {
		struct functional_unit temp;
		temp.op = 'M';
		temp.latency = mult_latency;
		temp.busy = 0;
		temp.ready_to_broadcast = 0;
		temp.id = i;
		temp.num_executed = 0;
		fu.push_back(temp);
		// std::cout << temp.op <<std::endl;
	}
	for (int i = 0; i < ls_number; i++) {
		struct functional_unit temp;
		temp.op = 'L';
		temp.latency = ls_latency;
		temp.busy = 0;
		temp.ready_to_broadcast = 0;
		temp.id = i;
		temp.num_executed = 0;
		fu.push_back(temp);
		// std::cout << temp.op <<std::endl;
	}

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
	uint16_t address;

	while (std::getline(inputFile, hex))
	{
    std::istringstream iss(hex);
    if (iss >> hex) {
			if (hex.substr(0,1) != "#") {
				value = strtoul(hex.substr(0,std::string::npos).c_str(), NULL, 16);
				// std::cout<<hex.substr(0,std::string::npos)<<std::endl;
				program.push_back(std::bitset<16>(value).to_string());
			}
		}
		if (iss >> hex) {
			if (hex.substr(0,1) != "#") {
				address = strtoul(hex.substr(0,std::string::npos).c_str(), NULL, 16);
				// std::cout<<"MemAddress: "<<hex.substr(0,std::string::npos)<<std::endl;
				memAccesses.push(address);
			}
		}
	}

	std::cout << "Program Length: " << program.size() << "\n";

}

int core::issue(std::string inst) {

	std::string opcode = inst.substr(0,5);

	struct instruction newInstruction;

	char type, fu_type;

	int rd, rs, rt;

	std::string imm;

	char hex[5];
	sprintf(hex, "%x", (unsigned int)strtoul(inst.c_str(), NULL, 2));

	strcpy(newInstruction.hex, hex);

	if (opcode.substr(0,1) == "0" || opcode == "10011") {
		type = 'R';
	} else if (opcode.substr(0,1) == "1" && opcode != "10011" && opcode != "11000") {
		type = 'I';
	} else {
		type = 'X';
	}

	if (type == 'R') {
		rd = std::stoi(inst.substr(5, 3), nullptr, 2);
		rs = std::stoi(inst.substr(8, 3), nullptr, 2);
		rt = std::stoi(inst.substr(11, 3), nullptr, 2);

	} else if (type == 'I') {
		rd = std::stoi(inst.substr(5, 3), nullptr, 2);
		imm = inst.substr(8, 8);
		// std::cout << "Imm: " << imm << "\n";
	} else {
		imm = inst.substr(5, 11);
	}

	if (opcode == "01000" || opcode == "01001") {
		fu_type = 'L';
	} else if (opcode == "00101") {
		fu_type = 'M';
	} else if (opcode == "00100" || opcode == "00110" || opcode == "00111") {
		fu_type = 'D';
	} else {
		fu_type = 'I';
	}

	newInstruction.state = 0;

	if (opcode == "00000") {
		//add
		// regFile[rd] = regFile[rs] + regFile[rt];
		newInstruction.rs1 = rs;
		newInstruction.rs2 = rt;
		newInstruction.rdest = rd;

	} else if (opcode == "00001") {
		//sub
		// regFile[rd] = regFile[rs] - regFile[rt];
		newInstruction.rs1 = rs;
		newInstruction.rs2 = rt;
		newInstruction.rdest = rd;

	}  else if (opcode == "00010") {
		//and
		// regFile[rd] = regFile[rs] & regFile[rt];
		newInstruction.rs1 = rs;
		newInstruction.rs2 = rt;
		newInstruction.rdest = rd;

	}  else if (opcode == "00011") {
		//nor
		// regFile[rd] = ~(regFile[rs] | regFile[rt]);
		newInstruction.rs1 = rs;
		newInstruction.rs2 = rt;
		newInstruction.rdest = rd;

	}  else if (opcode == "00100") {
		//div
		// regFile[rd] = regFile[rs] / regFile[rt];
		newInstruction.rs1 = rs;
		newInstruction.rs2 = rt;
		newInstruction.rdest = rd;

	}  else if (opcode == "00101") {
		//mul
		// regFile[rd] = regFile[rs] * regFile[rt];
		newInstruction.rs1 = rs;
		newInstruction.rs2 = rt;
		newInstruction.rdest = rd;

	}  else if (opcode == "00110") {
		//mod
		// regFile[rd] = regFile[rs] % regFile[rt];
		newInstruction.rs1 = rs;
		newInstruction.rs2 = rt;
		newInstruction.rdest = rd;

	}  else if (opcode == "00111") {
		//exp
		// regFile[rd] = std::pow(regFile[rs], regFile[rt]);
		newInstruction.rs1 = rs;
		newInstruction.rs2 = rt;
		newInstruction.rdest = rd;

	}  else if (opcode == "01000") {
		//lw
		// regFile[rd] = memory[address];
		newInstruction.load_store = 0;
		newInstruction.mem_address = memAccesses.front();
		newInstruction.rs1 = rs;
		newInstruction.rs2 = -1;
		newInstruction.rdest = rd;

	}  else if (opcode == "01001") {
		//sw
		// memory[address] = regFile[rt];
		newInstruction.load_store = 1;
		newInstruction.mem_address = memAccesses.front();
		newInstruction.rs1 = rs;
		newInstruction.rs2 = rt;
		newInstruction.rdest = -1;

	}  else if (opcode == "10000") {
		//liz
		// regFile[rd] = std::stoi(("00000000" + imm), nullptr, 2);
		newInstruction.rs1 = -1;
		newInstruction.rs2 = -1;
		newInstruction.rdest = rd;

	}  else if (opcode == "10001") {
		//lis
		// if (imm.substr(0, 1) == "1") {
		// 	regFile[rd] = std::stoi(("11111111" + imm), nullptr, 2);
		// } else {
		// 	regFile[rd] = std::stoi(("00000000" + imm), nullptr, 2);
		// }
		newInstruction.rs1 = -1;
		newInstruction.rs2 = -1;
		newInstruction.rdest = rd;

	}  else if (opcode == "10010") {
		//lui
		// std::string rdVal = std::bitset<16>(regFile[rd]).to_string();
		// regFile[rd] = std::stoi(imm + rdVal.substr(8,8), nullptr, 2);
		newInstruction.rs1 = -1;
		newInstruction.rs2 = -1;
		newInstruction.rdest = rd;

	}  else if (opcode == "01101") {
		//halt
		// halt = 1;
		newInstruction.rs1 = -1;
		newInstruction.rs2 = -1;
		newInstruction.rdest = -1;

	}  else if (opcode == "01110") {
		//put
		// std::cout << regFile[rs] << "\n";
		newInstruction.rs1 = rs;
		newInstruction.rs2 = -1;
		newInstruction.rdest = -1;

	}

	int toReturn = 0;

	newInstruction.src1_available = 0;
	newInstruction.src2_available = 0;
	newInstruction.src1_renamed = 0;
	newInstruction.src2_renamed = 0;
	newInstruction.src1_read = 0;
	newInstruction.src2_read = 0;
	newInstruction.cycles = 0;
	newInstruction.cycle_issued = num_cycles;

	std::cout << "Attempting to issue instruction: " << inst << std::endl;
	std::cout << "FU Type: " << fu_type << std::endl;
	if (fu_type == 'L') {
		std::cout << "Mem Address: " << newInstruction.mem_address << std::endl;
	}

	for (int i = 0; i < (int)res.size(); i++) {
		if (res[i].op == fu_type && res[i].busy == 0) {
			std::cout << "Assigned to: " << res[i].id << std::endl;
			res[i].inst = newInstruction;
			res[i].busy = 1;
			res[i].got_fu = 0;
			res[i].first_in_queue = 0;

			if (fu_type == 'L') {
				memAccesses.pop();
			}

			toReturn = 1;
			break;
		}
	}

	return toReturn;

}

void core::readOperands(struct reservation_station * res) {

	if (res->inst.state == 0) {
		res->inst.state = 1;
		std::cout << res->id << " was successfully issued!" <<std::endl;
	} else if (res->inst.state == 1) {
		std::cout << res->id << " (" << res->inst.hex << ")" << " is trying to read operands!" <<std::endl;

		// rename register 1 if not done already
		if (res->inst.src1_renamed == 0) {
			if (res->inst.rs1 == -1) {
				res->inst.src1_available = 1;
			} else {
				// inst.src1 = regFileStatus[inst.rs1];
				strcpy(res->inst.src1, regFileStatus[res->inst.rs1].c_str());
			}
			res->inst.src1_renamed = 1;
			std::cout << res->id << " successfully renamed operand 1!" <<std::endl;

			// rename destination register
			if (res->inst.rdest >= 0) {
				regFileStatus[res->inst.rdest] = res->id;
				std::cout << "Successfully set regfile index " << res->inst.rdest << ": " << regFileStatus[res->inst.rdest] <<std::endl;
			}

		}

		// try to read register file for operand if it isn't available
		if (res->inst.src1_available == 0) {

			if (res->inst.src1_read == 0) {
				res->inst.src1_read = 1;
				num_reg_reads++;
			}

			std::cout << res->id << " testing for output from " << regFileStatus[res->inst.rs1] <<std::endl;
			if (strcmp(regFileStatus[res->inst.rs1].c_str(),"RF") == 0) {
				// inst.src1 = "RF";
				std::cout << res->id << " read the regfile for operand 1!" <<std::endl;
				strcpy(res->inst.src1, "RF");
				res->inst.src1_available = 1;
			} else {
				std::cout << res->id << " needs operand 1, is waiting for the CDB for output from " << res->inst.src1 << "!" <<std::endl;

			}

		}

		// rename register 2 if not done already
		if (res->inst.src2_renamed == 0) {
			if (res->inst.rs2 == -1) {
				res->inst.src2_available = 1;
			} else {
				// inst.src2 = regFileStatus[inst.rs2];
				strcpy(res->inst.src2, regFileStatus[res->inst.rs2].c_str());
			}
			res->inst.src2_renamed = 1;
			std::cout << res->id << " successfully renamed operand 2!" <<std::endl;
		}

		// try to read register file for operand if it isn't available
		if (res->inst.src2_available == 0) {

			if (res->inst.src2_read == 0) {
				res->inst.src2_read = 1;
				num_reg_reads++;
			}

			if (strcmp(regFileStatus[res->inst.rs2].c_str(),"RF") == 0) {
				// inst.src1 = "RF";
				std::cout << res->id << " read the regfile for operand 1!" <<std::endl;
				strcpy(res->inst.src2, "RF");
				res->inst.src2_available = 1;
			} else {
				std::cout << res->id << " needs operand 2, is waiting for the CDB for output from " << res->inst.src2 << "!" <<std::endl;
			}

		}

		if (res->inst.src1_available == 1 && res->inst.src2_available == 1) {
			std::cout << res->id << " has all operands available!" <<std::endl;

			// see if there is an available FU
			if (res->op != 'L' || (res->op == 'L' && res->first_in_queue == 1)) {
				for (int i = 0; i < (int)fu.size(); i++) {
					if (fu[i].op == res->op && fu[i].busy == 0) {
						fu[i].inst = res->inst;
						fu[i].busy = 1;
						fu[i].res = res;
						// fu[i].broadcast_id = res.id;
						strcpy(fu[i].broadcast_id, res->id);
						res->got_fu = 1;
						std::cout << res->id << " got functional unit " << fu[i].op << fu[i].id << "!" <<std::endl;
						break;
					}
				}
			}
		}
	}

}

void core::execute(struct functional_unit * fu) {
	if (fu->inst.state == 1) {
		fu->inst.state = 2;
	} else if (fu->inst.state == 2) {
		std::cout << fu->res->id << " executed for 1 cycle." << std::endl;
		fu->inst.cycles++;
		if (fu->inst.cycles == fu->latency) {
			// move to writeback
			fu->ready_to_broadcast = 1;

			// send to memory if necessary
			if (fu->op == 'L') {
				if (fu->inst.load_store == 0) {
					std::function<void(uarch_t, uarch_t)> callback_function = [this](uarch_t request_id, uarch_t addr)
					{
						this->mem_pending=false;
						std::cout<<"Read request "<<request_id<<": Address "<<addr<<std::endl;
					};
					memory_latency->read(fu->inst.mem_address, callback_function);
				} else {
					std::function<void(uarch_t, uarch_t)> callback_function = [this](uarch_t request_id, uarch_t addr)
					{
						this->mem_pending=false;
						std::cout<<"Write request "<<request_id<<": Address "<<addr<<std::endl;
					};
					memory_latency->write(fu->inst.mem_address, callback_function);
				}

				mem_pending=true;
			}

		}
	}
}

void core::writeRegister(struct functional_unit * fu) {
	if (fu->inst.state == 2) {
		fu->inst.state = 3;
	} else if (fu->inst.state == 3) {
		if (fu->op != 'L' || (fu->op == 'L' && mem_pending == false)) {
			fu->busy = 0;
			fu->ready_to_broadcast = 0;
			fu->num_executed++;
			fu->res->busy = 0;
			fu->res->got_fu = 0;
			fu->res->first_in_queue = 0;

			if (fu->inst.rdest >= 0) {
				// send operands to all waiting reservation stations
				for (int i = 0; i < (int)res.size(); i++) {
					if (res[i].busy == 1 && res[i].got_fu == 0) {
						if (strcmp(res[i].inst.src1, fu->broadcast_id) == 0) {
							res[i].inst.src1_available = 1;
						}

						if (strcmp(res[i].inst.src2, fu->broadcast_id) == 0) {
							res[i].inst.src2_available = 1;
						}
					}
				}

				// write to register file
				regFileStatus[fu->inst.rdest] = "RF";
			}

			std::cout << fu->res->id << " broadcasted and wrote to register file." << std::endl;

		} else {
			std::cout << fu->res->id << " is waiting on a pending memory op." << std::endl;
		}
	}
}

std::string core::stringifyFu(struct functional_unit * fu) {
	std::string outputString = std::string("{ \"id\" : ") + std::to_string(fu->id)
		+ ", \"instructions\" : " + std::to_string(fu->num_executed) + " }";

	return outputString;
}

void core::outputStats() {
	std::string outputString =  std::string("{\"cycles\": ") + std::to_string(num_cycles) + ",\n"
		+ "\"integer\" : [";

	int num_added = 0;

	for (int i = 0; i < (int)fu.size(); i++) {
		if (fu[i].op == 'I') {
			outputString += stringifyFu(&fu[i]);
			num_added++;

			if (num_added < int_number) {
				outputString += ", ";
			}
		}
	}
	outputString += "],\n\"multiplier\" : [";

	num_added = 0;
	for (int i = 0; i < (int)fu.size(); i++) {
		if (fu[i].op == 'M') {
			outputString += stringifyFu(&fu[i]);
			num_added++;

			if (num_added < mult_number) {
				outputString += ", ";
			}
		}
	}
	outputString += "],\n\"divider\" : [";

	num_added = 0;
	for (int i = 0; i < (int)fu.size(); i++) {
		if (fu[i].op == 'D') {
			outputString += stringifyFu(&fu[i]);
			num_added++;

			if (num_added < div_number) {
				outputString += ", ";
			}
		}
	}
	outputString += "],\n\"ls\" : [";

	num_added = 0;
	for (int i = 0; i < (int)fu.size(); i++) {
		if (fu[i].op == 'L') {
			outputString += stringifyFu(&fu[i]);
			num_added++;

			if (num_added < ls_number) {
				outputString += ", ";
			}
		}
	}

	outputString = outputString + "],\n" + "\"reg reads\" : " + std::to_string(num_reg_reads) + ",\n"
		+ "\"stalls\" : " + std::to_string(num_stalls) + " }\n";

	std::ofstream out(output_path);
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
	std::cout<<"core::tick"<<std::endl;
	num_cycles++;

	// ISSUE - Issue attempt to issue an instruction
	// If returns 0, we must stall because no FUs are available
	if ((pc/2) < program.size()) {
		inst = program[pc/2];
		if (issue(inst) == 1) {
			pc+=2;
		} else {
			std::cout<<"Stalled"<<std::endl;
			num_stalls++;
		}
	}

	// Ensure FIFO loads and stores
	int min_cycles = INT_MAX;
	for (int i = 0; i < (int)res.size(); i++) {
		if(res[i].busy == 1 && res[i].got_fu == 0 && res[i].op == 'L') {
			if (res[i].inst.cycle_issued < min_cycles) {
				min_cycles = res[i].inst.cycle_issued;
				res[i].first_in_queue = 1;
			}
		}
	}

	// READ OPERANDS
	for (int i = 0; i < (int)res.size(); i++) {
		if(res[i].busy == 1 && res[i].got_fu == 0)
			readOperands(&res[i]);
	}

	// cdb.clear();

	// EXECUTE
	for (int i = 0; i < (int)fu.size(); i++) {
		if(fu[i].busy == 1 && fu[i].ready_to_broadcast == 0)
			execute(&fu[i]);
	}

	// WRITE REGISTER
	for (int i = 0; i < (int)fu.size(); i++) {
		if(fu[i].ready_to_broadcast == 1)
			writeRegister(&fu[i]);
	}

	// check if there are any reservation stations that are still busy
	int terminate = 1;
	for (int i = 0; i < (int)res.size(); i++) {
		if(res[i].busy == 1 ) {
			terminate = 0;
		}
	}
	if (mem_pending == 1) {
		terminate = 0;
	}

	if (terminate == 1) {
		primaryComponentOKToEndSim();
		unregisterExit();
		outputStats();

		// std::cout << "Cycles: " << num_cycles << std::endl;
		// std::cout << "Register Reads: " << num_reg_reads << std::endl;
		// std::cout << "Stalls: " << num_stalls << std::endl;

		return true;
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
