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

**Labels** maps each Health Connect exercise type to an Amount label and
measure. Initially, imports use a new general `Activity` label and elapsed
minutes. A mapping can choose an existing eligible label or create a specific
one. Blood glucose, meal and insulin labels are excluded because they already
have other semantics. Labels retain the existing limit of 11 UTF-8 bytes and
the existing total label limit.

An unlinked Amount with the mapped label inside the session interval receives
the activity reference while keeping its timestamp, value and label. More than
one match requires the user to choose **Link to amount**. With no match, a new
Amount is created when the chosen measure is available.

Long-press an Amount and press **Activity** in the existing editor to inspect
the original activity information. Its normal label/value/time controls make
individual edits. **Update label and measure of existing imported amounts** is
checked by default: saving a mapping updates both fields of previously generated
Amounts and refreshes the open Amount editor. It preserves individual overrides,
unsaved editor changes and manually entered Amounts. Uncheck it to apply the
mapping only to future Amounts. If a new measure is unavailable, the existing
Amount keeps its old label and value. Source corrections use each Amount's saved
measure and update unedited generated times.
Deleting an Amount does not cause
the next retrieval to recreate it. Deleting a source in Health Connect keeps
the saved record, marks the source deleted, and keeps the user's Amount.

Generated Amounts are rounded to one decimal place (whole numbers for steps);
the original activity measurements retain their precision. Details use the
system region for km/km/h or mi/mi/h, independently of Juggluco's language.
Elapsed duration and HR coverage use `hours:min:secs`, with a days component when
needed. Amount mappings explicitly offer both distance and speed units; changing
the system region does not change existing Amount values or their saved units.

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

Different origins describing a uniquely matching exercise can share an Amount
when the type matches, start/end are within two minutes, and overlap is at least
90 percent. Their records remain separate. Two different records from the same
origin are not collapsed into the same group.

The saved fields are session ID, origin, device description, title, notes,
type, start/end, modification time, aggregate distance, active calories, steps,
elevation, and original timestamped HR, resting HR, speed, power and cadence
samples. Metrics are read from the session's origin. Samples from distinguishable
devices retain separate source IDs. A summary uses one source, preferring the
session's device, and never averages the sources together. Health Connect
metadata cannot distinguish devices that supply identical or missing device
descriptions. Heart-rate summaries use time weights and cap a sample's carried
coverage at two minutes. Gaps remain uncovered. Resting HR uses the median of
the preceding seven days from the selected source.

`Num` is still 16 bytes. Bit 31 of its existing `mealptr` field tags an activity
index for non-blood Amounts. Its complete 32-bit value still goes to Kerfstok
and returns unchanged. Kerfstok does not receive the activity payload. Meal
rendering, matching and export skip these references. Activity payloads currently
stay on the importing Juggluco device; a number database marked as received from
a mirror does not resolve its indices against an unrelated local activity file.

## Validation

Run the included Linux test from the repository root with a C++26 compiler:

```sh
clang++ -std=c++26 -pthread -O1 -g -fsanitize=undefined \
  Common/src/main/cpp/activity/tests/store_test.cpp -o /tmp/activity-store-test
/tmp/activity-store-test
clang++ -std=c++26 -pthread -O1 -g -fsanitize=undefined \
  Common/src/main/cpp/activity/tests/amount_test.cpp -o /tmp/activity-amount-test
/tmp/activity-amount-test
```

The test covers reopen without serialization, mapping growth, exact-ID replay,
source grouping and ambiguity, source separation, weighted statistics and gaps,
zero-valued speed, resting HR, pending-cursor recovery, unavailable slices and
the unchanged Num representation. The Amount regression test covers the reported
7,134-second / 29,562.3-metre bike ride, mapping changes, source selection,
rounding, missing measures, individual overrides, subsequent retrieval and
metric/imperial conversions. `Common/src/test/java/tk/glucodata/ActivityDisplayTest.java`
contains plain JVM checks for the actual dialog formatting functions.

```sh
javac -d /tmp/activity-format-check \
  Common/src/main/java/tk/glucodata/ActivityFormat.java \
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
