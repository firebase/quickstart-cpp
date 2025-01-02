# Copyright 2022 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Script for restoring secrets into the testapp projects.

Usage:

python restore_secrets.py --passphrase [--repo_dir <path_to_repo>]
python restore_secrets.py --passphrase_file [--repo_dir <path_to_repo>]

--passphrase: Passphrase to decrypt the files. This option is insecure on a
    multi-user machine; use the --passphrase_file option instead.
--passphrase_file: Specify a file to read the passphrase from (only reads the
    first line). Use "-" (without quotes) for stdin.
--repo_dir: Path to C++ Quickstart Github repository. Defaults to current
    directory.
--apis: Specify a list of particular product APIs and retrieve only their
    secrets.

This script will perform the following:

- Google Service files (plist and json) will be restored into the
  testapp directories.
- The reverse id will be patched into all Info.plist files, using the value from
  the decrypted Google Service plist files as the source of truth.

"""

import os
import plistlib
import subprocess

from absl import app
from absl import flags


FLAGS = flags.FLAGS

flags.DEFINE_string("repo_dir", os.getcwd(), "Path to C++ SDK Github repo.")
flags.DEFINE_string("passphrase", None, "The passphrase itself.")
flags.DEFINE_string("passphrase_file", None,
                    "Path to file with passphrase. Use \"-\" (without quotes) for stdin.")
flags.DEFINE_string("artifact", None, "Artifact Path, google-services.json will be placed here.")
flags.DEFINE_list("apis",[], "Optional comma-separated list of APIs for which to retreive "
                   " secrets. All secrets will be fetched if this is flag is not defined.")


def main(argv):
  if len(argv) > 1:
    raise app.UsageError("Too many command-line arguments.")

  repo_dir = FLAGS.repo_dir
  # The passphrase is sensitive, do not log.
  if FLAGS.passphrase:
    passphrase = FLAGS.passphrase
  elif FLAGS.passphrase_file == "-":
    passphrase = input()
  elif FLAGS.passphrase_file:
    with open(FLAGS.passphrase_file, "r") as f:
      passphrase = f.readline().strip()
  else:
    raise ValueError("Must supply passphrase or passphrase_file arg.")

  if FLAGS.apis:
    print("Retrieving secrets for product APIs: ", FLAGS.apis)

  secrets_dir = os.path.join(repo_dir, "scripts", "gha-encrypted")
  encrypted_files = _find_encrypted_files(secrets_dir)
  print("Found these encrypted files:\n%s" % "\n".join(encrypted_files))

  for path in encrypted_files:
    if "google-services" in path or "GoogleService" in path:
      # We infer the destination from the file's directory, example:
      # /scripts/gha-encrypted/auth/google-services.json.gpg turns into
      # /<repo_dir>/auth/testapp/google-services.json
      api = os.path.basename(os.path.dirname(path))
      if FLAGS.apis and api not in FLAGS.apis:
        print("Skipping secret found in product api", api)
        continue
      print("Encrypted Google Service file found: %s" % path)
      file_name = os.path.basename(path).replace(".gpg", "")
      dest_paths = [os.path.join(repo_dir, api, "testapp", file_name)]
      if FLAGS.artifact:
        # /<repo_dir>/<artifact>/auth/google-services.json
        if "google-services" in path and os.path.isdir(os.path.join(repo_dir, FLAGS.artifact, api)):
          dest_paths = [os.path.join(repo_dir, FLAGS.artifact, api, file_name)]
        else:
          continue

      decrypted_text = _decrypt(path, passphrase)
      for dest_path in dest_paths:
        with open(dest_path, "w") as f:
          f.write(decrypted_text)
        print("Copied decrypted google service file to %s" % dest_path)
        # We use a Google Service file as the source of truth for the reverse id
        # that needs to be patched into the Info.plist files.
        if dest_path.endswith(".plist"):
          _patch_reverse_id(dest_path)
          _patch_bundle_id(dest_path)

  if FLAGS.artifact:
    return

def _find_encrypted_files(directory_to_search):
  """Returns a list of full paths to all files encrypted with gpg."""
  encrypted_files = []
  for prefix, _, files in os.walk(directory_to_search):
    for relative_path in files:
      if relative_path.endswith(".gpg"):
        encrypted_files.append(os.path.join(prefix, relative_path))
  return encrypted_files


def _decrypt(encrypted_file, passphrase):
  """Returns the decrypted contents of the given .gpg file."""
  print("Decrypting %s" % encrypted_file)
  # Note: if setting check=True, be sure to catch the error and not rethrow it
  # or print a traceback, as the message will include the passphrase.
  result = subprocess.run(
      args=[
          "gpg",
          "--passphrase", passphrase,
          "--quiet",
          "--batch",
          "--yes",
          "--decrypt",
          encrypted_file],
      check=False,
      text=True,
      capture_output=True)
  if result.returncode:
    # Remove any instances of the passphrase from error before logging it.
    raise RuntimeError(result.stderr.replace(passphrase, "****"))
  print("Decryption successful")
  # rstrip to eliminate a linebreak that GPG may introduce.
  return result.stdout.rstrip()


def _patch_reverse_id(service_plist_path):
  """Patches the Info.plist file with the reverse id from the Service plist."""
  print("Attempting to patch reverse id in Info.plist")
  with open(service_plist_path, "rb") as f:
    service_plist = plistlib.load(f)
  try:
    _patch_file(
        path=os.path.join(os.path.dirname(service_plist_path), "testapp", "Info.plist"),
        placeholder="REPLACE_WITH_REVERSED_CLIENT_ID",
        value=service_plist["REVERSED_CLIENT_ID"])
  except KeyError as e:
    print("Warning: Missing plist key %s in %s, skipping" % (e.args[0], service_plist_path))


def _patch_bundle_id(service_plist_path):
  """Patches the Info.plist file with the bundle id from the Service plist."""
  print("Attempting to patch bundle id in Info.plist")
  with open(service_plist_path, "rb") as f:
    service_plist = plistlib.load(f)
  try:
    _patch_file(
        path=os.path.join(os.path.dirname(service_plist_path), "testapp", "Info.plist"),
        placeholder="$(PRODUCT_BUNDLE_IDENTIFIER)",
        value=service_plist["BUNDLE_ID"])
  except KeyError as e:
    print("Warning: Missing plist key %s in %s, skipping" % (e.args[0], service_plist_path))


def _patch_file(path, placeholder, value):
  """Patches instances of the placeholder with the given value."""
  # Note: value may be sensitive, so do not log.
  with open(path, "r") as f_read:
    text = f_read.read()
  # Count number of times placeholder appears for debugging purposes.
  replacements = text.count(placeholder)
  patched_text = text.replace(placeholder, value)
  with open(path, "w") as f_write:
    f_write.write(patched_text)
  print("Patched %d instances of %s in %s" % (replacements, placeholder, path))


if __name__ == "__main__":
  app.run(main)
