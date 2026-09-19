[![Linux](https://github.com/christianhelle/sqlquery/actions/workflows/linux.yml/badge.svg)](https://github.com/christianhelle/sqlquery/actions/workflows/linux.yml)
[![MacOS](https://github.com/christianhelle/sqlquery/actions/workflows/macos.yml/badge.svg)](https://github.com/christianhelle/sqlquery/actions/workflows/macos.yml)
[![Windows](https://github.com/christianhelle/sqlquery/actions/workflows/windows.yml/badge.svg)](https://github.com/christianhelle/sqlquery/actions/workflows/windows.yml)

[![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=christianhelle_sqlquery&metric=alert_status)](https://sonarcloud.io/summary/new_code?id=christianhelle_sqlquery)
[![Bugs](https://sonarcloud.io/api/project_badges/measure?project=christianhelle_sqlquery&metric=bugs)](https://sonarcloud.io/summary/new_code?id=christianhelle_sqlquery)
[![Reliability Rating](https://sonarcloud.io/api/project_badges/measure?project=christianhelle_sqlquery&metric=reliability_rating)](https://sonarcloud.io/summary/new_code?id=christianhelle_sqlquery)
[![Security Rating](https://sonarcloud.io/api/project_badges/measure?project=christianhelle_sqlquery&metric=security_rating)](https://sonarcloud.io/summary/new_code?id=christianhelle_sqlquery)
[![Maintainability Rating](https://sonarcloud.io/api/project_badges/measure?project=christianhelle_sqlquery&metric=sqale_rating)](https://sonarcloud.io/summary/new_code?id=christianhelle_sqlquery)
[![Vulnerabilities](https://sonarcloud.io/api/project_badges/measure?project=christianhelle_sqlquery&metric=vulnerabilities)](https://sonarcloud.io/summary/new_code?id=christianhelle_sqlquery)

# SQL Query Analyzer

SQL Query Analyzer is a lightweight and efficient desktop utility designed
to simplify the process of managing SQLite, PostgreSQL, SQL Server and
MySQL/MariaDB databases.

It is a fork of [SQLite Query Analyzer](https://github.com/christianhelle/sqlitequery)
that talks to database servers as well as SQLite files.

It provides an intuitive interface for executing queries and direct table editing,
making database operations seamless and straightforward.

## Features

- Cross platform - Runs natively on Windows, MacOS, and Linux
- Works with SQLite files and PostgreSQL, SQL Server and MySQL/MariaDB servers
- Connect dialog with a Test Connection button; recent connections are remembered
  (passwords never are)
- Schema-aware database tree showing `schema.table` for providers that have schemas
- Easy-to-use interface for executing SQL queries
- Fast table data editing
- Persists application state and reloads last session on startup
- Export database schema as CREATE TABLE statements
- Export data as an SQL script containing INSERT statements or as CSV files
- Zoom the query editor and database tree independently, each with its own size,
  using Ctrl +/- on the focused pane or Ctrl + mouse wheel over it. The query
  result grid, Messages pane and Table Data grid zoom together with the query
  editor, and answer both gestures themselves
- Desktop color theme awareness with automatic switching between dark/light themes
- Command line interface for automation and scripting

## Supported Databases

| Provider | Qt driver | Client library needed at runtime |
|---|---|---|
| SQLite | `QSQLITE` | none, built into Qt |
| PostgreSQL | `QPSQL` | libpq (`libpq5` on Debian/Ubuntu, `libpq.dll` next to the executable on Windows) |
| SQL Server | `QODBC` | unixODBC plus the [Microsoft ODBC Driver 18 for SQL Server](https://learn.microsoft.com/sql/connect/odbc/download-odbc-driver-for-sql-server) (Windows ships the ODBC driver manager) |
| MySQL / MariaDB | `QMYSQL` | libmariadb or libmysqlclient |

The official Qt binaries for Windows and macOS do not include the `QMYSQL` driver,
so MySQL/MariaDB support there needs a Qt build that has it.

Use **File > Connect...** (Ctrl+Shift+O) to open any provider. **File > Open**
and **File > New** still open and create SQLite files directly.

### Connection URLs

The command line, the recent connections list and the saved session all describe a
connection in a single line:

| Provider | Example |
|---|---|
| SQLite | `/path/to/database.db` |
| PostgreSQL | `postgres://user@host:5432/database` |
| SQL Server | `mssql://user@host:1433/database?trust=1` |
| MySQL / MariaDB | `mysql://user@host:3306/database` |

SQL Server takes these extra query options: `trust=1` accepts a self-signed server
certificate, `integrated=1` uses Windows authentication, and `driver=...` names
another ODBC driver. A password is never written out. On the command line, give it
with `--password`, the `SQLQUERY_PASSWORD` environment variable, or inside the URL
(`postgres://user:secret@host/db`).

### Known limitations

- Scripts are split into statements on `;`, so PostgreSQL `$$` function bodies and a
  `;` inside a string literal are not understood yet.
- A SQL Server script that uses `GO` runs one batch at a time, like `sqlcmd`: a line
  holding only `GO` ends a batch, and `GO 5` runs it five times. A batch goes to the
  server whole, so a procedure body keeps its semicolons, but a batch holding several
  `SELECT`s shows only the first result. A script without `GO` is split on `;`.
- The table data grid for PostgreSQL and SQL Server reads the whole result set on
  the client, so paging only happens in the view.
- Views, stored procedures and other databases on the same server are not listed
  in the tree.
- **Shrink** runs `VACUUM` on SQLite and PostgreSQL and `DBCC SHRINKDATABASE` on
  SQL Server. It is disabled for MySQL.

## Installation

### Quick Install

#### Windows (PowerShell)
```pwsh
Invoke-RestMethod https://christianhelle.com/sqlquery/install.ps1 | Invoke-Expression
```

#### Linux / macOS (Bash)
```sh
curl -fsSL https://christianhelle.com/sqlquery/install.sh | bash
```

### Download Pre-built Binaries

Pre-built binaries for all platforms are available on the [GitHub Releases](https://github.com/christianhelle/sqlquery/releases/latest) page.

For more detailed installation instructions, visit the [Documentation Website](https://christianhelle.github.io/sqlquery/).

## CLI Usage

SQL Query Analyzer can be used as a command line tool for automating database operations without the GUI. The application supports several command line options for exporting data, executing SQL scripts, and more.

### Help Text

```sh
$ sqlquery --help
Usage: sqlquery [options] database
A fast and lightweight cross-platform command line and GUI tool for querying and manipulating SQLite, PostgreSQL, SQL Server and MySQL databases

Options:
  -h, --help                          Displays help on commandline options.
  --help-all                          Displays help, including generic Qt
                                      options.
  -v, --version                       Displays version information.
  -p, --progress                      Show progress during copy
  -e, --export-csv                    Export data to CSV.
  -d, --target-directory <directory>  Target directory for export.
  -r, --run-sql <file>                Execute SQL file.
  --password <password>               Password for a server connection.
                                      Defaults to the SQLQUERY_PASSWORD
                                      environment variable.

Arguments:
  database                            SQLite file or connection URL to open,
                                      e.g. postgres://user@host:5432/db,
                                      mysql://user@host/db or
                                      mssql://user@host/db?trust=1
```

### Usage Examples

#### Opening a database in GUI mode
```sh
sqlquery /path/to/database.db

# A server connection opens the Connect dialog to ask for the password,
# unless one is given
SQLQUERY_PASSWORD=secret sqlquery postgres://postgres@localhost/shop
```

#### Working with a server
```sh
# Export every table of a PostgreSQL database to CSV
sqlquery --export-csv -d ./out --password secret postgres://postgres@localhost:5432/shop

# Run a script against SQL Server
SQLQUERY_PASSWORD=secret sqlquery --run-sql seed.sql "mssql://sa@localhost/master?trust=1"
```

#### Exporting data to CSV files
```sh
# Export all tables to CSV files in the current directory
sqlquery --export-csv /path/to/database.db

# Export with progress indicator
sqlquery --export-csv --progress /path/to/database.db

# Export to a specific directory
sqlquery --export-csv --target-directory /path/to/export/folder /path/to/database.db
```

#### Executing SQL scripts
```sh
# Execute a SQL script file against a database
sqlquery --run-sql /path/to/script.sql /path/to/database.db
```

### CLI Features

- **Export to CSV**: Export all database tables to individual CSV files
- **Execute SQL Scripts**: Run SQL scripts from files against a database
- **Progress Reporting**: Show progress indicators for long-running operations
- **Flexible Output**: Specify custom directories for exported files

## Screenshots

Here are some screenshots of SQL Query Analyzer in action:

## Windows

![Insert query](images/windows-query-insert.png)
![Select query](images/windows-query-select.png)
![Table data editing](images/windows-table-data.png)
![Dark Mode Insert query](images/windows-dark-query-insert.png)
![Dark Mode Select query](images/windows-dark-query-select.png)
![Dark Mode Table data editing](images/windows-dark-table-data.png)

## MacOS

![Insert query](images/mac-query-insert.png)
![Select query](images/mac-query-select.png)
![Table data editing](images/mac-table-data.png)
![Dark Mode Insert query](images/mac-dark-query-insert.png)
![Dark Mode Select query](images/mac-dark-query-select.png)
![Dark Mode Table data editing](images/mac-dark-table-data.png)

## Linux (Ubuntu)

![Insert query](images/linux-query-insert.png)
![Select query](images/linux-query-select.png)
![Table data editing](images/linux-table-data.png)
![Dark Mode Insert query](images/linux-dark-query-insert.png)
![Dark Mode Select query](images/linux-dark-query-select.png)
![Dark Mode Table data editing](images/linux-dark-table-data.png)

## Building

### Prerequisites

- Git 😄
- CMake 3.16 or later - Install from [official website](https://cmake.org/download/)
- Qt 6.4.2 or later, 6.11.2 recommended - Install from [official website](https://www.qt.io/download-qt-installer-oss)
- [Powershell Core](https://learn.microsoft.com/en-us/powershell/scripting/install/installing-powershell) (Optional)

### Clone the repository

```sh
git clone https://github.com/christianhelle/sqlquery.git
cd sqlquery
```

### Build the project

Use the Makefile on Linux and macOS, or the cross-platform PowerShell script on Windows:

```sh
# Linux / macOS
make
```

```sh
# Windows
pwsh build.ps1
```

The build output folder is under `build` on all platforms. On Linux, `make` also installs into `./linux/`.

### Building on Linux

Install CMAke and Qt6 and XKB

```sh
sudo apt-get update
sudo apt-get install -y cmake qt6-base-dev libxkbcommon-dev
```

Build project

```sh
make
```

Install the Qt SQL drivers for the servers you want to reach (Ubuntu package names)

```sh
sudo apt-get install -y libqt6sql6-psql libqt6sql6-mysql libqt6sql6-odbc
```

Create installable packages (DEB, RPM, 7Z, ZIP, and compressed archives)

```sh
make package
```

Install a `sqlquery` symlink to `~/.local/bin`

```sh
make install
```

### Building on MacOS

Install CMake and Qt6. It's recommended to install Qt using the [official installer](https://www.qt.io/download-qt-installer-oss)
```sh
brew update
brew install cmake
brew install qt@6
```

Build project

```sh
make
```

Build MacOS disk image (Optional). 

There is a bug in the Homebrew distribution of Qt which causes the use of `macdeployqt` to fail.

```sh
make package
```

### Building on Windows

Build the project (These instructions assumes that Qt root folder is C:\Qt)

```pwsh
cd src
cmake . -DCMAKE_PREFIX_PATH=C:/Qt/6.11.2/msvc2022_64 -DCMAKE_CXX_STANDARD=17 -DCMAKE_CXX_FLAGS="/Zc:__cplusplus /permissive-" -B build
cmake --build build --config Release
C:\Qt\6.11.2\msvc2022_64\bin\windeployqt.exe .\build\Release\SQLQueryAnalyzer.exe
```

Build the installer project using Inno Setup (Optional)

```pwsh
../deps/innosetup/ISCC.exe dist/setup.iss
```

## Testing

The unit tests use in-memory and temporary SQLite databases and need nothing else:

```sh
make test
```

The provider integration tests in `tests/test_providers.cpp` run against real
servers. A provider is skipped unless its connection URL is set:

```sh
docker compose -f tests/docker-compose.yml up -d
export SQLQUERY_TEST_PG='postgres://postgres:SqlQuery_2026@localhost:5432/postgres'
export SQLQUERY_TEST_MYSQL='mysql://root:SqlQuery_2026@127.0.0.1:3306/test'
export SQLQUERY_TEST_MSSQL='mssql://sa:SqlQuery_2026@localhost:1433/master?trust=1'
./build/tests/SQLQueryTests --gtest_filter='Servers/*'
```

The `Tests` GitHub workflow runs both against PostgreSQL, MariaDB and SQL Server
service containers.

## Contributing

We welcome contributions to SQL Query Analyzer!
If you have any ideas, suggestions, or bug reports,
please open an issue or submit a pull request on GitHub.
