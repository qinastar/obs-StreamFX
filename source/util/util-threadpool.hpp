// Copyright (C) 2020-2022 Michael Fabian Dirks
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

#pragma once
#include "warning-disable.hpp"
#include <atomic>
#include <chrono>
#include <cinttypes>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <new>
#include <stdexcept>
#include <thread>
#include "warning-enable.hpp"

namespace streamfx::util::threadpool {
	typedef std::shared_ptr<void>            task_data_t;
	typedef std::function<void(task_data_t)> task_callback_t;

	struct worker_info {
#if __cpp_lib_hardware_interference_size >= 201603
		alignas(std::hardware_destructive_interference_size)
#endif
			std::atomic<bool> stop;

#if __cpp_lib_hardware_interference_size >= 201603
		alignas(std::hardware_destructive_interference_size)
#endif
			std::mutex lifeline;

		std::chrono::high_resolution_clock::time_point last_work_time;

		std::thread thread;
	};

	class task {
		task_callback_t _callback;
		task_data_t     _data;
		std::mutex      _lock;

#if __cpp_lib_hardware_interference_size >= 201603
		alignas(std::hardware_destructive_interference_size)
#endif
			std::condition_variable _status_changed;
#if __cpp_lib_hardware_interference_size >= 201603
		alignas(std::hardware_destructive_interference_size)
#endif
			std::atomic<bool> _cancelled;
#if __cpp_lib_hardware_interference_size >= 201603
		alignas(std::hardware_destructive_interference_size)
#endif
			std::atomic<bool> _completed;
#if __cpp_lib_hardware_interference_size >= 201603
		alignas(std::hardware_destructive_interference_size)
#endif
			std::atomic<bool> _failed;

		public:
		task(task_callback_t callback, task_data_t data);

		public:
		~task();

		public:
		void run();

		public:
		void cancel();

		public:
		bool is_cancelled();

		public:
		bool is_completed();

		public:
		bool has_failed();

		public:
		void wait();

		public:
		void await_completion();
	};

	class threadpool {
		std::pair<size_t, size_t> _limits;

#if __cpp_lib_hardware_interference_size >= 201603
		alignas(std::hardware_destructive_interference_size)
#endif
			std::mutex _workers_lock;
		std::list<std::shared_ptr<worker_info>> _workers;
#if __cpp_lib_hardware_interference_size >= 201603
		alignas(std::hardware_destructive_interference_size)
#endif
			std::atomic<size_t> _worker_count;
		std::chrono::high_resolution_clock::time_point _last_worker_death;

#if __cpp_lib_hardware_interference_size >= 201603
		alignas(std::hardware_destructive_interference_size)
#endif
			std::mutex _tasks_lock;
#if __cpp_lib_hardware_interference_size >= 201603
		alignas(std::hardware_destructive_interference_size)
#endif
			std::condition_variable _tasks_cv;
		std::list<std::shared_ptr<task>> _tasks;

		public:
		~threadpool();

		public:
		threadpool(size_t minimum = 2, size_t maximum = std::thread::hardware_concurrency());

		public:
		std::shared_ptr<task> push(task_callback_t callback, task_data_t data = nullptr);

		public:
		void pop(std::shared_ptr<task> task);

		private:
		void spawn(size_t count = 1);

		private:
		bool die(std::shared_ptr<worker_info>);

		private:
		void work(std::shared_ptr<worker_info>);
	};
} // namespace streamfx::util::threadpool
