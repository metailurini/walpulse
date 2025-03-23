## Roadmap for "Walpulse" Development

### Phase 1: Achieve MVP - Basic WAL Change Listener
**Goal**: Transform the current static WAL parser into a real-time listener that detects changes and invokes a callback with `table_name` and `rowid`.
**Estimated Duration**: 1-2 days (4-8 hours).
**Milestone**: A running program that monitors a SQLite WAL file and prints table name and row ID for new or updated rows.

#### Tasks:
1. **Define Callback Interface**
   - **File**: Create `wal_listener.h`.
   - **Details**: Define `wal_change_callback` typedef and `register_wal_callback` function.
   - **Effort**: 15-30 minutes.
   - **Outcome**: A clean API for users to register change handlers.

2. **Track WAL State**
   - **File**: Update `wal_parser.h` or create `wal_state.h`.
   - **Details**: Add `WalState` struct with `last_file_size`, `last_frame_count`, and `last_checkpoint`.
   - **Effort**: 30 minutes.
   - **Outcome**: Ability to track the WAL file’s state between checks.

3. **Implement Continuous Monitoring**
   - **File**: Create `wal_listener.c`.
   - **Details**: Implement `start_wal_listener` with a polling loop (e.g., check every 1 second using `sleep`).
   - **Dependencies**: Use `derive_db_filename` from `utils.c` to get the WAL file path.
   - **Effort**: 1-2 hours.
   - **Outcome**: A loop that detects when the WAL file grows.

4. **Detect and Process Changes**
   - **File**: Modify `wal_parser.c`.
   - **Details**:
     - Refactor `process_wal_frames` into `process_wal_changes` to skip processed frames (based on `WalState`).
     - Add `process_table_leaf_page` to parse table leaf pages (`0x0D`) and extract `rowid`.
     - Invoke the callback with `table_name` (from `get_table_name_from_page`) and `rowid`.
   - **Dependencies**: Reuse `parse_cell` from `page_analyzer.c` and `get_table_name_from_page` from `db_utils.c`.
   - **Effort**: 2-3 hours.
   - **Outcome**: Callback triggered for new table leaf page changes.

5. **Update Main Entry Point**
   - **File**: Modify `main.c`.
   - **Details**: Replace `print_wal_info` with `start_wal_listener` and register a sample callback (e.g., printing changes).
   - **Effort**: 30 minutes.
   - **Outcome**: Program runs continuously and reports changes.

6. **Test the MVP**
   - **Details**:
     - Create a SQLite database (e.g., `test.db`).
     - Insert/update rows to generate WAL frames.
     - Run `./walpulse test.db` and verify output.
   - **Effort**: 1-2 hours (includes debugging).
   - **Outcome**: Working MVP that meets the basic requirement.

#### Deliverable:
- A command-line tool that monitors `test.db-wal`, detects new table leaf page changes, and calls a callback with `table_name` and `rowid`.

---

### Phase 2: Improve Efficiency and Robustness
**Goal**: Enhance the MVP for better performance and reliability in real-world scenarios.
**Estimated Duration**: 2-3 days (10-15 hours).
**Milestone**: A more efficient and stable listener that handles edge cases.

#### Tasks:
1. **Replace Polling with File System Events**
   - **Why**: Polling is inefficient and delays change detection.
   - **Details**:
     - On Linux: Use `inotify` to watch the WAL file for modifications.
     - On macOS: Use `kqueue` or FSEvents.
     - On Windows: Use `ReadDirectoryChangesW` (requires cross-platform abstraction).
   - **File**: Update `wal_listener.c`.
   - **Effort**: 4-6 hours (platform-specific).
   - **Outcome**: Near-instant change detection without CPU waste.

2. **Handle WAL File Reset/Checkpoints**
   - **Why**: SQLite may reset or truncate the WAL file after a checkpoint.
   - **Details**:
     - Check `header.checkpoint` changes and reset `WalState` if needed.
     - Detect file truncation (size decreases) and reprocess from the start.
   - **File**: Update `process_wal_changes` in `wal_parser.c`.
   - **Effort**: 2-3 hours.
   - **Outcome**: Listener adapts to WAL file lifecycle.

3. **Improve Change Detection**
   - **Why**: Current frame-count-based skipping is fragile if frames are reused.
   - **Details**:
     - Track `salt1` and `salt2` from `FrameHeader` to uniquely identify frames.
     - Store a hash map of processed frame identifiers.
   - **File**: Update `wal_parser.c` and `WalState`.
   - **Effort**: 3-4 hours.
   - **Outcome**: Accurate detection of new/changed frames.

