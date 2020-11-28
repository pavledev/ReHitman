#include <BloodMoney/Client.h>

#include <G1ConfigurationService.h>
#include <HF/HackingFramework.hpp>
#include <spdlog/spdlog.h>

/// Delegates
#include <BloodMoney/Delegates/DX9Delegate.h>
#include <BloodMoney/Delegates/ImGuiInputDelegate.h>

/// Patches
#include <BloodMoney/Patches/All/ZHitman3Patches.h>
#include <BloodMoney/Patches/All/ZDirect3D9DevicePatches.h>
#include <BloodMoney/Patches/All/InputDevicesPatches.h>
#include <BloodMoney/Patches/All/ZGEOMManagementPatch.h>
#include <BloodMoney/Patches/All/EnableCheatsPatch.h>
#include <BloodMoney/Patches/All/ZCutSequencePlayerPatch.h>

namespace Hitman::BloodMoney
{
    static constexpr std::string_view kDirectX9DllName = "d3d9.dll";
    static constexpr std::string_view kProcessName = "HitmanBloodMoney.exe";

    bool Client::OnAttach()
    {
        spdlog::info("----------[ WELCOME TO RE:HITMAN PROJECT ]----------");

        if (!LocateModules()) return false;

        // Register game configuration
        if (!RegisterGameConfigurationForGlacier())
        {
            spdlog::error("Failed to register game configuration!");
            return false;
        }

        m_patches = std::make_shared<CommonPatches>(m_selfProcess, m_selfModule, m_d3d9Module);
        RegisterPatches();
        if (!m_patches->Setup())
        {
            spdlog::error("Failed to setup patches!");
            return false;
        }

        return true;
    }

    void Client::OnDestroy()
    {
        m_patches->Release();
        ReleaseModules();
    }

    bool Client::RegisterGameConfigurationForGlacier()
    {
        auto& config = Glacier::G1ConfigurationService::GetInstance();

        // Setup embedded functions
        config.G1API_FunctionAddress_ZGROUP_CreateGeom    = 0x004EA060;
        config.G1API_FunctionAddress_ZHumanBoid_SetTarget = 0x00585670;

        return true;
    }

    bool Client::LocateModules()
    {
        auto PrintModInfo = [](const HF::Win32::Module::Ptr& mod)
        {
            if (!mod) {
                spdlog::error("BAD MODULE");
                return;
            }

            spdlog::info("Module: {} base address at {:08x} of size {:08x}", mod->getName(), mod->getBaseAddress(), mod->getSize());
        };

        m_selfProcess = std::make_shared<HF::Win32::Process>(kProcessName);
        spdlog::info("Trying to locate modules...");

        if (!m_selfProcess->isValid())
        {
            spdlog::error("Failed to locate {} process", kProcessName);
            return false;
        }

        m_selfModule = m_selfProcess->getSelfModule();
        if (!m_selfModule)
        {
            spdlog::error("Failed to locate self module!");
            return false;
        }

        m_d3d9Module = m_selfProcess->getModule(kDirectX9DllName);
        if (!m_d3d9Module)
        {
            spdlog::error("Failed to locate {} module!", kDirectX9DllName);
            return false;
        }

        PrintModInfo(m_selfModule);
        PrintModInfo(m_d3d9Module);

        spdlog::info("All required modules located!");

        return true;
    }

    void Client::ReleaseModules()
    {
        m_selfModule = nullptr;
        m_d3d9Module = nullptr;
        m_selfProcess = nullptr;
    }

    void Client::RegisterPatches()
    {
        m_patches->RegisterPatch<ZDirect3D9DevicePatches>(std::make_unique<DX9Delegate>());
        m_patches->RegisterPatch<InputDevicesPatches>(std::make_unique<ImGuiInputDelegate>());
        m_patches->RegisterPatch<ZHitman3Patches>();
        //m_patches->RegisterPatch<ZGEOMManagementPatch>();
        m_patches->RegisterPatch<EnableCheatsPatch>();
        m_patches->RegisterPatch<ZCutSequencePlayerPatch>();
    }
}