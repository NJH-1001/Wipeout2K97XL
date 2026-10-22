#include "mod_packages.h"
#include <cassert>
#include <filesystem>
#include <iostream>
using namespace PSXRecompV4;
int main(int argc,char** argv) {
    assert(argc==2);
    const char* package="wxl.presentation.render60";
    const char* feature="render60";
    unsigned activated=0;
    /* The resolver must select the trusted implementation, never code from
     * the package. This fixture exercises selection without a GPU context. */
    static unsigned* activation_count;
    activation_count=&activated;
    assert(mod_register_activation_plugin("wxl.render60",+[]{++*activation_count;}));
    std::string error;
    ModPackageManager manager(argv[1]);
    assert(manager.scan(&error) && manager.scan_errors().empty());
    const auto* declared=manager.selected_feature(package,feature);
    assert(declared && declared->group=="Visual");
    assert(!manager.feature_enabled(package,feature));
    assert(manager.feature_option_value(package,feature,"frame-rate")=="60");
    assert(manager.set_feature_option(package,feature,"frame-rate","90",&error));
    auto off=manager.resolve("SCUS-94351");
    assert(off.ok && off.plugins.empty() && off.writes.empty() && off.overlays.empty());
    assert(manager.set_feature_enabled(package,feature,true,&error));
    assert(manager.save_state(&error));
    ModPackageManager loaded(argv[1]);
    assert(loaded.scan(&error) && loaded.load_state(&error));
    assert(loaded.feature_enabled(package,feature));
    assert(loaded.feature_option_value(package,feature,"frame-rate")=="90");
    assert(loaded.set_feature_option(package,feature,"frame-rate","120",&error));
    assert(!loaded.set_feature_option(package,feature,"frame-rate","144",&error));
    auto on=loaded.resolve("SCUS-94351");
    assert(on.ok && on.plugins.size()==1 && on.plugins[0].id=="wxl.render60");
    assert(on.writes.empty() && on.overlays.empty() && on.derived_discs.empty());
    mod_invoke_activation_plugin(on.plugins[0].id);
    assert(activated==1);
    assert(loaded.set_feature_enabled(package,feature,false,&error));
    assert(loaded.save_state(&error));
    ModPackageManager reloaded(argv[1]);
    assert(reloaded.scan(&error) && reloaded.load_state(&error));
    assert(!reloaded.feature_enabled(package,feature));
    assert(reloaded.feature_option_value(package,feature,"frame-rate")=="120");
    assert(reloaded.resolve("SCUS-94351").plugins.empty());
    std::cout<<"PASS: default off, Visual group, persistent checkbox, trusted activation and no guest patches\n";
}