4. **Error Handling and Recovery**
   - **Details**:
     - Add retries for file access failures.
     - Handle corrupt WAL files gracefully (e.g., skip bad frames).
   - **File**: Update `wal_listener.c` and `wal_parser.c`.
   - **Effort**: 1-2 hours.
   - **Outcome**: More resilient to file system or database issues.

#### Deliverable:
- A robust listener that uses file system events, handles WAL resets, and accurately tracks changes.

---

### Phase 3: Extend Functionality
**Goal**: Add features to make "walpulse" more useful for CDC use cases beyond the MVP.
**Estimated Duration**: 3-5 days (15-25 hours).
**Milestone**: A feature-rich tool suitable for basic production use.

#### Tasks:
1. **Support Full Cell Data in Callback**
   - **Why**: Users may want column values, not just `rowid`.
   - **Details**:
     - Extend `wal_change_callback` to include `CellInfo` or raw column data.
     - Use `parse_serial_type` and `print_column_value` to decode values.
   - **File**: Update `wal_listener.h`, `wal_parser.c`, and `page_analyzer.c`.
   - **Effort**: 4-6 hours.
   - **Outcome**: Callback provides detailed row changes.

2. **Distinguish Insert/Update/Delete**
   - **Why**: CDC typically needs operation type.
   - **Details**:
     - Infer operation from WAL context (e.g., new `rowid` = insert, existing = update).
     - Requires tracking previous state (e.g., via a shadow table or cache).
   - **File**: Add a state-tracking module (e.g., `wal_state.c`).
   - **Effort**: 6-8 hours.
   - **Outcome**: Callback includes operation type.

3. **Configuration Options**
   - **Details**:
     - Add command-line flags (e.g., polling interval, verbosity).
     - Support config file for callback registration.
   - **File**: Update `main.c` and add `config.c/h`.
   - **Effort**: 3-4 hours.
   - **Outcome**: User-friendly customization.

4. **Multi-Database Support**
   - **Why**: Monitor multiple SQLite databases simultaneously.
   - **Details**:
     - Use threads or an event loop to manage multiple `start_wal_listener` instances.
   - **File**: Update `wal_listener.c`.
   - **Effort**: 4-6 hours.
   - **Outcome**: Scalable for multi-database setups.

#### Deliverable:
- A versatile tool that provides detailed change data and supports multiple databases.

---

### Phase 4: Production-Ready Features
**Goal**: Polish "walpulse" for production use with logging, testing, and distribution.
**Estimated Duration**: 5-7 days (25-35 hours).
**Milestone**: A stable, documented, and distributable CDC tool.

#### Tasks:
1. **Comprehensive Testing**
   - **Details**:
     - Expand `tests/` with unit tests for `wal_listener.c`, `process_wal_changes`, etc.
     - Simulate WAL changes (e.g., scripted SQLite transactions).
   - **File**: Update `Makefile` and `tests/test_*.c`.
   - **Effort**: 8-10 hours.
   - **Outcome**: High confidence in correctness.

2. **Logging System**
   - **Details**:
     - Replace `printf` with a logging framework (e.g., custom or `log4c`).
     - Add levels (info, debug, error).
   - **File**: Add `logger.c/h` and refactor all files.
   - **Effort**: 4-6 hours.
   - **Outcome**: Structured, configurable logs.

3. **Documentation**
   - **Details**:
     - Write README with setup, usage, and examples.
     - Add inline comments for complex functions.
   - **Effort**: 4-6 hours.
   - **Outcome**: Easy onboarding for users.

4. **Packaging and Distribution**
   - **Details**:
     - Create a `make install` target.
     - Package as a library (optional) with API docs.
   - **File**: Update `Makefile`.
   - **Effort**: 3-5 hours.
   - **Outcome**: Deployable tool/library.

#### Deliverable:
- A production-ready CDC tool with tests, logs, and documentation.

---

### Timeline Summary
| Phase                  | Duration       | Key Deliverable                              |
|------------------------|----------------|---------------------------------------------|
| 1: Achieve MVP         | 1-2 days       | Basic WAL change listener with callback     |
| 2: Efficiency/Robustness | 2-3 days     | Efficient, reliable listener                |
| 3: Extend Functionality| 3-5 days       | Feature-rich CDC tool                       |
| 4: Production-Ready    | 5-7 days       | Stable, documented, distributable tool      |

**Total Estimated Time**: 11-17 days (50-85 hours), depending on complexity and debugging needs.

---

### Recommendations
- **Start with Phase 1**: Focus on the MVP to validate the core idea quickly.
- **Iterate Based on Feedback**: After Phase 1, test with real workloads to prioritize Phase 2 or 3 features.
- **Platform Consideration**: Initially target one OS (e.g., Linux with `inotify`) and expand later.
- **Dependencies**: Keep it lightweight (current SQLite dependency is fine; avoid heavy libraries unless necessary).
