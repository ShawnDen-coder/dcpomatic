/*
    Copyright (C) 2014-2016 Carl Hetherington <cth@carlh.net>

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

#include "text_subtitle_decoder.h"
#include "text_subtitle_content.h"
#include <dcp/subtitle_string.h>
#include <boost/foreach.hpp>
#include <iostream>

using std::list;
using std::vector;
using std::string;
using std::cout;
using std::max;
using boost::shared_ptr;
using boost::optional;
using boost::dynamic_pointer_cast;

TextSubtitleDecoder::TextSubtitleDecoder (shared_ptr<const TextSubtitleContent> content)
	: SubtitleDecoder (content)
	, TextSubtitle (content)
	, _next (0)
{

}

void
TextSubtitleDecoder::seek (ContentTime time, bool accurate)
{
	SubtitleDecoder::seek (time, accurate);

	_next = 0;
	while (_next < _subtitles.size() && ContentTime::from_seconds (_subtitles[_next].from.all_as_seconds ()) < time) {
		++_next;
	}
}

bool
TextSubtitleDecoder::pass (PassReason, bool)
{
	if (_next >= _subtitles.size ()) {
		return true;
	}

	/* XXX: we are ignoring positioning specified in the file */

	shared_ptr<const TextSubtitleContent> content = dynamic_pointer_cast<const TextSubtitleContent> (_subtitle_content);
	DCPOMATIC_ASSERT (content);

	list<dcp::SubtitleString> out;

	/* Highest line index in this subtitle */
	int highest = 0;
	BOOST_FOREACH (sub::Line i, _subtitles[_next].lines) {
		DCPOMATIC_ASSERT (i.vertical_position.reference && i.vertical_position.reference.get() == sub::TOP_OF_SUBTITLE);
		DCPOMATIC_ASSERT (i.vertical_position.line);
		highest = max (highest, i.vertical_position.line.get());
	}

	BOOST_FOREACH (sub::Line i, _subtitles[_next].lines) {
		BOOST_FOREACH (sub::Block j, i.blocks) {
			out.push_back (
				dcp::SubtitleString (
					TextSubtitleContent::font_id,
					j.italic,
					j.bold,
					/* force the colour to whatever is configured */
					content->colour(),
					j.font_size.points (72 * 11),
					1.0,
					dcp::Time (_subtitles[_next].from.all_as_seconds(), 1000),
					dcp::Time (_subtitles[_next].to.all_as_seconds(), 1000),
					0,
					dcp::HALIGN_CENTER,
					/* This 1.015 is an arbitrary value to lift the bottom sub off the bottom
					   of the screen a bit to a pleasing degree.
					*/
					1.015 - ((1 + highest - i.vertical_position.line.get()) * 1.5 / 22),
					dcp::VALIGN_TOP,
					j.text,
					content->outline() ? dcp::BORDER : dcp::NONE,
					content->outline_colour(),
					dcp::Time (0, 1000),
					dcp::Time (0, 1000)
					)
				);
		}
	}

	text_subtitle (content_time_period (_subtitles[_next]), out);

	++_next;
	return false;
}

list<ContentTimePeriod>
TextSubtitleDecoder::image_subtitles_during (ContentTimePeriod, bool) const
{
	return list<ContentTimePeriod> ();
}

list<ContentTimePeriod>
TextSubtitleDecoder::text_subtitles_during (ContentTimePeriod p, bool starting) const
{
	/* XXX: inefficient */

	list<ContentTimePeriod> d;

	for (vector<sub::Subtitle>::const_iterator i = _subtitles.begin(); i != _subtitles.end(); ++i) {
		ContentTimePeriod t = content_time_period (*i);
		if ((starting && p.contains (t.from)) || (!starting && p.overlaps (t))) {
			d.push_back (t);
		}
	}

	return d;
}

ContentTimePeriod
TextSubtitleDecoder::content_time_period (sub::Subtitle s) const
{
	return ContentTimePeriod (
		ContentTime::from_seconds (s.from.all_as_seconds()),
		ContentTime::from_seconds (s.to.all_as_seconds())
		);
}
