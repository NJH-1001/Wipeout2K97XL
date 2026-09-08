#include "mod_packages.h"
#include <cassert>
#include <filesystem>
#include <iostream>
using namespace PSXRecompV4;
int main(int argc,char**argv) {
 assert(argc==2); std::string error; const char* pkg="wxl.controls.negcon"; const char* feat="dual-sticks";
 ModPackageManager m(argv[1]); assert(m.scan(&error));
 assert(m.feature_enabled(pkg,feat));
 assert(m.feature_option_value(pkg,feat,"brakes")=="reversed");
 assert(m.set_feature_option(pkg,feat,"brakes","normal",&error));
 assert(m.set_feature_option(pkg,feat,"pitch","left",&error));
 assert(!m.set_feature_option(pkg,feat,"brakes","invalid",&error));
 assert(m.save_state(&error));
 ModPackageManager r(argv[1]); assert(r.scan(&error)); assert(r.load_state(&error));
 assert(r.feature_option_value(pkg,feat,"brakes")=="normal");
 assert(r.feature_option_value(pkg,feat,"pitch")=="left");
 assert(r.set_feature_enabled(pkg,feat,false,&error)); assert(r.save_state(&error));
 ModPackageManager off(argv[1]); assert(off.scan(&error)); assert(off.load_state(&error));
 assert(!off.feature_enabled(pkg,feat));
 assert(off.feature_option_value(pkg,feat,"brakes")=="normal");
 std::cout<<"PASS: real catalog defaults, option validation, persistence, disable preserves choices\n";
}
