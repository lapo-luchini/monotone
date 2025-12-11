// Copyright (C) 2017 Markus Wanner <markus@bluegap.ch>
//
// This program is made available under the GNU GPL version 2.0 or
// greater. See the accompanying file COPYING for details.
//
// This program is distributed WITHOUT ANY WARRANTY; without even the
// implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
// PURPOSE.

#include "base.hh"

#include <memory>

#include "botan_glue.hh"
// Botan 3 reorganized headers
#include <botan/pkcs8.h>
#include <botan/pk_keys.h>
#include <botan/rng.h>
#include <botan/x509_key.h>

// UI interface is not needed for Botan 3.0.0+

#include "gzip.hh"
#include "lazy_rng.hh"
#include "sanity.hh"

using std::make_shared;
using std::shared_ptr;
using std::string;
// Botan 3 changed the class name for private keys
using Botan::Private_Key;
using PrivateKeyPtr = std::shared_ptr<Private_Key>;


void
initialize_botan(bool for_testing)
{
  // Botan 3 doesn't need explicit initialization
}


// A helper class implementing Botan::User_Interface - which doesn't really
// interface with the user, but provides the necessary plumbing for Botan.
//
// See Botan commit 2d09d7d0cd4bd0e7155d001dd65a4f29103b158c
// Botan 3 uses different password callback mechanism


// A simplistic function throwing the Passphrase_Required exception. To be
// passed to Botan as a callback.
std::function<std::string ()> pass_req_throw_func =
  [] ()
    {
      throw Passphrase_Required("Passphrase required");
      return string();
    };

// A Botan-version agnostic key loader function trying to load an
// unprotected key, i.e. one that loads without any password. Returns
// a pointer to the loaded key, if successful, throws a Passphrase_Required
// exception, if a password is required or throws a Decoding_error in case
// of invalid data.
PrivateKeyPtr
load_pkcs8_key(string const & name, string const & priv_key)
{
  try
    {
      // Botan 3 renamed DataSource_Memory to DataSource_Stream
      Botan::DataSource_Stream ds(priv_key);
      // Botan 3 uses a different API for loading keys
      return PrivateKeyPtr(
        Botan::PKCS8::load_key(ds, pass_req_throw_func));
    }
  catch (Botan::Decoding_Error const & e)
    {
      // Botan 3 handles exceptions differently
      if (strstr(e.what(), "Passphrase required") != NULL)
        throw Passphrase_Required("Passphrase required");
      E(false, origin::user,
        F("malformed key_packet: invalid private key data for '%s': %s")
          % name % e.what());
    }
  // Since we do not want to prompt for a password to decode it finally,
  // we ignore the exceptions that indicate a missing password.  These
  // differ slightly between Botan versions.
  // Botan 3 uses standard exceptions
  catch (Passphrase_Required)
    { throw Passphrase_Required("Passphrase required"); }
}
