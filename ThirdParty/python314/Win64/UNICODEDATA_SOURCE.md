# Windows Unicode standard library extension

`unicodedata.pyd` and `LICENSE.txt` come from the official CPython 3.14.2 Windows
embeddable x64 archive, matching the bundled `python314.dll` and headers.

- Source: https://www.python.org/ftp/python/3.14.2/python-3.14.2-embed-amd64.zip
- Archive MD5 published on the release page: `85a5dcd0350f7ca4e2876295b0b281e3`
- `unicodedata.pyd` SHA-256: `64844c24f67e7eafb77142b4cbe4a430f4db3b16f59191586ba6596ca19fc775`

Python 3.14 traceback formatting uses this extension to calculate Unicode display
widths. Build rules stage native extensions beside the standard-library zip, and
the embedded interpreter searches that directory as well as the zip itself.
