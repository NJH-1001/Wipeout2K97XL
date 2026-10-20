"""Seed the AppImage with a checksum-pinned kit from a successful native CI run.

This lets all assets be audited before a release becomes public. Credentials
go only to the GitHub API; the signed artifact download is anonymous.
"""
import io
import json
import hashlib
import os
from pathlib import Path
import sys
import urllib.error
import urllib.request
import zipfile

root = Path(__file__).resolve().parents[1]
pin = json.loads((root / 'packaging/appimage/dependencies.json').read_text())['linux_kit']
run_id = int(sys.argv[1])
repository = os.environ['GITHUB_REPOSITORY']
assert repository == 'NJH-1001/Wipeout2K97XL'
base = 'https://api.github.com/repos/' + repository
headers = {'Authorization': 'Bearer ' + os.environ['GH_TOKEN'],
           'Accept': 'application/vnd.github+json', 'User-Agent': 'Wipeout2K97XL-packager'}


def api(path):
    with urllib.request.urlopen(urllib.request.Request(base + path, headers=headers), timeout=60) as response:
        return json.load(response)


run = api('/actions/runs/' + str(run_id))
assert run['head_sha'] == pin['source_commit'], 'Wrong native source revision'
jobs = api('/actions/runs/' + str(run_id) + '/jobs')['jobs']
linux_job, = [j for j in jobs if 'linux-x64' in j['name']]
assert linux_job['conclusion'] == 'success', 'Linux build/audit job has not passed'
artifacts = api('/actions/runs/' + str(run_id) + '/artifacts')['artifacts']
artifact, = [a for a in artifacts if a['name'] == 'wipeoutxl-linux-x64' and not a['expired']]


class NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, req, fp, code, msg, response_headers, newurl):
        return None


opener = urllib.request.build_opener(NoRedirect())
try:
    opener.open(urllib.request.Request(base + '/actions/artifacts/' + str(artifact['id']) + '/zip', headers=headers), timeout=60)
    raise RuntimeError('Expected a signed artifact download redirect')
except urllib.error.HTTPError as response:
    assert response.code == 302, response.code
    location = response.headers['Location']
assert location.startswith('https://')
with urllib.request.urlopen(location, timeout=180) as response:
    data = response.read(256 * 1024 * 1024 + 1)
assert len(data) <= 256 * 1024 * 1024
version = (root / 'VERSION').read_text().strip()
name = 'wipeoutxl-' + version + '-linux-x64.zip'
with zipfile.ZipFile(io.BytesIO(data)) as archive:
    entry, = [p for p in archive.namelist() if Path(p).name == name]
    payload = archive.read(entry)
assert hashlib.sha256(payload).hexdigest() == pin['sha256'], 'Linux kit checksum mismatch'
work = root / 'build-appimage'
work.mkdir(exist_ok=True)
(work / 'linux_kit').write_bytes(payload)
print('Verified Linux kit from native CI run', run_id, 'artifact', artifact['id'])
