# Working on this project

## Branching

- Never push directly to `main`.
- Developer A works on `feature/database-reports`.
- Developer B works on `feature/cost-analysis`.
- Merge through a reviewed pull request, using `--no-ff` so the branch history
  stays visible.

## Commit messages

`type(scope): summary` — for example `feat(services): add cost per unit`.

Types in use: `feat`, `fix`, `docs`, `chore`.

## Code rules

- Only `reports/` prints. Services return data, the database returns rows.
- Every query that carries a value uses a prepared statement with bound
  parameters, never string concatenation.
- The SQLite handle is owned by `DatabaseManager` and closed in its destructor.

## Daily update

Post a short note every evening: what was written today, and whether anything
is blocked.
