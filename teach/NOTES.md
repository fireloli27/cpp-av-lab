# Teaching notes

- User asked via `/teach` about issue-02 AV knowledge (PCM, WAV), then encode (Opus/CAF).
- Prefer Chinese prose; keep API/format names in English (AGENTS.md Doc language + this README).
- **Doc SSOT:** stage format contract lives in `docs/learning/audio/{stage}.notes.md`; short essay = runbook; this tree under `teach/` = classroom only. Do not triplicate contract numbers.
- 2026-10-20: classroom relocated from repo root → `teach/`.
- Do not conflate linear PCM with G.711 telephony PCM.
- 2026-10-22: mission shifted to encode; lesson 0002 Opus≠CAF; do not conflate Opus codec with CAF container; bitrate byte-math vs PCM 96k B/s.
- 2026-10-22: lesson 0003 on 64 kbps — bit≠byte, vs G.711 64 kbit/s coincidence; user asked explicitly what bitrate means.
- 2026-10-23: mission shifted to local-transport (issue 04); lessons 0004–0005 + transport-unit cheatsheet; do not conflate frames_per_packet with Transport Unit; learning length-prefix ≠ product must-RTP.
