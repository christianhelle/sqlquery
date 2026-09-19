# CONTEXT

Domain vocabulary for the SQL Query Analyzer codebase. Use these terms in
discussions, code, and documentation. The aim is shared language so the
architecture, the modules, and the tests all refer to the same things.

## Core domain objects

- **Provider** — the database engine behind a Connection: SQLite,
  PostgreSQL, SQL Server or MySQL (MariaDB included).
- **Connection** — how to reach a Database: a Provider plus a file (SQLite)
  or a host, port, database and user (servers), and SQL Server options.
  Written as a single line by `ConnectionInfo::toUrl` -- a bare path for
  SQLite, `postgres://`, `mssql://` or `mysql://` URLs otherwise. The
  password travels with a Connection to open it but is never written out.
- **Dialect** — what a Provider changes about SQL text: how Identifiers and
  literals are delimited, how a SELECT is capped, and which catalog queries
  read the Schema. `SqlDialect` holds all of it, so no other module knows
  which Provider it is talking to.
- **Database** — an open Connection. The thing the user is currently looking
  at. Has a Connection, may be open or closed, owns a single Qt SQL connection.
  Whoever opens a Database keeps it open. A PagedResult reads its later pages
  through that same connection, so closing one out from under live results
  cuts them short and Qt reports nothing.
- **Schema** — the set of tables, columns, and indexes inside a Database.
  Always filtered to user-visible objects (excludes SQLite's `sqlite_*`
  tables and each server's system catalogs).
- **Table** — a named relation in the Schema. Owns a list of Columns and
  Indexes. On PostgreSQL and SQL Server a Table also names the namespace it
  lives in (`public`, `dbo`); on SQLite and MySQL that is empty.
- **Column** — a typed field of a Table (name, declared type, nullability,
  default, primary-key flag).
- **Index** — a secondary access path over a Table (name, column, unique).
- **Identifier** — the name of a Table or Column as it is written into SQL.
  Always delimited, never interpolated raw: a name may hold spaces, a
  reserved word, or a double quote, and only the delimited form survives all
  three. Each Dialect knows its own rule: `"..."` for SQLite and
  PostgreSQL, `[...]` for SQL Server, `` `...` `` for MySQL.
- **DatabaseInfo** — a snapshot of a Schema plus metadata: the Provider,
  the file's name, size and creation date for SQLite, or the server,
  database, engine version and size for a server. Produced by the
  Analyzer; consumed by the Tree and the Exporter.

## User-facing actions

- **Query** — a single SQL statement the user wrote and wants executed.
- **Script** — a sequence of Queries loaded from a `.sql` file.
- **Export** — a one-shot transformation of Database content into a different
  representation. Two formats: **CSV** (one file per table) and **SQL**
  (single script of `INSERT` statements).
- **Recent Connections** — the Connections the user opened lately, kept as
  their single-line form. A file that no longer exists drops out.
- **Session** — the user's last-opened Connection, the text they had in the
  query editor, and the last folder they exported to. Persisted between runs.
- **Zoom** — a scale factor the user applies to the text of a pane, in discrete
  steps away from the size that pane was built with. Step 0 is the untouched
  size. There are two: one that the query editor shares with every pane that
  renders data -- the result Messages pane, the result views, the Table Data
  grid -- and one that the Tree carries, so either side can be enlarged
  without the other. Part of the window state, so both are persisted between
  runs.

## Modules (current shape)

- **Analyzer** — reads a Database, produces a DatabaseInfo.
- **Tree** — renders a DatabaseInfo into the left-hand QTreeWidget.
- **QueryExecutor** — runs a list of Queries against a Database, returns
  results. Pure logic, no widgets.
- **PagedResult** — a lazily fetched view over a Query's result set. Rows are
  read in pages as they are scrolled into view, so a result set of any size can
  be browsed without holding it in memory. Ordering is done by the Database,
  not over the rows already fetched.
- **QueryResultPresenter** — renders QueryExecutor output into the result
  area of the main window. Owns the scroll area and table views. Builds them
  fresh on every run, so whoever wants to do something to them -- apply a
  Zoom, say -- re-reads them after each one.
- **SchemaExporter** — produces a `CREATE TABLE` script from a DatabaseInfo.
- **DataExporter** — produces CSV files or an `INSERT` script from a
  Database + DatabaseInfo. Long-running; reports progress; cancellable.
- **ExportOrchestrator** — runs a DataExporter in the background, marshals
  progress to the GUI thread, surfaces cancel and completion.
- **ZoomPresenter** — owns one Zoom and applies it to the widgets registered
  with it, turning the zoom gestures over those widgets into steps. MainWindow
  keeps one per independently zoomable group of panes. Ctrl+wheel needs no
  routing because a presenter only watches its own widgets; a keyboard zoom
  goes to the presenter that holds the focus.
- **SessionManager** — persists and restores the Session and window state.
- **MainWindow** — Qt shell. Wires the modules to menu actions and the UI
  form. Does not contain business logic.

## Seams

- **IDatabase** — abstract Database. Production adapter is
  `ProviderDatabase`, which reaches any Provider; test adapter is
  `InMemoryDatabase`. Anything that takes a Database takes
  `IDatabase*`. `QSqlDatabase` does not leak across this seam.
- **SqlDialect** — abstract Dialect. Four adapters, one per Provider
  (`SqliteDialect`, `PostgresDialect`, `SqlServerDialect`, `MySqlDialect`).
  Pure string logic, so each is tested without a server.
- **ExportStrategy** — abstract DataExporter workflow (`SqlExportStrategy`,
  `CsvExportStrategy`). Two adapters = real seam.

## Out of scope (do not introduce)

- **Component / Service / API / Boundary** — banned architectural words.
  Use *Module*, *Adapter*, *Seam* instead.
