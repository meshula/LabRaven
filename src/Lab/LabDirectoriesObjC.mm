//
//  LabDirectoriesObjC.cpp
//  LabExcelsior
//
//  Created by Nick Porcino on 1/28/24.
//  Copyright © 2024 Nick Porcino. All rights reserved.
//

#import <Foundation/Foundation.h>
#include <stdio.h>
#include <string>
#include <map>
#if 0
namespace {
   std::map<std::string, bool> prefs;
}

extern "C"
const char* lab_pref_for_key(const char* key) {
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    NSString *path = [defaults objectForKey:[NSString stringWithUTF8String:key]];
    if (!path)
        return nullptr;
    
    std::string value = [path UTF8String];
    auto it = prefs.find(value);
    if (it == prefs.end()) {
        prefs[value] = true;
        it = prefs.find(value);
        return it->first.c_str();
    }
    prefs[value] = true;
    it = prefs.find(value);
    return it->first.c_str();
}

extern "C"
void lab_set_pref_for_key(const char* key, const char* value) {
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    NSString *path = [NSString stringWithUTF8String:value];
    [defaults setObject:path forKey:[NSString stringWithUTF8String:key]];
    [defaults synchronize];
}
#endif

