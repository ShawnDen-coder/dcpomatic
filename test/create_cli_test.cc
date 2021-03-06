/*
    Copyright (C) 2019 Carl Hetherington <cth@carlh.net>

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

#include "lib/create_cli.h"
#include "lib/ratio.h"
#include "lib/dcp_content_type.h"
#include <boost/test/unit_test.hpp>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <iostream>

using std::string;

static CreateCLI
run (string cmd)
{
	/* This approximates the logic which splits command lines up into argc/argv */

	boost::escaped_list_separator<char> els ("", " ", "\"\'");
	boost::tokenizer<boost::escaped_list_separator<char> > tok (cmd, els);

	char** argv = new char*[256];
	int argc = 0;

	for (boost::tokenizer<boost::escaped_list_separator<char> >::iterator i = tok.begin(); i != tok.end(); ++i) {
		argv[argc++] = strdup (i->c_str());
	}

	CreateCLI cc (argc, argv);

	for (int i = 0; i < argc; ++i) {
		free (argv[i]);
	}

	delete[] argv;

	return cc;
}

BOOST_AUTO_TEST_CASE (create_cli_test)
{
	CreateCLI cc = run ("dcpomatic2_create --version");
	BOOST_CHECK (!cc.error);
	BOOST_CHECK (cc.version);

	cc = run ("dcpomatic2_create --versionX");
	BOOST_REQUIRE (cc.error);
	BOOST_CHECK (boost::algorithm::starts_with(*cc.error, "dcpomatic2_create: unrecognised option '--versionX'"));

	cc = run ("dcpomatic2_create --help");
	BOOST_REQUIRE (cc.error);

	cc = run ("dcpomatic2_create -h");
	BOOST_REQUIRE (cc.error);

	cc = run ("dcpomatic2_create x --content-ratio 185 --name frobozz --template bar");
	BOOST_CHECK (!cc.error);
	BOOST_CHECK_EQUAL (cc.name, "frobozz");
	BOOST_REQUIRE (cc.template_name);
	BOOST_CHECK_EQUAL (*cc.template_name, "bar");

	cc = run ("dcpomatic2_create x --content-ratio 185 --dcp-content-type FTR");
	BOOST_CHECK (!cc.error);
	BOOST_CHECK_EQUAL (cc.dcp_content_type, DCPContentType::from_isdcf_name("FTR"));

	cc = run ("dcpomatic2_create x --content-ratio 185 --dcp-frame-rate 30");
	BOOST_CHECK (!cc.error);
	BOOST_REQUIRE (cc.dcp_frame_rate);
	BOOST_CHECK_EQUAL (*cc.dcp_frame_rate, 30);

	cc = run ("dcpomatic2_create x --content-ratio 185 --container-ratio 185");
	BOOST_CHECK (!cc.error);
	BOOST_CHECK_EQUAL (cc.container_ratio, Ratio::from_id("185"));

	cc = run ("dcpomatic2_create x --content-ratio 185 --container-ratio XXX");
	BOOST_CHECK (cc.error);

	cc = run ("dcpomatic2_create x --content-ratio 185 --content-ratio 239");
	BOOST_CHECK (!cc.error);
	BOOST_CHECK_EQUAL (cc.content_ratio, Ratio::from_id("239"));

	cc = run ("dcpomatic2_create x --content-ratio 240");
	BOOST_CHECK (cc.error);

	cc = run ("dcpomatic2_create x --content-ratio 185 --still-length 42");
	BOOST_CHECK (!cc.error);
	BOOST_CHECK_EQUAL (cc.still_length, 42);

	cc = run ("dcpomatic2_create x --content-ratio 185 --standard SMPTE");
	BOOST_CHECK (!cc.error);
	BOOST_CHECK_EQUAL (cc.standard, dcp::SMPTE);

	cc = run ("dcpomatic2_create x --content-ratio 185 --standard SMPTEX");
	BOOST_CHECK (cc.error);

	cc = run ("dcpomatic2_create x --content-ratio 185 --config foo/bar");
	BOOST_CHECK (!cc.error);
	BOOST_REQUIRE (cc.config_dir);
	BOOST_CHECK_EQUAL (*cc.config_dir, "foo/bar");

	cc = run ("dcpomatic2_create x --content-ratio 185 --output fred/jim");
	BOOST_CHECK (!cc.error);
	BOOST_REQUIRE (cc.output_dir);
	BOOST_CHECK_EQUAL (*cc.output_dir, "fred/jim");

	cc = run ("dcpomatic2_create x --content-ratio 185 --outputX fred/jim");
	BOOST_CHECK (cc.error);

	cc = run ("dcpomatic2_create --content-ratio 185 --config foo/bar --still-length 42 --output flaps fred jim sheila");
	BOOST_CHECK (!cc.error);
	BOOST_REQUIRE (cc.config_dir);
	BOOST_CHECK_EQUAL (*cc.config_dir, "foo/bar");
	BOOST_CHECK_EQUAL (cc.still_length, 42);
	BOOST_REQUIRE (cc.output_dir);
	BOOST_CHECK_EQUAL (*cc.output_dir, "flaps");
	BOOST_REQUIRE_EQUAL (cc.content.size(), 3);
	BOOST_CHECK_EQUAL (cc.content[0].path, "fred");
	BOOST_CHECK_EQUAL (cc.content[0].frame_type, VIDEO_FRAME_TYPE_2D);
	BOOST_CHECK_EQUAL (cc.content[1].path, "jim");
	BOOST_CHECK_EQUAL (cc.content[1].frame_type, VIDEO_FRAME_TYPE_2D);
	BOOST_CHECK_EQUAL (cc.content[2].path, "sheila");
	BOOST_CHECK_EQUAL (cc.content[2].frame_type, VIDEO_FRAME_TYPE_2D);

	cc = run ("dcpomatic2_create --content-ratio 185 --left-eye left.mp4 --right-eye right.mp4");
	BOOST_REQUIRE_EQUAL (cc.content.size(), 2);
	BOOST_CHECK_EQUAL (cc.content[0].path, "left.mp4");
	BOOST_CHECK_EQUAL (cc.content[0].frame_type, VIDEO_FRAME_TYPE_3D_LEFT);
	BOOST_CHECK_EQUAL (cc.content[1].path, "right.mp4");
	BOOST_CHECK_EQUAL (cc.content[1].frame_type, VIDEO_FRAME_TYPE_3D_RIGHT);
	BOOST_CHECK_EQUAL (cc.fourk, false);

	cc = run ("dcpomatic2_create --fourk --content-ratio 185 foo.mp4");
	BOOST_REQUIRE_EQUAL (cc.content.size(), 1);
	BOOST_CHECK_EQUAL (cc.content[0].path, "foo.mp4");
	BOOST_CHECK_EQUAL (cc.fourk, true);
	BOOST_CHECK (!cc.error);

	cc = run ("dcpomatic2_create --j2k-bandwidth 120 --content-ratio 185 foo.mp4");
	BOOST_REQUIRE_EQUAL (cc.content.size(), 1);
	BOOST_CHECK_EQUAL (cc.content[0].path, "foo.mp4");
	BOOST_REQUIRE (cc.j2k_bandwidth);
	BOOST_CHECK_EQUAL (*cc.j2k_bandwidth, 120000000);
	BOOST_CHECK (!cc.error);
}
