# Supabase Integration

**Document Type**: Integration Guide
**Version**: 2.0.1
**Last Updated**: 2025-11-14
**Status**: Approved
**Author**: NeonGlyph Development Team

## Overview

This project supports optional cloud logging of audio analysis metrics to Supabase via REST. It is disabled by default and activated only when environment variables are defined.

## Environment Variables

- `SUPABASE_ENABLE` = `1` to enable
- `SUPABASE_URL` = `https://<project>.supabase.co`
- `SUPABASE_ANON_KEY` = anon key
- `SUPABASE_TABLE` = target table, e.g. `audio_metrics`

Set for the current PowerShell session:

```
$env:SUPABASE_ENABLE="1"
$env:SUPABASE_URL="https://YOUR.supabase.co"
$env:SUPABASE_ANON_KEY="YOUR_ANON_KEY"
$env:SUPABASE_TABLE="audio_metrics"
```

## Database Schema

Create a table for metrics:

```
create table public.audio_metrics (
  id bigint generated always as identity primary key,
  timestamp_ms bigint not null,
  bpm integer,
  key text,
  energy double precision,
  peak double precision,
  rms_level_db double precision,
  section text,
  spectral_centroid double precision,
  created_at timestamp with time zone default now()
);

create index on public.audio_metrics (timestamp_ms);
```

## Permissions

Enable REST and allow `insert` for `anon` via Row Level Security policy:

```
alter table public.audio_metrics enable row level security;

create policy "allow insert anon"
  on public.audio_metrics
  for insert
  to anon
  with check (true);
```

## Client Behavior

- Logs a single summary record after capture completes in `AudioTest`.
- Uses headers: `Content-Type: application/json`, `apikey`, `Authorization: Bearer`, `Prefer: return=minimal`.
- Runs only when all environment variables are present.

## Verify REST

Test with curl:

```
curl -X POST \
  "$env:SUPABASE_URL/rest/v1/$env:SUPABASE_TABLE" \
  -H "Content-Type: application/json" \
  -H "apikey: $env:SUPABASE_ANON_KEY" \
  -H "Authorization: Bearer $env:SUPABASE_ANON_KEY" \
  -H "Prefer: return=minimal" \
  -d '{"timestamp_ms": 0, "energy": 0.0}'
```

## Build and Run

Reconfigure and build to include `AudioTest` target:

```
cmake -S . -B build64 -G "Visual Studio 17 2022" -A x64
cmake --build build64 --config Release --target AudioTest
```

Run with environment variables set.

Alternatively, use `build.bat` to generate the default `build` directory and then select the `AudioTest` target in Visual Studio.

## Security Notes

- Keep keys out of source code and logs.
- Prefer anon key for client-side logging; use service role key only on servers.

## References

- [Documentation Index](../.trae/documents/DOCUMENTATION_INDEX.md)
- [Build Guide](../.trae/documents/build-guide.md)

## Change History

### Version 2.0.1 (2025-11-14)
- Added metadata header and references
- Clarified build instructions to align with project build scripts

