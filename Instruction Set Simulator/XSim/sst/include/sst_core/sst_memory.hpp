#pragma once
#include <sst/core/interfaces/simpleMem.h>
#include <functional>

using uarch_t = std::uint16_t;

namespace XSim
{
namespace SST
{


class SSTMemory
{
	private:
		using SimpleMem=::SST::Interfaces::SimpleMem;

	public:
		SSTMemory(SimpleMem *data_memory_link);

		SSTMemory(const SSTMemory &other) = default;
		SSTMemory(SSTMemory &&other) = default;

		SSTMemory &operator=(const SSTMemory &other) = default;
		SSTMemory &operator=(SSTMemory &&other) = default;

		virtual ~SSTMemory() noexcept = default;

		void callback(SimpleMem::Request *ev);

		void read(uarch_t address, std::function<void(uarch_t, uarch_t)> cb);

		void write(uarch_t address, std::function<void(uarch_t, uarch_t)> cb);

	protected:
		std::map<uint64_t, std::function<void(uarch_t, uarch_t)> > callbacks;
		SimpleMem *data_memory_link=nullptr;
	private:

};

}
}
