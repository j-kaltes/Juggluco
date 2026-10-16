# Health Connect activities

Implemented against Juggluco 11.4.1, commit
`a983788a17a35f8e76d9da5c125996b648b4f27a`.

## Use

Open **Settings → Exchange → Activities** on the mobile build, Android 9 or
later. Choose the initial date and press **Retrieve**. Automatic retrieval is
off initially; the dialog offers intervals from 15 minutes to one day.
Background retrieval uses Android's JobScheduler when Health Connect supports
background reading and the user grants that permission. Android can delay the
job. Foreground retrieval also runs when the interval has elapsed.

Exercise permission is required. The remaining data permissions are optional.
**Permissions** revisits grants, including background and historical access.
The existing glucose-writing switch remains independent.

Without history access, Health Connect limits reads to 30 days before the first
permission grant, even when its own interface shows older data. Selecting an
earlier retrieval date does not override that limit. **Retrieve** also offers
history permission for an old start date when exercise access is already granted.
The dialog reports whether history access is granted, missing or unsupported by
the installed Health Connect version. Granting it restarts the initial read from
the configured date; the saved record IDs prevent duplicates. Retrieval logs the
history feature/grant state, requested date range and returned session date range.

Large imports run Health Connect calls without artificial delays, including every
raw-record page, aggregate and changes request. An API quota error retries that
same request after one second, doubling repeated waits up to five minutes.
Successful requests resume immediately. Waiting uses cancellable
coroutines; the dialog shows progress and a quota-wait message. Permission and
other failures still report an error. This does not change Health Connect quotas
or clip the requested history period. Keeping the app in the foreground generally
allows faster retrieval than background operation.

Raw heart-rate, resting-heart-rate, speed, power and cadence reads share descending
pages across the pending activities. Requests use up to 5,000 records per page.
An empty result covers the entire pending period in one call, and sparse records
from one page can serve many activities. Dense reads stop after covering the
current activity and seek over gaps instead of reading unrelated daily records.
Only records needed by remaining activities and the unread tail of one page stay
in memory; overlapping or containing activities retain their required records.
The cache is discarded between imports so later corrections and deletions are
re-read. Per-activity aggregate requests retain Health Connect's totals semantics.
Completed activities remain saved, and an unchanged permission dialog no longer
resets import cursors. Actual permission changes still trigger a fresh read.
An interrupted import finishes its saved changes and pending details before
requesting newer changes for those record types. Otherwise a new measurement
could mark already completed activities pending again on every restart.

**Create Amounts from activities** is off by default, including when upgrading
from the earlier activity implementation. **Retrieve** stores activity details;
automatic linking and creation of Amounts require explicitly enabling this
checkbox and pressing **Save** or **Retrieve**. Opening the app or granting
Health Connect permissions does not enable it.

**Labels** maps each Health Connect exercise type to an Amount label and
measure. After Amount creation is enabled, unconfigured types use a general
`Activity` label and elapsed minutes. A mapping can choose an existing eligible label or create a specific
one. Blood glucose, meal and insulin labels are excluded because they already
have other semantics. Labels retain the existing limit of 11 UTF-8 bytes and
the existing total label limit.
The new-label field is single-line; the keyboard **Done** action saves the label.

An unlinked Amount with the mapped label receives the activity reference while
keeping its timestamp, value and label. Matching includes ten minutes before
and after the session, covering minute-rounded times and delayed watch starts.
If nearby activities compete for an entry, the closest session interval wins;
entries inside an interval have distance zero. Ties and multiple candidate
Amounts stop automatic linking. For an unlinked activity, **Link to amount**
lets the user choose from the expanded interval. With no match, a new Amount is
created when the chosen measure is available.

On resume, an untouched generated Amount is also checked for an existing manual
entry in that window. A unique match keeps the manual entry's original time,
label and value and removes the generated duplicate. The surviving link is
marked as manual, so later Health Connect corrections preserve those values.
Individually edited imported Amounts are retained. This repair requires **Create
Amounts from activities** and waits for the Amount editor to close; it also
honors **Mirror sends amounts**. No new Health Connect download is required.

