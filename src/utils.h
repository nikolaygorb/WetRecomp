// utils.h - Utility functions for path handling and config loading
#pragma once

#include <filesystem>

#include <rex/cvar.h>
#include <rex/filesystem.h>

namespace utils
{
  // Find the repository root by walking up from the executable location
  static std::filesystem::path RepoRoot()
  {
    namespace fs = std::filesystem;
    fs::path dir = rex::filesystem::GetExecutableFolder();
    for (int i = 0; i < 6 && !dir.empty() && dir.has_parent_path(); ++i)
    {
      if (fs::exists(dir / "perfectdarkzerorecomp_manifest.toml") || fs::exists(dir / "assets"))
      {
        break;
      }
      dir = dir.parent_path();
    }
    return dir;
  }

  // Get the settings directory path
  static std::filesystem::path SettingsDir() { return RepoRoot() / "settings"; }

  // Load all settings files from the settings directory
  static void LoadSettingsFiles()
  {
    namespace fs = std::filesystem;
    fs::path dir = SettingsDir();
    for (const char *file : {"hardware.toml", "mapping.toml"})
    {
      if (fs::path p = dir / file; fs::exists(p))
      {
        rex::cvar::LoadConfig(p);
      }
    }
  }
} // namespace utils
