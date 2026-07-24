# Security Policy

Windscribe takes the security of our software and our users seriously. This
document explains how to report security vulnerabilities in the Windscribe
Desktop application and what you can expect from us in return.

## Vulnerability Disclosure Policy

Windscribe operates a formal Vulnerability Disclosure Program (VDP). Before
reporting, please read the full policy, including scope, rules of engagement,
and safe-harbor terms:

> **https://windscribe.com/vdp**

By submitting a report you agree to the terms described in that policy.

## Reporting a Vulnerability

**Please do not report security vulnerabilities through public GitHub issues,
pull requests, or discussions.**

Instead, report them through one of the following private channels:

- **Preferred:** Submit your report in accordance with our Vulnerability
  Disclosure Policy at https://windscribe.com/vdp
- **Email:** Send details to **security@windscribe.com** with a subject line
  beginning with `[SECURITY]`.

For sensitive reports, please encrypt your message using our [PGP key](https://windscribe.com/windscribe_pgp.txt).

To help us triage and resolve the issue as quickly as possible, please include
as much of the following as you can:

- A description of the vulnerability and its potential impact.
- The affected component, platform, and application version (see the About
  screen, or `src/client/client-common/version/`).
- Step-by-step instructions to reproduce the issue.
- Proof-of-concept code, scripts, or screenshots where applicable.
- Any relevant logs, configurations, or environment details.
- Your assessment of severity and any suggested remediation.

Please report each distinct vulnerability separately.

## What to Expect

When you submit a report, you can expect Windscribe to:

- **Acknowledge** receipt of your report in a timely manner.
- **Investigate** and validate the reported issue, and keep you informed of our
  progress.
- **Remediate** confirmed vulnerabilities in a manner commensurate with their
  severity and risk to users.
- **Credit** you for the discovery once the issue is resolved, if you wish to be
  acknowledged.

We ask that you give us a reasonable amount of time to investigate and address
the issue before any public disclosure, and that you make a good-faith effort to
avoid privacy violations, data destruction, and service disruption during your
research. Full details are described in our
[Vulnerability Disclosure Policy](https://windscribe.com/vdp).

## Scope

This policy applies to the Windscribe Desktop application contained in this
repository, across all supported platforms (Windows, macOS, and Linux) and its
components, including the GUI, CLI, VPN engine, and the privileged helper
service.

Note that this repository is a mirror of Windscribe's internal development
repository and is updated only on public releases. The full, authoritative scope
for the disclosure program — including other Windscribe products, apps, and web
properties — is defined at https://windscribe.com/vdp.

## Supported Versions

Security fixes are applied to the current release line of the Windscribe Desktop
application. We strongly recommend that all users run the latest available
version, which can always be downloaded from
[windscribe.com/download](https://windscribe.com/download).

## Safe Harbor

Windscribe supports safe-harbor protections for security research conducted in
good faith and in accordance with our
[Vulnerability Disclosure Policy](https://windscribe.com/vdp). We will not pursue
or support legal action against researchers who comply with that policy. If in
doubt about whether an action is authorized, contact us before proceeding.

Thank you for helping keep Windscribe and our users safe.