Long-press an Amount and press **Activity** in the existing editor to inspect
the original activity information. Its normal label/value/time controls make
individual edits. **Update label and measure of existing imported amounts** is
checked by default: saving a mapping updates both fields of previously generated
Amounts and refreshes the open Amount editor. It preserves individual overrides,
unsaved editor changes and manually entered Amounts. Uncheck it to apply the
mapping only to future Amounts. If a new measure is unavailable, the existing
Amount keeps its old label and value. Source corrections use each Amount's saved
measure and update unedited generated times.
Deleting a linked Amount also removes its whole activity group from saved
activities, details and exports. **Saved activities → activity → Delete** does
the same without requiring an Amount. Local removal is remembered across
retrievals, corrections, overlapping copies and restarts; Health Connect is not
modified. The first reconciliation after upgrading also removes activities
whose linked Amounts were already deleted before this fix.

After retrieval reports a Health Connect deletion, that source disappears from
details and exports, even if it was the only source. The merged activity remains
while at least one source survives. When no sources remain, its unedited
generated Amounts are removed. Manually entered or individually edited Amounts
are kept without the activity link. Local activity deletion uses the same Amount
cleanup; deleting an Amount itself still deletes that selected Amount normally.
Cleanup also runs with **Create Amounts** off and waits for the Amount editor to
close before changing numbers. Deletion logs identify affected sources/groups
and distinguish removed generated Amounts from detached manual Amounts.

Generated Amounts are rounded to one decimal place (whole numbers for steps);
the original activity measurements retain their precision. **Distance and speed
units** in the activity dialog selects km/km/h (the default) or mi/mi/h for
details. This is an explicit saved choice, independent of phone region and
Health Connect's own display preferences.
Elapsed duration and HR coverage use `hours:min:secs`, with a days component when
needed. Amount mappings explicitly offer both distance and speed units; changing
the detail display unit does not change existing Amount values or their saved units.

Activity details use a larger bold activity type and bold app headings ending
in a colon. The time range includes duration; distance and mean speed follow
together, then steps, energy, elevation, power and cadence. Units follow their
values, such as `16.9 km/h`. Phone names and numeric device-type tags are omitted.
Named watches are shown only when needed to distinguish devices from one app.
The matching HR source is displayed first without repeating the activity app's
heading. Other HR sources each have a heading; a cross-app HR source is identified
even if it supplies the primary summary. Each section shows its own available
resting HR, mean, mean-minus-resting HR, SD and coverage, once each.

Activity dialog failures log the operation and complete exception through
`Log.stack`, before showing the FullToast. Look for `ActivitySettings`,
`ActivityHealth` and `NumberView` in the logs. Retrieval also logs the record type,
read/change counts, pending details, permission skips, expired cursors and job
scheduling. It does not log cursor contents or activity notes. As elsewhere in
Juggluco, logging builds write these to the app log; `Log.stack` falls back to
Android logcat when app logging is disabled.

## Export

**Left middle menu → Export → Activities** saves a standard TSV with a header.
Its columns, in order, are Record, Start, Duration (d:h:m:s), Activity,
Distance (km), Mean speed (km/h), Steps, Resting HR (bpm), Mean HR (bpm),
HR SD (bpm), Mean-resting HR (bpm), and Active energy (kcal). It has one row per
source activity record and uses that recording's saved HR summary. The HR
sources are never averaged together. The distance/speed headings and values
use miles instead if selected in the activity settings.

Check **Extended**, next to the Activities button, for the previous full table
including all HR sources and metadata. Extended starts unchecked. Extended
filenames include `-extended`; both file formats always contain the header.
The existing **Days** field selects the period ending at the displayed graph's end
(limited to the current time), as with other exports.

The webserver supplies the same TSV at `x/activities`. There is no column header
unless `header` is present: `x/activities?header`. Add `extended` for the full
table, for example `x/activities?header&extended` (with a header) or
`x/activities?extended` (without one). Existing date parameters work,
for example `x/activities?header&days=30` or
`x/activities?header&start=2026-10-10&end=2026-10-11`. The default web period is
the last five hours, matching the other data endpoints. Existing webserver
access controls apply. Export reads the locally saved activities without
requesting Health Connect data or enabling Amount creation.

