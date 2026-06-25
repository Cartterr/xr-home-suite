# Security Policy

This repository is public. Treat every file as visible to the internet.

## Secret Handling

- Do not commit API keys, OAuth tokens, certificates, private keys, `.env` files, auth JSON, or service credentials.
- Do not paste secrets into docs, logs, reports, screenshots, or issue templates.
- Use machine-local environment variables or secure OS storage for credentials.
- Keep generated reports under `C:\XRHomeSuite\artifacts` unless they have been reviewed for public sharing.

## Reporting A Problem

Open a GitHub issue for non-sensitive problems. For anything involving credentials, private camera access, or device security boundaries, do not attach raw logs until they have been scrubbed.

## Meta/OpenXR Boundary

The project must use public APIs for product behavior. Private Meta/OpenXR camera extensions are diagnostic-only unless Meta publishes a supported ABI or the work is isolated in a clearly marked research branch.
