#include "mia.hpp"

#include <TZXFile.h>

namespace mia {

function<string ()> homeLocation = [] { return string{Path::user(), "Emulation/Systems/"}; };
function<string ()> saveLocation = [] { return string{}; };
vector<string> media;

auto locate(const string& name) -> string {
  // First, check the application directory
  // This allows ares to function in 'portable' mode
  string location = {Path::program(), name};
  if(inode::exists(location)) return location;

  // On macOS, also check the AppBundle Resource path
#if defined(PLATFORM_MACOS)
  location = {Path::program(), "../Resources/", name};
  if(inode::exists(location)) return location;
#endif

  // Check the userData directory, this is the default
  // on non-windows platforms for any resouces that did not
  // ship with the executable.
  // On Windows, this allows settings from to be carried over
  // from previous versions (pre-portable)
  location = {Path::userData(), "ares/", name};
  if(inode::exists(location)) return location;

  // On non-windows platforms, this time check the shared
  // data directory, on Windows, default to program dir
  // this ensures Portable mode is the default on Windows platforms.
#if !defined(PLATFORM_WINDOWS)
  string shared_location = {Path::sharedData(), "ares/", name};
  if(inode::exists(shared_location)) return shared_location;

  // On non-windows platforms, after exhausting other options,
  // default to userData
  directory::create({Path::userData(), "ares/"});
  return {Path::userData(), "ares/", name};
#else
  return {Path::program(), name};
#endif
}

auto operator+=(string& lhs, const string& rhs) -> string& {
  lhs.append(rhs);
  return lhs;
}

auto hexString(array_view<u8> view) -> string {
  string s;
  for(u8 n : view) s.append(hex(n, 2L), " ");
  return s.stripRight();
}

#include "settings/settings.cpp"
#include "system/system.cpp"
#include "medium/medium.cpp"
#include "pak/pak.cpp"
#if !defined(MIA_LIBRARY)
#include "program/program.cpp"
#endif

auto setHomeLocation(function<string ()> callback) -> void {
  homeLocation = callback;
}

auto setSaveLocation(function<string ()> callback) -> void {
  saveLocation = callback;
}

auto construct() -> void {
  static bool initialized = false;
  if(initialized) return;
  initialized = true;

  media.append("Atari 2600");
  media.append("BS Memory");
  media.append("ColecoVision");
  media.append("MyVision");
  media.append("Famicom");
  media.append("Famicom Disk System");
  media.append("Game Boy");
  media.append("Game Boy Color");
  media.append("Game Boy Advance");
  media.append("Game Gear");
  media.append("Master System");
  media.append("Mega Drive");
  media.append("Mega 32X");
  media.append("Mega CD");
  media.append("MSX");
  media.append("MSX2");
  media.append("Neo Geo");
  media.append("Neo Geo Pocket");
  media.append("Neo Geo Pocket Color");
  media.append("Nintendo 64");
  media.append("Nintendo 64DD");
  media.append("PC Engine");
  media.append("PC Engine CD");
  media.append("PlayStation");
  media.append("Pocket Challenge V2");
  media.append("Saturn");
  media.append("SC-3000");
  media.append("SG-1000");
  media.append("Sufami Turbo");
  media.append("Super Famicom");
  media.append("SuperGrafx");
  media.append("WonderSwan");
  media.append("WonderSwan Color");
  media.append("ZX Spectrum");
}

auto identify(const string& filename) -> string {
  construct();
  auto extension = Location::suffix(filename).trimLeft(".", 1L).downcase();

  if(extension == "zip") {
    Decode::ZIP archive;
    if(archive.open(filename)) {
      for(auto& file : archive.file) {
        auto match = Location::suffix(file.name).trimLeft(".", 1L).downcase();
        for(auto& medium : media) {
          auto pak = mia::Medium::create(medium);
          if(pak->extensions().find(match)) {
            extension = match;
          }
        }
      }
    }
  }

  for(auto& medium : media) {
    auto pak = mia::Medium::create(medium);
    if(pak->extensions().find(extension)) {
      return pak->name();
    }
  }

  return {};  //unable to identify
}

auto import(shared_pointer<Pak> pak, const string& filename) -> bool {
  if(pak->load(filename)) {
    string pathname = {Path::user(), "Emulation/", pak->name(), "/", Location::prefix(filename), ".", pak->extensions().first(), "/"};
    if(!directory::create(pathname)) return false;
    for(auto& node : *pak->pak) {
      if(auto input = node.cast<vfs::file>()) {
        if(input->name() == "manifest.bml" && !settings.createManifests) continue;
        if(auto output = file::open({pathname, input->name()}, file::mode::write)) {
          while(!input->end()) output.write(input->read());
        }
      }
    }
    return true;
  }
  return false;
}

auto main(Arguments arguments) -> void {
  #if !defined(MIA_LIBRARY)
  Application::setName("mia");
  #endif

  construct();

  if(auto document = file::read(locate("settings.bml"))) {
    settings.unserialize(document);
  }

  if(arguments.take("--name")) {
    return print("mia");
  }

  if(string filename; arguments.take("--identify", filename)) {
    return print(identify(filename), "\n");
  }

  if(string system; arguments.take("--system", system)) {
    auto pak = mia::Medium::create(system);
    if(!pak) return;

    if(string manifest; arguments.take("--manifest", manifest)) {
      if(pak->load(manifest)) {
        if(auto fp = pak->pak->read("manifest.bml")) return print(fp->reads());
      }
      return;
    }

    if(string import; arguments.take("--import", import)) {
      return (void)mia::import(pak, import);
    }

    #if !defined(MIA_LIBRARY)
    if(arguments.take("--import")) {
      if(auto import = BrowserDialog()
      .setTitle({"Import ", system, " Game"})
      .setPath(settings.recent)
      .setAlignment(Alignment::Center)
      .openFile()
      ) {
        if(!mia::import(pak, import)) {
          MessageDialog()
          .setTitle("Error")
          .setAlignment(Alignment::Center)
          .setText({"Failed to import: ", Location::file(import)})
          .error();
        }
      }
      return;
    }
    #endif
  }

  #if !defined(MIA_LIBRARY)
  Instances::programWindow.construct();

  #if defined(PLATFORM_MACOS)
  Application::Cocoa::onAbout([&] { programWindow.aboutAction.doActivate(); });
  Application::Cocoa::onPreferences([&] {});
  Application::Cocoa::onQuit([&] { Application::quit(); });
  #endif

  programWindow.setVisible();
  Application::run();

  Instances::programWindow.destruct();
  #endif

  file::write(locate("settings.bml"), settings.serialize());
}

}

#if !defined(MIA_LIBRARY)
#include <nall/main.hpp>

auto nall::main(Arguments arguments) -> void {
  mia::main(arguments);
}
#endif