In the extended table, each source activity record has one row per HR
app/device, or one row with empty HR fields when no HR is available.
`Group` identifies the merged activity;
`Record` and `Health Connect ID` identify its individual recordings. Activity
fields repeat on HR rows; do not sum those repeated distances or durations.
`Summary HR` is `1` for the HR source used in the activity summary and `0` for
other sources. Each row's resting HR and mean-minus-resting HR use that same
source's baseline; unavailable values stay empty. No sources are averaged.
These are session summaries, not an export of the individual raw samples.

Records overlapping the requested interval are included with their complete
session statistics. Rows are chronological; `maxcount` limits source activity
records, retaining all HR sources in extended mode. Locally removed activities
and all sources confirmed deleted in Health Connect are omitted using the same
rule as the details dialog. The extended status column marks
cached records omitted by Health Connect as `not returned`, and incomplete
details as `details pending`.

Columns contain explicit units. Distance and speed follow the saved activity
unit setting. Times include the UTC offset, duration/coverage use
`[days:]hours:minutes:seconds`, and missing measurements are empty rather than
zero. Numeric values use a decimal point independently of locale.

There is exactly one tab between columns. Cells are padded with spaces to the
widest header/value in the selected output, using Unicode terminal character
widths, so headers and data align in a monospace viewer. Headerless output uses
the same widths. TSV readers should trim padding spaces. Backslashes, tabs,
newlines and carriage returns in text become `\\`, `\t`, `\n` and `\r`;
other control/invalid bytes become `\xHH`. No text is truncated.

## Native data

`activities.dat` holds a fixed header, mappings, cursors and fixed-size records.
`activity-data.dat` holds append-only strings, source metadata and sample arrays.
Both files use `MAP_SHARED`; no serialization or explicit save/load cycle is
needed at startup. Records and slices contain fixed-width integers and offsets,
never process pointers, containers or mutexes. The mutex is outside the mapping.
Layouts are asserted for little-endian Linux with 32-bit IEEE floats.

Payloads are written before their slices or new-record count are published.
Dirty flags precede pending retrieval cursors; completed cursors follow detail
processing and Amount linking. The pending cursor lets an interrupted or
quota-limited import resume its unfinished details. Bounds checks reject missing
payloads. These are ordinary mmap writes: assignment order does not guarantee
physical disk write ordering. Rare power loss can lose recent data; this code
does not add synchronous flushes or a transaction journal.

Health Connect record ID plus origin identifies an imported session. Changes
tokens handle late uploads and corrections; an expired token triggers a fresh
read and deduplication. Replaying unchanged metadata retains completed work.
Records omitted from a fresh query retain their cached details; a narrowed date
range or revoked history access therefore does not replace old samples with empty
results.
Metric changes invalidate saved details, including changes whose time interval
has moved and deletions that supply only an ID. A changed initial date invalidates
cursors. A configuration generation prevents an in-flight import from committing
cursors for the previous date range.

Upgrading from the importer that filtered heart rate by session app marks saved
activities for one details refresh at the next retrieval. A version field in
previously unused header space records this after the pending flags are written.
Record layouts, saved samples, cursors, Amount references and user edits remain
intact; completed refreshes are not repeated on every app restart.

Overlapping exercise intervals share an activity group, even with different end
times, exercise types or origins. A longer recording can join several shorter
segments; merely touching end/start times do not merge. Every source record and
its original measurements remain stored separately. A source explicitly deleted
in Health Connect is removed from details, exports and the activity list even
without another overlapping source. Its stored ID and Amount/Kerfstok references
remain valid as tombstones. Sources merely omitted from a query remain visible;
a missing result alone does not prove deletion. Reopening the details applies
this to sources already marked deleted; detecting new Health Connect deletions
requires retrieval. The longest current
recording supplies the group's displayed type and the type used by **Labels**.
For an Amount, the longest completed recording with the chosen measure supplies
the value; a positive reported distance takes precedence over zero-only fragments.
Distances and other duplicate measurements are not summed or averaged. Duration
counts the union of current intervals once, excluding any gaps, and generated
Amounts use the group's earliest current start time.

