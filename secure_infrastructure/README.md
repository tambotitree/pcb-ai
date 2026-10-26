# PCB-ai secure infrastructure

Status: proposed architecture, 2026-10-26.

## Purpose

Preserve engineering evidence across design, manufacturing, calibration and field
service. Traceability supports investigation and corrective action; it is not a
mechanism for assigning blame to an individual.

This is a proposed architecture, not an implemented security or certification
system. No blockchain backend, signing integration or complete edit journal is
claimed by these documents.

## Proposed architecture

1. **Design identity:** a persistent design ID and explicit revision/checkpoint IDs.
2. **Change journal:** one structured entry per completed engineering operation,
   identifying its actor, description, before/after state and prior entry hash.
3. **Checkpoints:** complete, restorable design snapshots, optionally stored in Git.
4. **Signatures:** hardware-protected signing of checkpoints and attestations.
5. **Anchoring:** optional publication of signed checkpoint commitments to a
   selected external blockchain. Detailed designs and measurements remain in
   retrievable storage unless their publication is explicitly intended.
6. **Lifecycle evidence:** links to manufactured unit IDs, component lots, firmware,
   test fixtures, calibration references, measurements and release decisions.

A conceptual journal link is:

```text
entry_hash = SHA256(canonical_encode(previous_hash, change_record, resulting_design_hash))
```

Canonical encoding, format versioning and domain separation must be specified
before implementation. A hash chain detects changes relative to a trusted
reference; it cannot prevent someone from rewriting an entire unanchored chain.
A signature identifies a registered signing key. Measurement accuracy depends on
controlled procedures and calibrated references, not on hashes or signatures.

## Proposed configuration

```sh
--log-blockchain=local
--log-blockchain=ethereum
```

These are proposed options, not currently supported commands. `local` would enable
local hash-chained evidence; external backends would anchor selected checkpoints,
not require a blockchain transaction for every mouse gesture.

Service configuration could be discovered through the existing environment and
`.env` conventions:

```dotenv
BLOCKCHAIN_RPC_URL=https://your-provider.example
BLOCKCHAIN_API_KEY=your-provider-api-key
```

An API key grants service access; it is not a signing identity. Do not place
private signing keys in this example, logs, journal entries or version control.
Network/chain identity and public signer identity belong in the evidence record.
Provider-specific credential names, discovery precedence and backend settings
remain to be designed. Do not couple journal recording to debug verbosity.

## Hardware-backed signing

The initial development platform is Apple Silicon macOS. Investigate Secure
Enclave integration through Apple security APIs rather than assuming a conventional
PC TPM interface. Verify available key algorithms, access controls and signing
behavior on the actual laptop. Other platforms can use TPM-backed implementations
through a defined signer interface.

Keep checkpoint/attestation signing separate from blockchain transaction signing:
the selected chain may require a different algorithm or wallet. Define key
registration, rotation, revocation, replacement and evidence verification before
claiming durable identity. Historical signatures must remain verifiable when the
original laptop or test station is unavailable.

## Recording boundaries

Capture placement, movement, attribute changes, wiring/routing, deletion, undo,
redo and implemented AI edits. Group a completed gesture or reviewed proposal as
one operation. Viewing, zooming and redraws are not engineering modifications.
Undo appends a new record; it does not erase prior journal history.

Loading or detecting an externally edited file requires explicit evidence. We can
record observed snapshots and their differences, but cannot reconstruct every
unobserved editing action or reliably identify its actor.

Journal recording must work when undo recording is disabled. Audit coverage of
native mutation paths; an undo hook or dirty flag alone is not proof of complete
capture. Define crash consistency, concurrent writers, missing history, recovery,
and behavior when a required record cannot be written.

## Manufacturing and calibration extension

A test run could link an individual device to the exact design, BOM, firmware,
fixture circuit, fixture firmware, procedure, acceptance limits and calibration
references used. Record measurements, result, station identity and authorization
for production release. The fixture has its own versioned and signed history.

This enables tracing backward to possible causes and forward to other affected
units. Unrecorded substitutions, repairs and process steps remain evidence gaps.
A field device remaining in specification requires ongoing trustworthy checks;
a valid factory certificate alone cannot establish continuing accuracy.

## First milestone

Use a disposable two-resistor design. Change one value, record the completed
operation, create a checkpoint, close, reopen and verify the journal and snapshot.
Then exercise undo/redo and tampering detection. Establish this local foundation
before requiring any external blockchain service.

See [TODO.md](TODO.md) for the staged work.

## Existing hooks to investigate

- `src/undo.c`: typed native undo records and serial-number transaction grouping.
- `src/set.c`: changed-state flags, which do not by themselves describe a change.
- `src/hid/gtk/pcb_ai_edit.c`: guarded native operation implementation.
- `src/hid/gtk/pcb_ai_patch.c`: whole-board reviewed patch transactions and undo.
- Native save/load and external-file monitoring paths: audit separately.
- Diagnostic message/log-window infrastructure: retain for troubleshooting,
  separate from durable design evidence.

Companion design notes: [gEDA-ai](../../secure_infrastructure/README.md) in the
nested development checkout. They are separate repositories; this relative link
is not guaranteed in a standalone clone.
