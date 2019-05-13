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

#ifdef DCPOMATIC_VARIANT_SWAROOP

#include "encrypted_ecinema_kdm.h"
#include <dcp/key.h>
#include <dcp/data.h>
#include <dcp/certificate.h>

class DecryptedECinemaKDM
{
public:
	DecryptedECinemaKDM (std::string id, dcp::Key content_key);
	DecryptedECinemaKDM (EncryptedECinemaKDM kdm, std::string private_key);

	EncryptedECinemaKDM encrypt (dcp::Certificate recipient);

	std::string id () const {
		return _id;
	}

	dcp::Key key () const {
		return _content_key;
	}

private:
	std::string _id;
	/** unenecrypted content key */
	dcp::Key _content_key;
};

#endif