On resume or retrieval, cached overlaps from the previous matcher are merged too.
This includes deleted or omitted source records whose old Amounts still exist.
With Amount creation enabled, untouched generated duplicates are removed through
the normal number deletion path, including its backup/Kerfstok notifications.
A manually entered or individually edited Amount takes precedence over generated
duplicates; if there are several user-owned Amounts, all are preserved. Cleanup
waits until the Amount editor closes. Once linked, groups are retained through
source corrections and deletions; they are not automatically split again.

The oldest group index remains canonical. Original references still resolve to
the merged group, including references returned by Kerfstok. Each surviving
Amount keeps its original `mealptr` bits and its own generation/override baseline.
Parent links and deletion history are written before duplicate Amounts are removed;
an interrupted cleanup can be repeated. No record, sample or mmap layout changes
are needed for overlap merging. Local deletion adds a record flag and an upgrade
marker in previously unused header space, without moving records or changing
their size. Duplicate consolidation explicitly bypasses local activity removal;
deleting a duplicate Amount must not delete the surviving group. Native logs identify session intervals/groups and removed duplicates
under `Activity:`.

The saved fields are session ID, origin, device description, title, notes,
type, start/end, modification time, aggregate distance, active calories, steps,
elevation, and original timestamped HR, resting HR, speed, power and cadence
samples. Distance, energy, steps, elevation, speed, power and cadence are read
from the session's origin. Heart rate and resting heart rate are read from all
origins: a Strava session can therefore use Samsung Health watch measurements.
Raw queries filter by record start, so HR reads include a 24-hour lead-in to
include earlier-starting workout and daily batches. A record starting more than
24 hours before the activity is outside this read window.
Only heart-rate samples inside the session interval are retained, even when the
containing Health Connect record extends past the session end.
Samples from distinguishable devices retain separate source IDs. The HR summary
prefers the exact session source, then a matching nonempty device description,
then the session's app. Within each preference it uses the source with the most
time coverage, also used when none matches. It never averages sources together.
The details identify the source used for the summary and list the other sources
separately, ordered by the same device/app preference as the primary summary.
Resting HR uses only that HR source's own records; an unavailable matching
baseline remains missing. Changing the selected source recomputes that baseline.
Alternative HR sections now also display their own available resting baseline
and mean-minus-resting HR. The importer already reads `RestingHeartRateRecord`
from all apps when permission is granted. It can only show Samsung's resting HR
if Samsung exports those records to Health Connect with matching source metadata;
it does not estimate Samsung's baseline from exercise HR or borrow Garmin's.
Samsung's published synchronization table lists `HeartRateRecord` but not
`RestingHeartRateRecord`:
https://developer.samsung.com/health/blog/en/accessing-samsung-health-data-through-health-connect
Health Connect metadata cannot distinguish devices that supply identical or missing device
descriptions. Heart-rate summaries use time weights and cap a sample's carried
coverage at two minutes. Gaps remain uncovered. Resting HR uses the median of
the preceding seven days from the selected source.

`Num` is still 16 bytes. Bit 31 of its existing `mealptr` field tags an activity
index for non-blood Amounts. Its complete 32-bit value still goes to Kerfstok
and returns unchanged. Kerfstok does not receive the activity payload. Meal
rendering, matching and export skip these references. Activity payloads currently
stay on the importing Juggluco device. Each local activity link records the
number database identity, base and exact Amount in the activity file. A copied
activity index alone never selects local details or causes local deletion.
Local edits update the binding; Health Connect corrections preserve it. This
uses reserved record/header space and a payload slice without changing either
the activity record layout or the 16-byte number sent to mirrors and Kerfstok.

**Mirror sends amounts** disables activity Amount creation and automatic changes
while it is checked. Disabling it allows local linking even if the number
database's historical `receivedbackup` flag is still set. That flag is used for
synchronization and is not a current write-permission setting. When **Create
Amounts from activities** is enabled, returning to Juggluco links completed
saved activities without downloading them again. A single copied Amount with
the mapped label in the matching window can be adopted by that time/label match;
its original value is preserved. Multiple candidates remain ambiguous.
Native logs report the saved-group and linked-Amount counts, or explain that
the current mirror setting blocks linking.

