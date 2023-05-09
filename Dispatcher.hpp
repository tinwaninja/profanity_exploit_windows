#ifndef HPP_DISPATCHER
#define HPP_DISPATCHER

#include <stdexcept>
#include <fstream>
#include <string>
#include <vector>
#include <mutex>
#include <unordered_map>

#if defined(__APPLE__) || defined(__MACOSX)
#include <OpenCL/cl.h>
#define clCreateCommandQueueWithProperties clCreateCommandQueue
#else
#include <CL/cl.h>
#endif

#include "SpeedSample.hpp"
#include "CLMemory.hpp"
#include "types.hpp"
#include "Mode.hpp"

#define PROFANITY_SPEEDSAMPLES 20
#define PROFANITY_MAX_SCORE 40

typedef std::pair<std::pair<unsigned int, unsigned int>, unsigned int> addr;

class Dispatcher {
	private:
		class OpenCLException : public std::runtime_error {
			public:
				OpenCLException(const std::string s, const cl_int res);

				static void throwIfError(const std::string s, const cl_int res);

				const cl_int m_res;
		};

		struct Device {
			static cl_command_queue createQueue(cl_context & clContext, cl_device_id & clDeviceId);
			static cl_kernel createKernel(cl_program & clProgram, const std::string s);
			static cl_ulong4 createSeed();

			Device(Dispatcher & parent, cl_context & clContext, cl_program & clProgram, cl_device_id clDeviceId, const size_t worksizeLocal, const size_t size, const size_t index, const Mode & mode);
			~Device();

			Dispatcher & m_parent;
			const size_t m_index;

			cl_device_id m_clDeviceId;
			size_t m_worksizeLocal;
			cl_uchar m_clScoreMax;
			cl_command_queue m_clQueue;

			cl_kernel m_kernelInit;
			cl_kernel m_kernelClearHashTable;
			cl_kernel m_kernelInitHashTable;
			cl_kernel m_kernelInverse;
			cl_kernel m_kernelIterate;
			cl_kernel m_kernelTransform;
			cl_kernel m_kernelClearResults;
			cl_kernel m_kernelScore;

			CLMemory<point> m_memPrecomp;
			CLMemory<mp_number> m_memPointsDeltaX;
			CLMemory<mp_number> m_memInversedNegativeDoubleGy;
			CLMemory<mp_number> m_memPrevLambda;
			CLMemory<result> m_memResult;
			CLMemory<cl_ulong4> m_memSeed;
			CLMemory<cl_uint> m_memHashTable;
			CLMemory<cl_uint> m_memPublicAddress;
			CLMemory<cl_uint> m_memPublicBytes;

			// Data parameters used in some modes
			const Mode& m_mode;
			CLMemory<cl_uchar> m_memData1;
			CLMemory<cl_uchar> m_memData2;

			// Seed and round information
			cl_ulong4 m_clSeed;
			cl_ulong m_round;
			point m_Target;
			size_t m_batchIndex;

			// Speed sampling
			SpeedSample m_speed;

			// Initialization
			size_t m_sizeInitialized;
			cl_event m_eventFinished;

			// HashTable Initialization
			size_t m_sizeHashTableCleared;
			size_t m_iterHashTableInitialized;
			size_t m_sizeHashTableInitialized;
			std::vector<addr> m_addresses;
		};

	public:
		Dispatcher(cl_context & clContext, cl_program & clProgram, const Mode mode, const size_t worksizeMax, const size_t inverseSize, const size_t inverseMultiple, const cl_uchar clScoreQuit = 0);
		~Dispatcher();

		void addDevice(cl_device_id clDeviceId, const size_t worksizeLocal, const size_t index);
		void run();
		void runReverse();

		void writeAddresses(std::string& filename, std::vector<addr>& addresses);
		void readAddresses(std::string& filename, std::vector<addr>& addresses);

	private:
		void init();
		void initBegin(Device & d);
		void initContinue(Device & d);
		void initHashTableContinue(Device & d);

		void dispatch(Device & d);
		void enqueueKernel(cl_command_queue & clQueue, cl_kernel & clKernel, size_t worksizeGlobal, const size_t worksizeLocal, cl_event * pEvent);
		void enqueueKernelDevice(Device & d, cl_kernel & clKernel, size_t worksizeGlobal, cl_event * pEvent);

		void handleResult(Device & d);
		void handleReverse(Device & d);
		void randomizeSeed(Device & d);

		void onEvent(cl_event event, cl_int status, Device & d);

		void printSpeed();

	private:
		static void CL_CALLBACK staticCallback(cl_event event, cl_int event_command_exec_status, void * user_data);

		static std::string formatSpeed(double s);

	private: /* Instance variables */
		cl_context & m_clContext;
		cl_program & m_clProgram;
		const Mode m_mode;
		const size_t m_worksizeMax;
		const size_t m_inverseSize;
		const size_t m_size;
		const size_t m_HashTableSize;
		cl_uchar m_clScoreMax;
		cl_uchar m_clScoreQuit;

		std::vector<Device *> m_vDevices;

		cl_event m_eventFinished;

		// Run information
		std::mutex m_mutex;
		std::chrono::time_point<std::chrono::steady_clock> timeStart;
		std::chrono::time_point<std::chrono::steady_clock> timeInitStart;
		std::chrono::time_point<std::chrono::steady_clock> timeRunStart;
		unsigned int m_countPrint;
		unsigned int m_countRunning;
		size_t m_sizeInitTotal;
		size_t m_sizeInitDone;
		size_t m_sizeHashTableInitTotal;
		size_t m_sizeHashTableInitDone;
		size_t m_stepsDone;
		size_t m_stepsTotal;
		bool m_quit;
		// size_t m_batchX;
		size_t m_batchY;
};

#endif /* HPP_DISPATCHER */
