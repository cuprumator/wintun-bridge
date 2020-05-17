#include "stdafx.h"
#include "wintun.h"

namespace wintun
{
	using namespace std;

	const wstring hardwareID = L"Wintun";
	const GUID giud = { 0x4d36e972, 0xe325, 0x11ce, {0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18} };

	bool IsNewer(SP_DRVINFO_DATA data, FILETIME driverDate, uint64_t driverVersion)
	{
		if (data.DriverDate.dwHighDateTime > driverDate.dwHighDateTime)
		{
			return true;
		}
		if (data.DriverDate.dwHighDateTime < driverDate.dwHighDateTime)
		{
			return false;
		}
		if (data.DriverDate.dwLowDateTime > driverDate.dwLowDateTime)
		{
			return true;
		}
		if (data.DriverDate.dwLowDateTime < driverDate.dwLowDateTime) 
		{
			return false;
		}
		if (data.DriverVersion > driverVersion) 
		{
			return true;
		}
		if (data.DriverVersion < driverVersion) 
		{
			return false;
		}

		return false;
	}

	wstring ToLower(wstring data)
	{
		transform(data.begin(), data.end(), data.begin(),
			[](wchar_t c) { return tolower(c); });

		return data;
	}
	
	wstring GetHardwareId(PSP_DRVINFO_DETAIL_DATA drvDetailData)
	{
		if (drvDetailData->CompatIDsOffset > 1)
		{
			return wstring(drvDetailData->HardwareID, drvDetailData->CompatIDsOffset - 1);
		}

		return L"";
	}
	
	vector<wstring> GetCompatIds(PSP_DRVINFO_DETAIL_DATA drvDetailData)
	{
		vector<wstring> ids;
		if (drvDetailData->CompatIDsLength > 0)
		{
			vector<byte> buffer;
			buffer.assign(reinterpret_cast<byte*>(drvDetailData + drvDetailData->CompatIDsOffset), reinterpret_cast<byte*>(drvDetailData + drvDetailData->CompatIDsOffset + drvDetailData->CompatIDsLength));
			//TODO: implement compat ids parsing
		}

		return ids;
	}
	
	bool IsComatible(wstring hwid, PSP_DRVINFO_DETAIL_DATA drvDetailData)
	{
		if (ToLower(hwid) == ToLower(GetHardwareId(drvDetailData)))
		{
			return true;
		}

		auto ids = GetCompatIds(drvDetailData);
		for(auto id: ids)
		{
			if (ToLower(hwid) == ToLower(id))
			{
				return true;
			}
		}
		
		return false;
	}
		
	void CreateInterface(string name)
	{
		const auto devInfoSet = SetupDiCreateDeviceInfoListEx(&giud, nullptr, nullptr, nullptr);

		//add scope exit and delete devInfo

		wstring className;
		className.resize(MAX_CLASS_NAME_LEN);

		//check result
		SetupDiClassNameFromGuidEx(&giud, &className.front(), MAX_CLASS_NAME_LEN, nullptr, nullptr, nullptr);

		SP_DEVINFO_DATA devInfoData;
		devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
		wstring deviceDescription = L"Assguard Tunnel";

		//check result
		SetupDiCreateDeviceInfo(devInfoSet, &className.front(), &giud, nullptr, nullptr, DICD_GENERATE_ID, &devInfoData);

		SP_DEVINSTALL_PARAMS devInstallParams;
		//check result~
		SetupDiGetDeviceInstallParams(devInfoSet, &devInfoData, &devInstallParams);

		devInstallParams.Flags |= DI_QUIETINSTALL;

		//check result
		SetupDiSetDeviceInstallParams(devInfoSet, &devInfoData, &devInstallParams);

		//check result
		SetupDiSetSelectedDevice(devInfoSet, &devInfoData);

		//check result
		auto res = SetupDiSetDeviceRegistryProperty(devInfoSet, &devInfoData, SPDRP_HARDWAREID, reinterpret_cast<const byte*>(hardwareID.c_str()), sizeof(hardwareID));

		//check result
		SetupDiBuildDriverInfoList(devInfoSet, &devInfoData, SPDIT_COMPATDRIVER);

		//add scope exit and delete with SetupDiDestroyDriverInfoList

		FILETIME driverDate = {};
		uint64_t driverVersion = 0;
		for (auto i = 0; ; i++)
		{
			SP_DRVINFO_DATA drvInfoData;
			drvInfoData.cbSize = sizeof(SP_DRVINFO_DATA);

			if (!SetupDiEnumDriverInfo(devInfoSet, &devInfoData, SPDIT_COMPATDRIVER, i, &drvInfoData))
			{
				if (GetLastError() == ERROR_NO_MORE_ITEMS)
				{
					break;
				}
				continue;
			}

			if (IsNewer(drvInfoData, driverDate, driverVersion))
			{
				DWORD reqSize = 2048;
				vector<byte> buffer(reqSize);
				PSP_DRVINFO_DETAIL_DATA drvDetailData = reinterpret_cast<PSP_DRVINFO_DETAIL_DATA>(&buffer.at(0));
				drvDetailData->cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);

				if (!SetupDiGetDriverInfoDetail(devInfoSet, &devInfoData, &drvInfoData, drvDetailData, sizeof(byte) * reqSize, &reqSize))
				{
					auto err = GetLastError();
					if (err == ERROR_INSUFFICIENT_BUFFER)
					{
						continue;
					}
				}

				drvDetailData->cbSize = reqSize;

				if (IsComatible(hardwareID, drvDetailData))
				{
					//set selected driver
					if (!SetupDiSetSelectedDriver(devInfoSet, &devInfoData, &drvInfoData) && GetLastError() != NULL)
					{
						continue;
					}

					driverDate = drvInfoData.DriverDate;
					driverVersion = drvInfoData.DriverVersion;
				}
			}

			if (driverVersion == 0)
			{
				//error "No driver for device %q installed"
			}

			SetupDiCallClassInstaller(DIF_REGISTERDEVICE, devInfoSet, &devInfoData);
			SetupDiCallClassInstaller(DIF_REGISTER_COINSTALLERS, devInfoSet, &devInfoData);
			
			SetupDiCallClassInstaller(DIF_INSTALLINTERFACES, devInfoSet, &devInfoData);
			SetupDiCallClassInstaller(DIF_INSTALLDEVICE, devInfoSet, &devInfoData);


		}
	}
}