Linked Amount lookup uses the saved binding's timestamp to search the sorted
number database. It does not scan the whole Amount history for each activity.
Edited timestamps and merged-source aliases retain their exact bindings and
the original database/position preference. Ordinary and unbound Amounts are
rejected before calling the logging `getindex()` getter; this avoids the
millions of log writes that could block the UI during linking on resume.

On upgrade, existing local links are registered once. A database whose
`receivedbackup` flag is set requires an exact stored generated baseline for
that legacy registration, so a coinciding foreign index alone is insufficient.

## Validation

Run the included Linux test from the repository root with a C++26 compiler:

```sh
clang++ -std=c++26 -pthread -O1 -g -fsanitize=undefined \
  Common/src/main/cpp/activity/tests/store_test.cpp -o /tmp/activity-store-test
/tmp/activity-store-test
clang++ -std=c++26 -pthread -O1 -g -fsanitize=undefined \
  Common/src/main/cpp/activity/tests/amount_test.cpp -o /tmp/activity-amount-test
/tmp/activity-amount-test
clang++ -std=c++26 -pthread -O1 -g -fsanitize=undefined \
  Common/src/main/cpp/activity/tests/amount_binding_test.cpp -o /tmp/activity-binding-test
/tmp/activity-binding-test
clang++ -std=c++26 -pthread -O1 -g -fsanitize=undefined \
  Common/src/main/cpp/activity/tests/heart_sources_test.cpp -o /tmp/activity-heart-test
/tmp/activity-heart-test
clang++ -std=c++26 -pthread -O1 -g -fsanitize=undefined \
  Common/src/main/cpp/activity/tests/merge_test.cpp -o /tmp/activity-merge-test
/tmp/activity-merge-test
clang++ -std=c++26 -pthread -O1 -g -fsanitize=undefined \
  Common/src/main/cpp/activity/tests/export_test.cpp -o /tmp/activity-export-test
/tmp/activity-export-test
clang++ -std=c++26 -pthread -O1 -g -fsanitize=undefined \
  Common/src/main/cpp/activity/tests/deletion_test.cpp -o /tmp/activity-deletion-test
/tmp/activity-deletion-test
CXX=clang++ python3 Common/src/main/cpp/activity/tests/amount_lookup_test.py
CXX=clang++ python3 Common/src/main/cpp/activity/tests/amount_match_test.py
```

The export regression checks the exact standard columns/order, one summary row
per recording, both unit settings, and the extended format's columns, including maximum HR.
It also checks multiple HR sources and their separate baselines,
merged/deleted recordings, empty values, date boundaries, units, headers,
Unicode and control characters, four/eight-column tab stops, and byte-for-byte
unchanged mmap files after export.

The deletion regression covers final-source deletion, surviving merged sources,
both export formats, retained manual edits, one-time recovery of old Amount
deletions, alias references, repeated/revised imports, overlapping copies, and
reopening the mmap files. Cached records missing from a read remain visible.

The Amount binding regression checks database/base isolation, coinciding foreign
activity indices, local edits, explicit detachment, Health Connect updates and
reopening the saved bindings.

The lookup regression compiles `ownAmount` and `findNumber` directly from the
JNI adapter against 42,000 ordinary Amounts and 559 activity groups. It bounds
getter calls and Amount visits, and checks copied IDs, merged/deleted aliases,
timestamp overrides, equal timestamps and shifted positions. The test counts
operations instead of depending on machine speed or writing millions of logs.

The matching regression compiles the production association, edit and deletion
functions with fake number I/O. It covers minute rounding, delayed starts,
repair of an already generated duplicate, preservation of manual values after
source corrections/deletion, edited imported Amounts, ambiguity between entries
and sessions, mirrored references and the current mirror setting.

