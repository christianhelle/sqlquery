# PostgreSQL client libraries (Windows)

`libpq.dll` and the DLLs it depends on, shipped next to `SQLQueryAnalyzer.exe`
so the Qt `QPSQL` driver loads without a local PostgreSQL install. Without
every DLL here, Qt reports "Driver not loaded".

Taken from the EDB PostgreSQL 17.11 Windows x64 installer (`bin/`):

| DLL                   | Needed by   | Library                     |
|-----------------------|-------------|-----------------------------|
| `libpq.dll`           | `qsqlpsql`  | PostgreSQL 17.11 client     |
| `libssl-3-x64.dll`    | `libpq`     | OpenSSL 3.5.8               |
| `libcrypto-3-x64.dll` | `libpq`     | OpenSSL 3.5.8               |
| `libintl-9.dll`       | `libpq`     | gettext                     |
| `libiconv-2.dll`      | `libintl`   | libiconv                    |
| `libwinpthread-1.dll` | `libintl`   | mingw-w64 winpthreads       |

Licenses are in [`LICENSES.txt`](./LICENSES.txt).

To update, copy the same files from a newer PostgreSQL install's `bin/` and
check the import list still matches:

```sh
objdump -p libpq.dll | grep "DLL Name"
```
