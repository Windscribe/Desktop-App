**wssecure** is a library used on Windows to mitigate against code-injection attacks.

It utilizes Microsoft's `SetProcessMitigationPolicy` API to block loading of any DLL not signed by Microsoft, and blocks the process from generating dynamic code or modify existing executable code.

The library is implicitly linked with the client app, ensuring the DLL is loaded and its `DLL_PROCESS_ATTACH` section run before `WinMain()` executes.

This blocks the attack where a local attacker:
- Starts the client app process suspended.
- Creates a remote thread in the process with high priority which runs code to load a malicious DLL into the process.
- Resumes the process.
- The high priority thread will run before WinMain(), and before any globally static objects are created.

This library does NOT prevent an attack where:
- The local attacker modifies the executable's library loader table, causing the Windows loader to load their malicious DLL before this mitigation DLL is loaded.
