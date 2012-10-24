/*
    Copyright (C) 2012 Carl Hetherington <cth@carlh.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

/** @file src/job.h
 *  @brief A parent class to represent long-running tasks which are run in their own thread.
 */

#ifndef DVDOMATIC_JOB_H
#define DVDOMATIC_JOB_H

#include <string>
#include <boost/thread/mutex.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/signals2.hpp>

class Film;
class Options;

/** @class Job
 *  @brief A parent class to represent long-running tasks which are run in their own thread.
 */
class Job : public boost::enable_shared_from_this<Job>
{
public:
	Job (boost::shared_ptr<Film> s, boost::shared_ptr<Job> req);

	/** @return user-readable name of this job */
	virtual std::string name () const = 0;
	/** Run this job in the current thread. */
	virtual void run () = 0;
	
	void start ();

	bool is_new () const;
	bool running () const;
	bool finished () const;
	bool finished_ok () const;
	bool finished_in_error () const;

	std::string error () const;

	int elapsed_time () const;
	virtual std::string status () const;

	void set_progress_unknown ();
	void set_progress (float);
	void ascend ();
	void descend (float);
	float overall_progress () const;

	boost::shared_ptr<Job> required () const {
		return _required;
	}

	boost::signals2::signal<void()> Finished;

protected:

	virtual int remaining_time () const;

	/** Description of a job's state */
	enum State {
		NEW,           ///< the job hasn't been started yet
		RUNNING,       ///< the job is running
		FINISHED_OK,   ///< the job has finished successfully
		FINISHED_ERROR ///< the job has finished in error
	};
	
	void set_state (State);
	void set_error (std::string e);

	/** Film for this job */
	boost::shared_ptr<Film> _film;

private:

	void run_wrapper ();

	boost::shared_ptr<Job> _required;

	/** mutex for _state and _error */
	mutable boost::mutex _state_mutex;
	/** current state of the job */
	State _state;
	/** message for an error that has occurred (when state == FINISHED_ERROR) */
	std::string _error;

	/** time that this job was started */
	time_t _start_time;

	/** mutex for _stack and _progress_unknown */
	mutable boost::mutex _progress_mutex;

	struct Level {
		Level (float a) : allocation (a), normalised (0) {}

		float allocation;
		float normalised;
	};

	std::list<Level> _stack;

	/** true if this job's progress will always be unknown */
	bool _progress_unknown;

	int _ran_for;
};

#endif
