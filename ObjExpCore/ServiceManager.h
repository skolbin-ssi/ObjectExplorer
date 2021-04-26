#pragma once

#include <vector>
#include "ServiceInfo.h"
#include "Sid.h"
#include <memory>
#include <string>

namespace WinSys {
	struct ServiceInfo;

	enum class ServiceAccessMask : uint32_t {
		None = 0,
		AllAccess = SERVICE_ALL_ACCESS,
		QueryConfig = SERVICE_QUERY_CONFIG,
		ChangeConfig = SERVICE_CHANGE_CONFIG,
		QueryStatus = SERVICE_QUERY_STATUS,
		EnumerateDependents = SERVICE_ENUMERATE_DEPENDENTS,
		Start = SERVICE_START,
		Stop = SERVICE_STOP,
		PauseContinue = SERVICE_PAUSE_CONTINUE,
		Interrogate = SERVICE_INTERROGATE,
		UserDefinedControl = SERVICE_USER_DEFINED_CONTROL,
		Delete = DELETE
	};
	DEFINE_ENUM_FLAG_OPERATORS(ServiceAccessMask);

	enum class ServiceStartType {
		Boot = 0,
		System = 1,
		Auto = 2,
		Demand = 3,
		Disabled = 4,
	};

	enum class ServiceErrorControl {
		Ignore,
		Normal,
		Severe,
		Critical
	};

	struct ServiceConfiguration {
		ServiceType Type;
		ServiceStartType  StartType;
		ServiceErrorControl ErrorControl;
		std::wstring BinaryPathName;
		std::wstring LoadOrderGroup;
		uint32_t Tag;
		std::vector<std::wstring> Dependencies;
		std::wstring AccountName;
		std::wstring DisplayName;
		bool DelayedAutoStart;
		bool TriggerStart;
	};

	struct ServiceInstallParams {
		std::wstring ServiceName;
		std::wstring DisplayName;
		std::wstring AccountName;
		std::wstring Password;
		std::wstring ImagePath;
		std::wstring TargetPath;
		std::wstring Dependencies;
		std::wstring LoadOrderGroup;
		WinSys::ServiceStartType StartupType;
		WinSys::ServiceType ServiceType;
		WinSys::ServiceErrorControl ErrorControl;
		bool DelayedAutoStart;
	};

	class ServiceManager final abstract {
		friend class Service;
	public:
		static std::vector<ServiceInfo> EnumServices(ServiceEnumType enumType, ServiceEnumState enumState = ServiceEnumState::All);
		static std::unique_ptr<ServiceConfiguration> GetServiceConfiguration(const std::wstring& serviceName);
		static std::wstring GetServiceDescription(const std::wstring& name);
		static ServiceState GetServiceState(const std::wstring& name);
		static ServiceStatusProcess GetServiceStatus(const std::wstring& name);
		static bool Uninstall(const std::wstring& name);

		static std::unique_ptr<Service> Install(const std::wstring& name, const std::wstring& displayName, ServiceAccessMask desiredAccess, ServiceType type,
			ServiceStartType startType, ServiceErrorControl errorControl, const std::wstring& imagePath);
		static std::unique_ptr<Service> Install(const std::wstring& name, ServiceType type,	ServiceStartType startType, const std::wstring& imagePath);
		static std::unique_ptr<Service> Install(const ServiceInstallParams& params);
		static Sid GetServiceSid(const wchar_t* name);

	private:
		static wil::unique_schandle OpenServiceHandle(const std::wstring& name, ServiceAccessMask accessMask = ServiceAccessMask::QueryConfig | ServiceAccessMask::QueryStatus);
	};
}