The test covers reopen without serialization, mapping growth, exact-ID replay,
source grouping, source separation, weighted statistics and gaps,
zero-valued speed, resting HR, pending-cursor recovery, unavailable slices and
the unchanged Num representation. The Amount regression test covers the reported
7,134-second / 29,562.3-metre bike ride, mapping changes, source selection,
rounding, missing measures, individual overrides, subsequent retrieval and
metric/imperial conversions. `Common/src/test/java/tk/glucodata/ActivityDisplayTest.java`
contains plain JVM checks for the actual dialog formatting functions.
`Common/src/testMobile/java/tk/glucodata/ActivityTypesTest.java` checks exercise
codes and names against connect-client's own map. The production lookup filters
public static final integer fields; the SDK's `EXERCISE_TYPE_*_MAP` fields must
never be passed to `Field.getInt()`.
The heart-source regression covers a Strava activity with Samsung Health HR,
interval clipping, multiple watches, source preference, matching resting HR,
source removal, ordered per-source summaries with independent baselines, and a
resumable upgrade without changing IDs or cursors. The JVM formatting checks also
cover trailing units, locales, device-tag removal, hidden phone names and
distinguishing watches from one app.
The merge regression uses the Strava/Garmin/Fit intervals from the reported log,
including shorter zero-distance and deleted Fit records. It checks cached duplicate
repair, same-app and differing-type overlaps, later bridging records, touching
intervals, source corrections, duration without double counting, manual overrides,
permanent reference aliases and mmap reopen without cursor changes.

```sh
javac -d /tmp/activity-format-check \
  Common/src/mobile/java/tk/glucodata/ActivityFormat.java \
  Common/src/test/java/tk/glucodata/ActivityDisplayTest.java
java -cp /tmp/activity-format-check tk.glucodata.ActivityDisplayTest
```

During development this passed with Clang 22 in C++26 mode. The native JNI
translation unit compiled in GNU C++26 mode, including `JUGGLUCO_APP` and the
project's exception/RTTI settings. The storage test also compiled for 32-bit ARM
to check the mapped layouts. The Kotlin adapter and permission callback compiled
against connect-client 1.1.0-rc03, matching the project dependency; the Java
dialogs compiled against Android APIs. These focused Android checks used small
stubs for existing app classes.

A complete Gradle/NDK APK build and on-device Health Connect/Kerfstok testing have
not been performed in this environment. In particular, check permission denial,
late uploads, process restart during retrieval, label edits, actual Kerfstok
round-tripping and background scheduling on a phone before release.

`Common/src/testMobile/java/tk/glucodata/ActivityReadPolicyTest.kt` checks quota
retry, cancellation and the absence of artificial request delays with a virtual
clock. `ActivityReadPagesTest.kt` compares shared pages with independent raw
queries, including empty/sparse histories, containing activities, tied timestamps
across page boundaries, skipped date gaps and interrupted pages. Both are plain
JVM entry points requiring Kotlin stdlib and kotlinx-coroutines-core; compile
them together with `ActivityReadRetry.kt` and `ActivityReadPages.kt`.


## Filtering, source removal and mirrors

Completed recordings with no nonzero measurements, samples, title or notes are
excluded when their mapped Amount measure is not duration. An existing generated
duration Amount also keeps its recording available. Pending detail reads are not
classified as empty. A recording from another app is excluded when its interval
is contained in, or within one minute of, a richer recording in its group and
its measurements/text/samples add no information. Different measurements and
additional sample sources are retained. Numeric comparisons use display precision;
raw samples are compared by source, timestamp and value. Filtering retains original
record IDs and bindings. Generated zero summaries for excluded groups are removed;
manual Amounts are detached and kept. Changing the mapping can expose a duration-only
recording again.

Remove source in the mobile details dialog excludes the chosen app/device from
that activity, including its session and HR data. The group-specific exclusion is
stored independently of Health Connect deletion, survives source corrections and
re-imports, and is respected by source selection and both exports. Other recordings
can become visible when the richer source is removed. Maximum HR is taken from
positive samples within each activity interval, separately for every HR source.
It is displayed, exported and available as an Amount mapping. Its persisted float
uses a previously reserved record word; records remain 184 bytes and existing
field/metric indices remain unchanged.

The real ActivitySettings, ActivityNative and ActivityFormat classes are in mobile.
Wear and small provide empty shared editor/settings hooks, and their native builds
do not include the activity JNI adapter. The storage/mirror backend remains available
for cloning and forwarding Amounts with activity references.

