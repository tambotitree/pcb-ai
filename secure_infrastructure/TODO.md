# Secure infrastructure implementation plan

Status: design notes only, 2026-10-26. Checkboxes represent proposed work, not
completed capabilities. Work in small, reviewable increments.

## 1. Evidence format and identity

- [ ] Define persistent design IDs, checkpoint IDs and operation IDs.
- [ ] Specify canonical serialization, schema/version migration and hash domain separation.
- [ ] Specify journal fields: actor/source, timestamp, description, before/after
  state hashes, previous-entry hash and entry hash.
- [ ] Define exact-byte versus normalized design hashing; exclude volatile metadata
  only through an explicit documented rule.
- [ ] Define genesis records, branching, imported files and missing-history handling.
- [ ] Define timestamp trust and the distinct meanings of local time, signed
  timestamps and external anchoring time.

## 2. Audit operation boundaries

- [ ] Trace all application-specific mutation paths listed in README.md.
- [ ] Separate engineering changes from selection, viewport and redraw events.
- [ ] Group gestures and multi-step native operations into completed transactions.
- [ ] Capture undo/redo, AI implementation and external-file load/reload explicitly.
- [ ] Confirm coverage when undo recording is disabled or a mutation bypasses it.
- [ ] Define a shared journal contract usable by both companion applications.

## 3. Local append-only journal

- [ ] Design the journal writer and structured operation API independently of text logs.
- [ ] Define record ordering, append durability, partial-write recovery and locking.
- [ ] Detect concurrent writers and changes made outside the running application.
- [ ] Define behavior for full disk, permission errors and failed record writes.
- [ ] Define opt-in/required recording policy; never silently claim complete history.
- [ ] Protect secrets and define access/retention rules for design evidence.

## 4. Checkpoints and verification

- [ ] Store complete restorable snapshots with checksums and journal references.
- [ ] Add independent verification and recovery tooling.
- [ ] Define optional Git milestone storage without committing every mouse event.
- [ ] Detect modified, deleted, reordered and duplicated records and wrong snapshots.
- [ ] Test crashes during append and checkpoint creation, plus corrupted tails.
- [ ] Demonstrate the two-resistor edit/close/reopen/verify milestone.

## 5. Hardware signing

- [ ] Verify Apple Silicon Secure Enclave capabilities and supported signing algorithms.
- [ ] Define a signer interface suitable for later TPM-backed platforms.
- [ ] Define key creation, application permissions and any required user interaction.
- [ ] Specify public-key registration, rotation, revocation and replacement.
- [ ] Verify old evidence independently of the original laptop or station.
- [ ] Sign checkpoints/attestations and test unavailable or inaccessible hardware.

## 6. Optional blockchain backends

- [ ] Implement `--log-blockchain=local` only after defining the journal contract.
- [ ] Define backend selection at design creation and explicit backend migration.
- [ ] Evaluate an Ethereum backend; select network/chain ID and transaction mechanism.
- [ ] Define minimal public commitments and how private evidence is retrieved.
- [ ] Integrate RPC/API configuration with existing `.env` discovery conventions.
- [ ] Keep API access credentials separate from attestation and transaction signers.
- [ ] Define costs, offline queueing, retries, duplicate prevention, confirmations
  and chain reorganization handling.
- [ ] Distinguish locally recorded, signed, submitted and confirmed checkpoints.
- [ ] Test backend failure without losing local engineering history.

## 7. Manufacturing, calibration and field service

- [ ] Model serialized units, batches, component lots and approved substitutions.
- [ ] Version fixture circuits, firmware, procedures, limits and calibration references.
- [ ] Specify signed measurements, acceptance results and production-release records.
- [ ] Link commissioning, maintenance, field checks and firmware/configuration changes.
- [ ] Demonstrate backward investigation and forward affected-unit queries.
- [ ] Define what a certification asserts, who can issue it and how it expires or
  is superseded; do not equate a signature with engineering correctness.

## 8. Integration and acceptance

- [ ] Specify debug controls separately from evidence recording.
- [ ] Exercise manual and AI edits through the same journal contract.
- [ ] Verify restoration reproduces a checkpoint's exact declared design state.
- [ ] Test record tampering, lost keys, invalid signatures and unavailable networks.
- [ ] Review evidence completeness, privacy, interoperability and operational costs.
- [ ] Update documentation with tested capabilities and remaining limitations.
