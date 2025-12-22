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
#include <botan/pkcs8.h>
#include <botan/pk_keys.h>
#include <botan/rng.h>
#include <botan/x509_key.h>

#include "gzip.hh"
#include "lazy_rng.hh"
#include "sanity.hh"

using std::make_shared;
using std::shared_ptr;
using std::string;
#if BOTAN_VERSION_CODE >= BOTAN_VERSION_CODE_FOR(3,0,0)
using Botan::Private_Key;
using PrivateKeyPtr = std::shared_ptr<Private_Key>;
#else
using Botan::PKCS8_PrivateKey;
using PrivateKeyPtr = std::shared_ptr<PKCS8_PrivateKey>;
#endif


void
initialize_botan(bool for_testing)
{
}


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
      Botan::DataSource_Memory ds(priv_key);
      return PrivateKeyPtr(
        Botan::PKCS8::load_key(ds, pass_req_throw_func));
    }
  catch (Botan::Decoding_Error const & e)
    {
      // Since Botan 1.11.14, the load_key method catches *all* exceptions
      // thrown by the `get_passphrase` function passed and wrapps them in
      // a `Decoding_Error`. The only way to figure it has been thrown is
      // checking the error message.
      //
      // FIXME: instead of blindly calling load_key and let it throw,
      //        consider parsing the PEM header before-hand, instead.
      if (strstr(e.what(), "Passphrase required") != NULL)
        throw Passphrase_Required("Passphrase required");
      E(false, origin::user,
        F("malformed key_packet: invalid private key data for '%s': %s")
          % name % e.what());
    }
  // Since we do not want to prompt for a password to decode it finally,
  // we ignore the exceptions that indicate a missing password.  These
  // differ slightly between Botan versions.
}