updatenums calls updateactivities after Amounts and meals. Each peer's cursor is
changednums::activityState, a 20-byte Cursor packed to four-byte alignment. Its
member offsets are unchanged from the earlier 24-byte cursor; the old unused word
joins the remaining eight genuinely reserved bytes. Static assertions preserve
changednums's original 840-byte size and the offset of lastlastpos. The previously
reserved Record words holding an AmountBinding slice are now Record::amountBinding;
the 184-byte record and existing binding bytes remain compatible. Activity records,
source metadata, samples, mappings, exclusions, permanent aliases and exact Amount
bindings use the existing acknowledged file-once transport. Initial sync and Reinit
send a full activity image. Later changes send the records and appended data;
unchanged state sends nothing. Network waits never hold the activity mutex.
The receiver stages and validates the image before refreshing its mappings, then
rebases exact Amount signatures to its local databases. Activity and Amount packets
may arrive in either order. Failed incremental sends reset the peer cursor and retry
with a full image. Health Connect cursors/permissions are not cloned.

ICE sends at most 1 MiB per transport block in both directions. Command/file and
encryption formats are unchanged; the existing receive stream joins partial reads.
A block has at most 4,096 packet acknowledgement slots (4,882,432 bytes). The phone
trace from 12 October showed a 43,793,080-byte activity command; sending that as
one block exceeded this window. ICE now rejects oversized blocks and out-of-range
or malformed DATA/ACK packets, and acknowledgement advancement is bounded. The
activity mutex is still released for the whole network wait. The logs do not
contain thread stacks that establish the cause of a UI ANR.

staticnum blocks Health Connect reads in manual, resume, timer and JobScheduler
paths. Changing the mirror checkbox cancels scheduled retrieval. The retry guard
stops a running import and the native import entry points reject late writes after
that setting changes. Viewing and receiving mirrored activities remain available.

Additional regression entry points: filter_sources_test.cpp, mirror_test.cpp and
mirror_adapter_test.py. The last compiles the production updatenums/transfer/binding
functions with fake network I/O and 42,000 ordinary Amounts plus 559 activity links.
It checks send order, exact clone ownership, late HR updates without Amount changes,
incremental fallback and release of the activity mutex during network waits.
layout_test.py checks actual changednums offsets, prior saved cursor bytes and clear.
ice_stream_test.py exercises the production ICE writes and packet bounds with fake
I/O: a valid 43.8 MB activity image, both directions, interrupted sends and invalid
packet/ACK indices. The receiver validates and installs the reconstructed image.

Measurement change events now invalidate only overlapping saved sessions. HR
changes can affect sessions from any app; steps, distance, calories, elevation,
speed, power and cadence affect only the session app used by its detail queries.
Resting HR affects the seven days of subsequent session starts that include it in
their baseline. Filtered sessions remain eligible for new information. Removed
sources/groups and inaccessible snapshot entries are excluded.

The mobile activity-changes.db stores Health Connect record IDs, types, intervals
and app origins. Its generation follows the native permission/date configuration,
and it is not part of activity mirroring. Each granted measurement type is indexed
once before its first change batch. Initial/upgrade indexing refreshes cached
details once; ordinary retrieval does not repeat that historical read. Corrected
records refresh both their former and current intervals/apps. Known deletion IDs
refresh their saved interval; an unknown deletion ID conservatively refreshes all
readable saved details rather than leaving stale measurements behind. Exercise
session changes/deletions continue to use their own IDs and saved change cursor.

Index snapshots use short SQLite transactions per page, never across a suspending
API call. An incomplete snapshot is rebuilt on resume. Native dirty flags precede
index completion and pending cursor publication, including the one-time upgrade
refresh. Changes still checkpoint before reading details, so completed details do
not repeat after interruption. Changing data received during a long import is
checked against the current clock rather than the import's original start time.

changes_test.cpp covers 871 sessions with one steps update, unrelated sources,
HR/resting windows, moved measurements, exclusions and checkpoint/reopen.
ActivityRecordChangesTest.kt tests the production change dispatcher with saved
intervals, source/time corrections, deletions, new records and resume. Compile it
together with mobile ActivityRecordChanges.kt using the Kotlin compiler, then run
tk.glucodata.ActivityRecordChangesTestKt.
