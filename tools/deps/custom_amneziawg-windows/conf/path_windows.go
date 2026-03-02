/* SPDX-License-Identifier: MIT
 *
 * Copyright (C) 2019-2021 WireGuard LLC. All Rights Reserved.
 */

package conf

import (
	"errors"
	"os"
	"path/filepath"
	"strings"
	"unsafe"

	"golang.org/x/sys/windows"
)

var cachedConfigFileDir string
var cachedRootDir string

func tunnelConfigurationsDirectory() (string, error) {
	if cachedConfigFileDir != "" {
		return cachedConfigFileDir, nil
	}
	root, err := RootDirectory(true)
	if err != nil {
		return "", err
	}
	c := filepath.Join(root, "Configurations")
	err = os.Mkdir(c, os.ModeDir|0700)
	if err != nil && !os.IsExist(err) {
		return "", err
	}
	cachedConfigFileDir = c
	return cachedConfigFileDir, nil
}

// PresetRootDirectory causes RootDirectory() to not try any automatic deduction, and instead
// uses what's passed to it. This isn't used by amneziawg-windows, but is useful for external
// consumers of our libraries who might want to do strange things.
func PresetRootDirectory(root string) {
	cachedRootDir = root
}

func RootDirectory(create bool) (string, error) {
	if cachedRootDir != "" {
		return cachedRootDir, nil
	}
	root, err := windows.KnownFolderPath(windows.FOLDERID_ProgramFiles, windows.KF_FLAG_DEFAULT)
	if err != nil {
		return "", err
	}
	root = filepath.Join(root, "Windscribe")
	if !create {
		return filepath.Join(root, "config"), nil
	}
	root16, err := windows.UTF16PtrFromString(root)
	if err != nil {
		return "", err
	}

	// The root directory inherits its ACL from Program Files; we don't want to mess with that
	err = windows.CreateDirectory(root16, nil)
	if err != nil && err != windows.ERROR_ALREADY_EXISTS {
		return "", err
	}

	// Allow SYSTEM and Administrators full access, and Authenticated Users read access
	configDirectorySd, err := windows.SecurityDescriptorFromString("O:SYG:SYD:PAI(A;OICI;FA;;;SY)(A;OICI;FA;;;BA)(A;OICI;GR;;;AU)")
	if err != nil {
		return "", err
	}
	configDirectorySa := &windows.SecurityAttributes{
		Length:             uint32(unsafe.Sizeof(windows.SecurityAttributes{})),
		SecurityDescriptor: configDirectorySd,
	}

	config := filepath.Join(root, "config")
	config16, err := windows.UTF16PtrFromString(config)
	if err != nil {
		return "", err
	}
	var configHandle windows.Handle
	for {
		err = windows.CreateDirectory(config16, configDirectorySa)
		if err != nil && err != windows.ERROR_ALREADY_EXISTS {
			return "", err
		}
		configHandle, err = windows.CreateFile(config16, windows.READ_CONTROL|windows.WRITE_OWNER|windows.WRITE_DAC, windows.FILE_SHARE_READ|windows.FILE_SHARE_WRITE|windows.FILE_SHARE_DELETE, nil, windows.OPEN_EXISTING, windows.FILE_FLAG_BACKUP_SEMANTICS|windows.FILE_FLAG_OPEN_REPARSE_POINT|windows.FILE_ATTRIBUTE_DIRECTORY, 0)
		if err != nil && err != windows.ERROR_FILE_NOT_FOUND {
			return "", err
		}
		if err == nil {
			break
		}
	}
	defer windows.CloseHandle(configHandle)
	var fileInfo windows.ByHandleFileInformation
	err = windows.GetFileInformationByHandle(configHandle, &fileInfo)
	if err != nil {
		return "", err
	}
	if fileInfo.FileAttributes&windows.FILE_ATTRIBUTE_DIRECTORY == 0 {
		return "", errors.New("config directory is actually a file")
	}
	if fileInfo.FileAttributes&windows.FILE_ATTRIBUTE_REPARSE_POINT != 0 {
		return "", errors.New("config directory is reparse point")
	}
	buf := make([]uint16, windows.MAX_PATH+4)
	for {
		bufLen, err := windows.GetFinalPathNameByHandle(configHandle, &buf[0], uint32(len(buf)), 0)
		if err != nil {
			return "", err
		}
		if bufLen < uint32(len(buf)) {
			break
		}
		buf = make([]uint16, bufLen)
	}
	if !strings.EqualFold(`\\?\`+config, windows.UTF16ToString(buf[:])) {
		return "", errors.New("config directory jumped to unexpected location")
	}
	err = windows.SetKernelObjectSecurity(configHandle, windows.DACL_SECURITY_INFORMATION|windows.GROUP_SECURITY_INFORMATION|windows.OWNER_SECURITY_INFORMATION|windows.PROTECTED_DACL_SECURITY_INFORMATION, configDirectorySd)
	if err != nil {
		return "", err
	}
	cachedRootDir = config
	return cachedRootDir, nil
}
