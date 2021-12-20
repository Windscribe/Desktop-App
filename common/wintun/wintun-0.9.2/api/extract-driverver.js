/* SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (C) 2018-2020 WireGuard LLC. All Rights Reserved.
 */

while (!WScript.StdIn.AtEndOfStream) {
	var line = WScript.StdIn.ReadLine();
	if (line.substr(0, 12) != "DriverVer = ")
		continue;
	var val = line.substr(12).split(",");
	var date = val[0].split("/");
	var ver = val[1].split(".");
	var time = Date.UTC(date[2], date[0] - 1, date[1]).toString()
	WScript.Echo("#define WINTUN_INF_FILETIME { (DWORD)((" + time + "0000ULL + 116444736000000000ULL) & 0xffffffffU), (DWORD)((" + time + "0000ULL + 116444736000000000ULL) >> 32) }")
	WScript.Echo("#define WINTUN_INF_VERSION ((" + ver[0] + "ULL << 48) | (" + ver[1] + "ULL << 32) | (" + ver[2] + "ULL << 16) | (" + ver[3] + "ULL << 0))")
	break;
}
