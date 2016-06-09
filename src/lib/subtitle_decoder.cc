/*
    Copyright (C) 2013-2016 Carl Hetherington <cth@carlh.net>

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

#include "subtitle_decoder.h"
#include "subtitle_content.h"
#include "util.h"
#include <sub/subtitle.h>
#include <boost/shared_ptr.hpp>
#include <boost/foreach.hpp>
#include <iostream>

using std::list;
using std::cout;
using std::string;
using boost::shared_ptr;
using boost::optional;
using boost::function;

SubtitleDecoder::SubtitleDecoder (
	Decoder* parent,
	shared_ptr<const SubtitleContent> c,
	function<list<ContentTimePeriod> (ContentTimePeriod, bool)> image_during,
	function<list<ContentTimePeriod> (ContentTimePeriod, bool)> text_during
	)
	: _parent (parent)
	, _content (c)
	, _image_during (image_during)
	, _text_during (text_during)
{

}

/** Called by subclasses when an image subtitle is ready.
 *  @param period Period of the subtitle.
 *  @param image Subtitle image.
 *  @param rect Area expressed as a fraction of the video frame that this subtitle
 *  is for (e.g. a width of 0.5 means the width of the subtitle is half the width
 *  of the video frame)
 */
void
SubtitleDecoder::give_image (ContentTimePeriod period, shared_ptr<Image> image, dcpomatic::Rect<double> rect)
{
	_decoded_image.push_back (ContentImageSubtitle (period, image, rect));
}

void
SubtitleDecoder::give_text (ContentTimePeriod period, list<dcp::SubtitleString> s)
{
	_decoded_text.push_back (ContentTextSubtitle (period, s));
}

/** @param sp Full periods of subtitles that are showing or starting during the specified period */
template <class T>
list<T>
SubtitleDecoder::get (list<T> const & subs, list<ContentTimePeriod> const & sp, ContentTimePeriod period, bool starting, bool accurate)
{
	if (sp.empty ()) {
		/* Nothing in this period */
		return list<T> ();
	}

	/* Seek if what we want is before what we have, or a more than a little bit after */
	if (subs.empty() || sp.back().to < subs.front().period().from || sp.front().from > (subs.back().period().to + ContentTime::from_seconds (1))) {
		_parent->seek (sp.front().from, true);
	}

	/* Now enough pass() calls will either:
	 *  (a) give us what we want, or
	 *  (b) hit the end of the decoder.
	 */
	while (!_parent->pass(Decoder::PASS_REASON_SUBTITLE, accurate) && (subs.empty() || (subs.back().period().to < sp.back().to))) {}

	/* Now look for what we wanted in the data we have collected */
	/* XXX: inefficient */

	list<T> out;
	for (typename list<T>::const_iterator i = subs.begin(); i != subs.end(); ++i) {
		if ((starting && period.contains (i->period().from)) || (!starting && period.overlaps (i->period ()))) {
			out.push_back (*i);
		}
	}

	/* Discard anything in _decoded_image_subtitles that is outside 5 seconds either side of period */

	list<ContentImageSubtitle>::iterator i = _decoded_image.begin();
	while (i != _decoded_image.end()) {
		list<ContentImageSubtitle>::iterator tmp = i;
		++tmp;

		if (
			i->period().to < (period.from - ContentTime::from_seconds (5)) ||
			i->period().from > (period.to + ContentTime::from_seconds (5))
			) {
			_decoded_image.erase (i);
		}

		i = tmp;
	}

	return out;
}

list<ContentTextSubtitle>
SubtitleDecoder::get_text (ContentTimePeriod period, bool starting, bool accurate)
{
	return get<ContentTextSubtitle> (_decoded_text, _text_during (period, starting), period, starting, accurate);
}

list<ContentImageSubtitle>
SubtitleDecoder::get_image (ContentTimePeriod period, bool starting, bool accurate)
{
	return get<ContentImageSubtitle> (_decoded_image, _image_during (period, starting), period, starting, accurate);
}

void
SubtitleDecoder::seek (ContentTime, bool)
{
	_decoded_text.clear ();
	_decoded_image.clear ();
}

void
SubtitleDecoder::give_text (ContentTimePeriod period, sub::Subtitle const & subtitle)
{
	/* See if our next subtitle needs to be placed on screen by us */
	bool needs_placement = false;
	BOOST_FOREACH (sub::Line i, subtitle.lines) {
		if (!i.vertical_position.reference && i.vertical_position.reference.get() == sub::TOP_OF_SUBTITLE) {
			needs_placement = true;
		}
	}

	list<dcp::SubtitleString> out;
	BOOST_FOREACH (sub::Line i, subtitle.lines) {
		BOOST_FOREACH (sub::Block j, i.blocks) {

			float v_position;
			dcp::VAlign v_align;
			if (needs_placement) {
				DCPOMATIC_ASSERT (i.vertical_position.line);
				/* This 0.878 is an arbitrary value to lift the bottom sub off the bottom
				   of the screen a bit to a pleasing degree.
				*/
				v_position = 0.878 + i.vertical_position.line.get() * 1.5 / 22;
				v_align = dcp::VALIGN_BOTTOM;
			} else {
				DCPOMATIC_ASSERT (i.vertical_position.proportional);
				DCPOMATIC_ASSERT (i.vertical_position.reference);
				v_position = i.vertical_position.proportional.get();
				switch (i.vertical_position.reference.get()) {
				case sub::TOP_OF_SCREEN:
					v_align = dcp::VALIGN_TOP;
					break;
				case sub::CENTRE_OF_SCREEN:
					v_align = dcp::VALIGN_CENTER;
					break;
				case sub::BOTTOM_OF_SCREEN:
					v_align = dcp::VALIGN_BOTTOM;
					break;
				default:
					v_align = dcp::VALIGN_TOP;
					break;
				}
			}

			out.push_back (
				dcp::SubtitleString (
					string(TEXT_FONT_ID),
					j.italic,
					j.bold,
					/* force the colour to whatever is configured */
					content()->colour(),
					j.font_size.points (72 * 11),
					1.0,
					dcp::Time (subtitle.from.all_as_seconds(), 1000),
					dcp::Time (subtitle.to.all_as_seconds(), 1000),
					0,
					dcp::HALIGN_CENTER,
					v_position,
					v_align,
					dcp::DIRECTION_LTR,
					j.text,
					content()->outline() ? dcp::BORDER : dcp::NONE,
					content()->outline_colour(),
					dcp::Time (0, 1000),
					dcp::Time (0, 1000)
					)
				);
		}
	}

	give_text (period, out);
}
