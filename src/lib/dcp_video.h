/*
    Copyright (C) 2012-2016 Carl Hetherington <cth@carlh.net>

    This file is part of DCP-o-matic.

    DCP-o-matic is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    DCP-o-matic is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with DCP-o-matic.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "types.h"
#include "encode_server_description.h"
#include <libcxml/cxml.h>
#include <dcp/data.h>

/** @file  src/dcp_video_frame.h
 *  @brief A single frame of video destined for a DCP.
 */

class Log;
class PlayerVideo;

/** @class DCPVideo
 *  @brief A single frame of video destined for a DCP.
 *
 *  Given an Image and some settings, this class knows how to encode
 *  the image to J2K either on the local host or on a remote server.
 *
 *  Objects of this class are used for the queue that we keep
 *  of images that require encoding.
 */
class DCPVideo : public boost::noncopyable
{
public:
	DCPVideo (boost::shared_ptr<const PlayerVideo>, int, int, int, Resolution);
	DCPVideo (boost::shared_ptr<const PlayerVideo>, cxml::ConstNodePtr);

	dcp::Data encode_locally ();
	dcp::Data encode_remotely (EncodeServerDescription, int timeout = 30);

	int index () const {
		return _index;
	}

	Eyes eyes () const;

	bool same (boost::shared_ptr<const DCPVideo> other) const;

	static boost::shared_ptr<dcp::OpenJPEGImage> convert_to_xyz (boost::shared_ptr<const PlayerVideo> frame, dcp::NoteHandler note);

private:

	void add_metadata (xmlpp::Element *) const;

	boost::shared_ptr<const PlayerVideo> _frame;
	int _index;			 ///< frame index within the DCP's intrinsic duration
	int _frames_per_second;		 ///< Frames per second that we will use for the DCP
	int _j2k_bandwidth;		 ///< J2K bandwidth to use
	Resolution _resolution;          ///< Resolution (2K or 4K)
};
